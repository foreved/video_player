#ifndef PLAYER_H
#define PLAYER_H

#include "parser.h"
#include <QVBoxLayout>
#include <QString>
#include <QWidget>
#include <QPainter>
#include <QThread>

class Player : public QWidget
{
    Q_OBJECT
    
public:
    std::atomic<int> fps_cnt_{0};
    
    explicit Player(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        this->parser_.register_callback([this](){
            if (!this->stop_flag_)
                update();
        });
    }
    ~Player() = default;

    bool parse(const char *url, const char *opt)
    {
        if (this->parser_.open(url, opt))
        {
            this->parser_.start();
            return true;
        }
        return false;
    }
    void start()
    {
        this->stop_flag_ = false;
    }
    void pause()
    {
        this->stop_flag_ = true;
    }
    void stop()
    {
        this->stop_flag_ = true;
        this->first_flag_ = false;
        this->current_frame_.reset();
        this->parser_.stop();
        this->parser_.close();
    }

private:
    Parser::SharedQueue frame_queue_{3};
    Parser parser_{frame_queue_};
    
    bool stop_flag_{true};
    
    Parser::SharedItem current_frame_{};
    bool first_flag_{false};
    double first_pts_{0};
    std::chrono::steady_clock::time_point first_clock_;
    inline constexpr static double t1{0.03}, t2{-0.1};
    
    void paintEvent(QPaintEvent *e)
    {
        Q_UNUSED(e);
        
        if (this->stop_flag_)
        {
            if (this->current_frame_)
            {
                QPainter painter(this);
                QImage img(this->current_frame_->get_raw()->data[0], this->current_frame_->width_, this->current_frame_->height_, \
                           this->current_frame_->get_raw()->linesize[0], QImage::Format_RGB888);
                painter.drawImage(this->rect(), img);
            }
            else
            {
                QPainter painter(this);
                painter.fillRect(this->rect(), Qt::black);
            }
            return;
        }
        
        while (!this->frame_queue_.is_empty())
        {
            this->current_frame_ = this->frame_queue_.pop();
            
            if (!this->first_flag_)
            {
                this->first_pts_ = this->current_frame_->pts_;
                this->first_clock_ = std::chrono::steady_clock::now();
                this->first_flag_ = true;
            }
            
            auto pts = this->current_frame_->pts_ - this->first_pts_;
            auto clock = std::chrono::duration<double>(std::chrono::steady_clock::now() - this->first_clock_).count();
            auto diff = pts - clock;
            if (diff > Player::t1)
                QThread::sleep(diff - Player::t1);
            else if (diff < Player::t2)
                continue;
            
            QPainter painter(this);
            QImage img(this->current_frame_->get_raw()->data[0], this->current_frame_->width_, this->current_frame_->height_, \
                       this->current_frame_->get_raw()->linesize[0], QImage::Format_RGB888);
            painter.drawImage(this->rect(), img);
            
            this->fps_cnt_.fetch_add(1, std::memory_order_release);
            
            break;
        }
    }
};

#endif // PLAYER_H
