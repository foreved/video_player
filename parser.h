#ifndef PARSER_H
#define PARSER_H

#include "ff_class.h"
#include "ring_buffer.h"
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <optional>
#include <functional>

class Parser
{
public:
    std::string url_;
    
public:
    using SharedItem = std::unique_ptr<Frame>;
    using SharedQueue = RingBufferSPSC<std::unique_ptr<Frame>>;
    using SharedQueuePtr = std::shared_ptr<RingBufferSPSC<std::unique_ptr<Frame>>>;
    
    explicit Parser(SharedQueue &q)
        : rgb_frame_queue_{q}   {}
    ~Parser() = default;

    bool open(const char *url, const char *opt)
    {
        auto ret1 = Demuxer::create(url, opt);
        if (!ret1.has_value())
            return false;
        this->demuxer_ = std::move(ret1.value());

        auto ret2 = Decoder::create(*this->demuxer_, FF_StreamType::Video);
        if (!ret2.has_value())
            return false;
        this->decoder_ = std::move(ret2.value());
        
        auto ret3 = Scaler::create(this->decoder_->width_, this->decoder_->height_, this->decoder_->pix_fmt_, \
                                   this->decoder_->width_, this->decoder_->height_, FF_ColorFormat::RGB24);
        if (!ret3.has_value())
            return false;
        this->scaler_ = std::move(ret3.value());
        
        this->url_ = url;

        return true;
    }
    void close()
    {
        this->demuxer_.reset();
        this->decoder_.reset();
        this->scaler_.reset();
        
        this->packet_queue_.clear();
        this->yuv_frame_queue_.clear();
        this->rgb_frame_queue_.clear();
    }

    void start()
    {
        this->stop_flag_.store(false, std::memory_order_release);
        
        this->image_thread_ = std::thread([this](){image_task();});
        this->decoding_thread_ = std::thread([this](){decoding_task();});
        this->demuxing_thread_ = std::thread([this](){demuxing_task();});
    }
    void stop()
    {
        this->stop_flag_.store(true, std::memory_order_release);

        this->demuxing_thread_.join();
        this->decoding_thread_.join();
        this->image_thread_.join();
    }
    
    void register_callback(std::function<void()> f)
    {
        this->frame_ready_callback_ = f;    
    }

private:
    std::unique_ptr<Demuxer> demuxer_{};
    std::unique_ptr<Decoder> decoder_{};
    std::unique_ptr<Scaler> scaler_{};
    
    RingBufferSPSC<std::unique_ptr<Packet>> packet_queue_{3};
    RingBufferSPSC<std::unique_ptr<Frame>> yuv_frame_queue_{3};
    SharedQueue &rgb_frame_queue_;
    
    std::function<void()> frame_ready_callback_;
    
    std::mutex decoding_mutex_;
    std::mutex image_mutex_;
    std::condition_variable decoding_cond_;
    std::condition_variable image_cond_;
    
    std::thread demuxing_thread_;
    std::thread decoding_thread_;
    std::thread image_thread_;
    
    std::atomic<bool> stop_flag_{true};
    
    // 线程1: 解封装
    void demuxing_task()
    {
        while (1)
        {
            if (this->stop_flag_.load(std::memory_order_acquire))
            {
                this->decoding_cond_.notify_one();
                break;
            }

            auto ret = Packet::create();
            if (!ret.has_value())
                continue;
            auto packet = std::move(ret.value());

            if (this->demuxer_->output_one_demuxed_packet(*packet) != FF_ErrorCode::DemuxingOK)
                continue;

            if (packet->stream_index_ == this->decoder_->stream_index_)
            {
                // 自动覆盖旧 Packet
                this->packet_queue_.push(std::move(packet));
                this->decoding_cond_.notify_one();
            }
        }
    }
    // 线程2: 解码
    void decoding_task()
    {
        while (1)
        {
            std::unique_lock<std::mutex> ul(this->decoding_mutex_);
            this->decoding_cond_.wait(ul, [this](){
                if (!this->packet_queue_.is_empty())
                    return true;
                if (this->stop_flag_.load(std::memory_order_acquire))
                    return true;

                return false;
            });
            if (this->stop_flag_.load(std::memory_order_acquire))
            {
                this->image_cond_.notify_one();
                break;
            }

            auto packet = this->packet_queue_.pop();
            this->decoder_->inuput_one_encoded_packet(*packet);
            
            while (1)
            {
                auto ret1 = Frame::create();
                if (!ret1.has_value())
                    continue;
                auto frame = std::move(ret1.value());

                auto ret2 = this->decoder_->output_one_decoded_frame(*frame);
                if (ret2 != FF_ErrorCode::DecodingOK)
                    break;
                
                // 自动覆盖旧 YUV Frame
                this->yuv_frame_queue_.push(std::move(frame));
                this->image_cond_.notify_one();
            }
        }
    }
    // 线程3: 图像处理
    void image_task()
    {
        while (1)
        {
            std::unique_lock<std::mutex> ul(this->image_mutex_);
            this->image_cond_.wait(ul, [this](){
                if (!this->yuv_frame_queue_.is_empty())
                    return true;
                if (this->stop_flag_.load(std::memory_order_acquire))
                    return true;
                
                return false;
            });
            if (this->stop_flag_.load(std::memory_order_acquire))
                break;
            
            auto yuv_frame = this->yuv_frame_queue_.pop();
            
            auto ret = Frame::create();
            if (!ret.has_value())
                continue;
            auto rgb_frame = std::move(ret.value());
            if (!rgb_frame->alloc_buffer(this->scaler_->pix_fmt_, this->scaler_->width_, this->scaler_->height_))
                continue;
            
            if (this->scaler_->scale(*yuv_frame, *rgb_frame))
            {
                // 自动覆盖旧 RGB Frame
                this->rgb_frame_queue_.push(std::move(rgb_frame));
                this->frame_ready_callback_();
            }
        }
    }
};

#endif