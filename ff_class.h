#ifndef FF_CLASS_H
#define FF_CLASS_H

#include <optional>
#include <cstdint>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}
#include <nlohmann/json.hpp>

enum class FF_ErrorCode : std::uint8_t
{  
    DemuxingOK,
    DemuxingFailed,
    DemuxingEOF,

    DecodingOK,
    DecodingFailed,
    DecodingEOF,

};

enum class FF_ColorFormat
{
    RGB24 = AV_PIX_FMT_RGB24,
    YUV420 = AV_PIX_FMT_YUV420P,
    UnKnown,
};

enum class FF_StreamType
{
    Video = AVMEDIA_TYPE_VIDEO,
    UnKnown,
};

enum class FF_CodecType
{
    H264 = AV_CODEC_ID_H264,
    H265 = AV_CODEC_ID_H265,
    UnKnown,
};

class Packet
{
public:
    int stream_index_{-1};

    static std::optional<std::unique_ptr<Packet>> create()
    {
        AVPacket *pkt{};

        pkt = av_packet_alloc();
        if (pkt)
            return std::unique_ptr<Packet>(new Packet(pkt));
        else
            return std::nullopt;
    }

    Packet() = delete;
    ~Packet()
    {
        if (this->pkt_)
        {
            // free 内部会先 unref
            av_packet_free(&this->pkt_);
        }
    }
    Packet(Packet &&other) noexcept
        : stream_index_(other.stream_index_)
        , pkt_{other.pkt_}
    {
        other.stream_index_ = -1;
        other.pkt_ = nullptr;
    }
    Packet& operator=(Packet &&other) noexcept
    {
        if (this != &other)
        {
            this->stream_index_ = other.stream_index_;
            other.stream_index_ = -1;

            if (this->pkt_)
            {
                // free 内部会先 unref
                av_packet_free(&this->pkt_);
            }
            this->pkt_ = other.pkt_;
            other.pkt_ = nullptr;
        }
        return *this;
    }
    Packet(const Packet &other) = delete;
    Packet& operator=(const Packet &other) = delete;

    AVPacket* get_raw() noexcept
    {
        return this->pkt_;
    }
    const AVPacket* get_raw() const noexcept
    {
        return this->pkt_;
    }

    bool is_valid()
    {
        return this->pkt_ != nullptr;
    }

private:
    explicit Packet(AVPacket *pkt) noexcept : pkt_{pkt} {};

    AVPacket *pkt_{};

};

class Frame
{
public:
    enum FF_ColorFormat pix_fmt_{FF_ColorFormat::UnKnown};
    double pts_{0};
    int width_{0}, height_{0};

    static std::optional<std::unique_ptr<Frame>> create()
    {
        AVFrame *frm{};

        frm = av_frame_alloc();
        if (frm)
            return std::unique_ptr<Frame>(new Frame(frm));
        else
            return std::nullopt;
    }

    Frame() = delete;
    ~Frame()
    {
        if (this->frm_)
        {
            // free 内部会先 unref
            av_frame_free(&this->frm_);
        }
    }
    Frame(Frame &&other) noexcept
        : pix_fmt_{other.pix_fmt_}
        , pts_{other.pts_}
        , width_{other.width_}
        , height_{other.height_}
        , frm_{other.frm_}
    {
        other.pix_fmt_ = FF_ColorFormat::UnKnown;
        other.pts_ = 0;
        other.width_ = 0;
        other.height_ = 0;
        other.frm_ = nullptr;
    }
    Frame& operator=(Frame &&other) noexcept
    {
        if (this != &other)
        {
            this->pix_fmt_ = other.pix_fmt_;
            other.pix_fmt_ = FF_ColorFormat::UnKnown;
            
            this->pts_ = other.pts_;
            other.pts_ = 0;
            
            this->width_ = other.width_;
            other.width_ = 0;
            
            this->height_ = other.height_;
            other.height_ = 0;
            
            if (this->frm_)
                // free 内部会先 unref
                av_frame_free(&this->frm_);
            this->frm_ = other.frm_;
            other.frm_ = nullptr;
        }
        return *this;
    }
    Frame(const Frame &) = delete;
    Frame& operator=(const Frame &) = delete;

    AVFrame* get_raw() noexcept
    {
        return this->frm_;
    }
    const AVFrame* get_raw() const noexcept
    {
        return this->frm_;
    }
    
    bool is_valid()
    {
        return this->frm_ != nullptr;
    }

    void free_buffer() noexcept
    {
        if (this->frm_)
            av_frame_unref(this->frm_);
    }
    bool alloc_buffer(const FF_ColorFormat pix_fmt, const int width, const int height) noexcept
    {
        this->frm_->format = static_cast<int>(pix_fmt);
        this->frm_->height = height;
        this->frm_->width = width;

        this->width_ = width;
        this->height_ = height;
        this->pix_fmt_ = pix_fmt;

        return av_frame_get_buffer(this->frm_, 0) == 0;
    }
    
private:
    explicit Frame(AVFrame *frm) noexcept : frm_{frm} {};

    AVFrame *frm_{};
};

class Demuxer
{
public:   
    static std::optional<std::unique_ptr<Demuxer>> create(const char *url, const char *opt = nullptr)
    {
        AVFormatContext *fmt_ctx{};
        AVDictionary *dict{};

        if (opt)
            dict = Demuxer::get_opt(opt);
        
        auto ret1 = avformat_open_input(&fmt_ctx, url, nullptr, &dict);
        if (ret1 == 0)
        {
            auto ret2 = avformat_find_stream_info(fmt_ctx, nullptr);
            if (ret2 < 0)
            {
                avformat_close_input(&fmt_ctx);
                return std::nullopt;
            }
            
            return std::unique_ptr<Demuxer>(new Demuxer(fmt_ctx));
        }
        else
            return std::nullopt;
    }

    Demuxer() = delete;
    ~Demuxer()
    {
        if (this->fmt_ctx_)
            avformat_close_input(&this->fmt_ctx_);
    }
    Demuxer(Demuxer &&other) = delete;
    Demuxer& operator=(Demuxer &&other) = delete;
    Demuxer(const Demuxer &) = delete;
    Demuxer& operator=(const Demuxer &) = delete;

    AVFormatContext* get_raw() noexcept
    {
        return this->fmt_ctx_;
    }
    const AVFormatContext* get_raw() const noexcept
    {
        return this->fmt_ctx_;
    }
    
    bool is_valid()
    {
        return this->fmt_ctx_ != nullptr;
    }

    FF_ErrorCode output_one_demuxed_packet(Packet &packet) noexcept
    {
        if (!packet.is_valid())
            return FF_ErrorCode::DemuxingFailed;
        
        auto ret = av_read_frame(this->fmt_ctx_, packet.get_raw());
        switch (ret)
        {
            case 0:
                packet.stream_index_ = packet.get_raw()->stream_index;
                return FF_ErrorCode::DemuxingOK;
                break;
            case AVERROR_EOF:
                return FF_ErrorCode::DemuxingEOF;
                break;
            default:
                return FF_ErrorCode::DemuxingFailed;
        }
    }

private:
    explicit Demuxer(AVFormatContext *fmt_ctx) noexcept : fmt_ctx_{fmt_ctx} {};
    
    using json = nlohmann::json;
    static AVDictionary* get_opt(const char *opt) noexcept
    {
        AVDictionary *dict{};
        json j = json::parse(opt);

        for (auto e = j.begin(); e != j.end(); ++e)
        {
            if (av_dict_set(&dict, e.key().c_str(), \
                e.value().get<std::string>().c_str(), 0) < 0)
            {
                av_dict_free(&dict);
                return nullptr;
            }
        }
        return dict;
    }

    AVFormatContext *fmt_ctx_{};
};

class Decoder
{
public:
    int stream_index_{-1};
    double time_base_{0};
    double duration_{0};
    double avg_frame_rate_{0};
    FF_StreamType codec_type_{FF_StreamType::UnKnown};
    FF_CodecType codec_id_{FF_CodecType::UnKnown};
    std::int64_t bit_rate_{0};
    int width_{0}, height_{0};
    FF_ColorFormat pix_fmt_{FF_ColorFormat::UnKnown};
    
    static std::optional<std::unique_ptr<Decoder>> create(Demuxer &demuxer, const FF_StreamType type)
    {
        AVCodecContext *codec_ctx{};
        const AVCodec *codec{};
        
        auto idx = av_find_best_stream(demuxer.get_raw(), static_cast<enum AVMediaType>(type), -1, -1, &codec, 0);
        if (idx < 0 || codec == nullptr)
            return std::nullopt;

        codec_ctx = avcodec_alloc_context3(codec);
        if (codec_ctx == nullptr)
            return std::nullopt;
        
        AVStream *stream = demuxer.get_raw()->streams[idx];
        if (avcodec_parameters_to_context(codec_ctx, stream->codecpar) < 0)
        {
            avcodec_free_context(&codec_ctx);
            return std::nullopt;
        }
        if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
        {
            avcodec_free_context(&codec_ctx);
            return std::nullopt;
        }

        return std::unique_ptr<Decoder>(new Decoder(codec_ctx, idx, stream));
    }

    Decoder() = delete;
    ~Decoder() noexcept
    {
        if (this->codec_ctx_)
        {
            avcodec_free_context(&this->codec_ctx_);
        }
    }
    Decoder(Decoder &&other) = delete;
    Decoder& operator=(Decoder &&other) = delete;
    Decoder(const Decoder &) = delete;
    Decoder& operator=(const Decoder &) = delete;

    AVCodecContext* get_raw() noexcept
    {
        return this->codec_ctx_;
    }
    const AVCodecContext* get_raw() const noexcept
    {
        return this->codec_ctx_;
    }
    
    bool is_valid() const noexcept
    {
        return this->codec_ctx_ != nullptr;
    }

    FF_ErrorCode inuput_one_encoded_packet(Packet &packet) noexcept
    {
        if (!packet.is_valid() || packet.stream_index_ != this->stream_index_)
            return FF_ErrorCode::DecodingFailed;
        
        if (avcodec_send_packet(this->codec_ctx_, packet.get_raw()) < 0)
            return FF_ErrorCode::DecodingFailed;
        return FF_ErrorCode::DecodingOK;
    }
    FF_ErrorCode output_one_decoded_frame(Frame &frame) noexcept
    {
        if (!frame.is_valid())
            return FF_ErrorCode::DecodingFailed;
        
        int ret = avcodec_receive_frame(this->codec_ctx_, frame.get_raw());
        switch (ret)
        {
            case 0:
                frame.pix_fmt_ = this->pix_fmt_;
                frame.pts_ = this->time_base_ * frame.get_raw()->pts;
                frame.width_ = this->width_;
                frame.height_ = this->height_;
                return FF_ErrorCode::DecodingOK;
                break;
            case AVERROR(EAGAIN):
            case AVERROR_EOF:
                return FF_ErrorCode::DecodingEOF;
                break;
            default:
                return FF_ErrorCode::DecodingFailed;
        }
    }

private:
    explicit Decoder(AVCodecContext *codec_ctx, int idx, AVStream *stream) noexcept
    {
        this->stream_index_ = idx;
        this->time_base_ = av_q2d(stream->time_base);
        this->duration_ = stream->duration * this->time_base_;
        this->avg_frame_rate_ = av_q2d(stream->avg_frame_rate);
        this->codec_type_ = static_cast<FF_StreamType>(codec_ctx->codec_type);
        this->codec_id_ = static_cast<FF_CodecType>(codec_ctx->codec_id);
        this->bit_rate_ = codec_ctx->bit_rate;
        this->width_ = codec_ctx->width;
        this->height_ = codec_ctx->height;
        this->pix_fmt_ = static_cast<FF_ColorFormat>(codec_ctx->pix_fmt);
        this->codec_ctx_ = codec_ctx;
    }

    AVCodecContext *codec_ctx_{};
};

class Scaler
{
public:
    int width_{0}, height_{0};
    FF_ColorFormat pix_fmt_{FF_ColorFormat::UnKnown};
    
    static std::optional<std::unique_ptr<Scaler>>
    create(int src_width, int src_height, enum FF_ColorFormat src_fmt,
           int dst_width, int dst_height, enum FF_ColorFormat dst_fmt)
    {
        SwsContext *sws_ctx{};

        sws_ctx = sws_getContext(src_width, src_height, static_cast<enum AVPixelFormat>(src_fmt), \
                                 dst_width, dst_height, static_cast<enum AVPixelFormat>(dst_fmt), \
                                 SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (sws_ctx)
            return std::unique_ptr<Scaler>(new Scaler(sws_ctx, dst_width, dst_height, dst_fmt));
        else
            return std::nullopt;
    }

    Scaler() = delete;
    ~Scaler() noexcept
    {
        if (this->sws_ctx_)
            sws_free_context(&this->sws_ctx_);
    }
    Scaler(Scaler &&other) = delete;
    Scaler& operator=(Scaler &&other) = delete;
    Scaler(const Scaler &) = delete;
    Scaler& operator=(const Scaler &) = delete;

    bool scale(Frame &src, Frame &dst)
    {
        if (!src.is_valid() || !dst.is_valid())
            return false;
        
        auto src_p = src.get_raw();
        auto dst_p = dst.get_raw();
        
        sws_scale(this->sws_ctx_, src_p->data, src_p->linesize, 0, \
            src_p->height, dst_p->data, dst_p->linesize);

        dst.pts_ = src.pts_;
        
        return true;
    }

private:
    explicit Scaler(SwsContext *sws_ctx, int width, int height, FF_ColorFormat fmt) noexcept
        : width_{width}
        , height_{height}
        , pix_fmt_{fmt}
        , sws_ctx_{sws_ctx} {}

    SwsContext *sws_ctx_{};
};

#endif // FF_CLASS_H
