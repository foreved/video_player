#ifndef PAGE_PLAY_H
#define PAGE_PLAY_H

#include <format>
#include <string>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QComboBox>
#include <QStackedWidget>
#include <QFormLayout>
#include <QRadioButton>
#include <QCheckBox>
#include <QSpinBox>
#include <nlohmann/json.hpp>
#include <QThread>
#include <QTimer>
#include "player.h"

class BaseConfig : public QWidget
{
    Q_OBJECT
    
public:
    explicit BaseConfig(QWidget *parent = nullptr) : QWidget(parent)
    {
        this->layout = new QFormLayout;
        this->setLayout(this->layout);
    }
    
    using json = nlohmann::json;
    virtual json get_json() = 0;
    
    QFormLayout *layout{};
};

class FileConfig : public BaseConfig
{
    Q_OBJECT
    
public:
    explicit FileConfig(QWidget *parent = nullptr) : BaseConfig(parent)
    {        
        this->cfg_seekable = new QComboBox;
        this->layout->addRow("seekable", this->cfg_seekable);
        this->cfg_seekable->addItem("默认");
        this->cfg_seekable->addItem("0");
        this->cfg_seekable->addItem("1");
        
        this->cfg_probesize = new QSpinBox;
        this->layout->addRow("probesize (byte)", this->cfg_probesize);
        this->cfg_probesize->setValue(0);
    }
    
    json get_json() override
    {
        json j;
        
        if (this->cfg_seekable->currentText() != "默认")
            j["seekable"] = this->cfg_seekable->currentText().toStdString();
        if (this->cfg_probesize->value() != 0)
            j["probesize"] = std::to_string(this->cfg_probesize->value());
        
        return j;
    }
    
// private:
    QComboBox *cfg_seekable{};
    QSpinBox *cfg_probesize{};
};

class NetBaseConfig : public BaseConfig
{
    Q_OBJECT
    
public: 
    explicit NetBaseConfig(QWidget *parent = nullptr) : BaseConfig(parent)
    {
        this->cfg_timeout = new QSpinBox;
        this->layout->addRow("timeout (us)", this->cfg_timeout);
        this->cfg_timeout->setValue(0);
        
        this->cfg_stimeout = new QSpinBox;
        this->layout->addRow("stimeout (us)", this->cfg_stimeout);
        this->cfg_stimeout->setValue(0);
        
        this->cfg_rw_timeout = new QSpinBox;
        this->layout->addRow("rw_timeout (us)", this->cfg_rw_timeout);
        this->cfg_rw_timeout->setValue(0);
    }
    
    json get_json() override
    {
        json j;
        
        if (this->cfg_timeout->value() != 0)
            j["timeout"] = std::to_string(this->cfg_timeout->value());
        if (this->cfg_stimeout->value() != 0)
            j["stimeout"] = std::to_string(this->cfg_stimeout->value());
        if (this->cfg_rw_timeout->value() != 0)
            j["rw_timeout"] = std::to_string(this->cfg_rw_timeout->value());
        
        append_json(j);
        
        return j;
    }
    
    virtual void append_json(json &j) = 0;
// private:
    QSpinBox *cfg_timeout{}, *cfg_stimeout{}, *cfg_rw_timeout{};
};

class RTSPConfig : public NetBaseConfig
{
    Q_OBJECT
    
public:
    explicit RTSPConfig(QWidget *parent = nullptr) : NetBaseConfig(parent)
    {
        this->cfg_rtsp_transport = new QComboBox;
        this->layout->addRow("rtsp_transport", this->cfg_rtsp_transport);
        this->cfg_rtsp_transport->addItem("默认");
        this->cfg_rtsp_transport->addItem("tcp");
        this->cfg_rtsp_transport->addItem("udp");
        this->cfg_rtsp_transport->addItem("http");
        
        this->cfg_max_delay = new QSpinBox;
        this->layout->addRow("max_delay (us)", this->cfg_max_delay);
        this->cfg_max_delay->setValue(0);
        
        this->cfg_buffer_size = new QSpinBox;
        this->layout->addRow("buffer_size (byte)", this->cfg_buffer_size);
        this->cfg_buffer_size->setValue(0);
    }
    
    void append_json(json &j) override
    {
        if (this->cfg_rtsp_transport->currentText() != "默认")
            j["rtsp_transport"] = this->cfg_rtsp_transport->currentText().toStdString();
        if (this->cfg_max_delay->value() != 0)
            j["max_delay"] = std::to_string(this->cfg_max_delay->value());
        if (this->cfg_buffer_size->value() != 0)
            j["buffer_size"] = std::to_string(this->cfg_buffer_size->value());
    }
// private:
    QComboBox *cfg_rtsp_transport{};
    QSpinBox *cfg_max_delay{}, *cfg_buffer_size{};
};

class HTTPConfig : public NetBaseConfig
{
    Q_OBJECT
    
public:
    explicit HTTPConfig(QWidget *parent = nullptr) : NetBaseConfig(parent)
    {
        this->cfg_user_agent = new QLineEdit;
        this->layout->addRow("user_agent", this->cfg_user_agent);
        
        this->cfg_headers = new QLineEdit;
        this->layout->addRow("headers", this->cfg_headers);
        
        this->cfg_http_persistent = new QComboBox;
        this->layout->addRow("http_persistent", this->cfg_http_persistent);
        this->cfg_http_persistent->addItem("默认");
        this->cfg_http_persistent->addItem("0");
        this->cfg_http_persistent->addItem("1");
    }
    
    void append_json(json &j) override
    {
        if (!this->cfg_user_agent->text().isEmpty())
            j["user_agent"] = this->cfg_user_agent->text().toStdString();
        if (!this->cfg_headers->text().isEmpty())
            j["headers"] = this->cfg_headers->text().toStdString();
        if (this->cfg_http_persistent->currentText() != "默认")
            j["http_persistent"] = this->cfg_http_persistent->currentText().toStdString();
    }
    
// private:
    QLineEdit *cfg_user_agent{}, *cfg_headers{};
    QComboBox *cfg_http_persistent{};
};

class PageURL : public QWidget
{
    Q_OBJECT
    
public:   
    explicit PageURL(QWidget *parent = nullptr) : QWidget(parent)
    {
        this->vlayout = new QVBoxLayout;
        this->setLayout(this->vlayout);
        
        this->url_widget = new QWidget;
        this->vlayout->addWidget(this->url_widget);
        this->hlayout = new QHBoxLayout;
        this->url_widget->setLayout(this->hlayout);
        
        this->url_type = new QComboBox;
        this->hlayout->addWidget(this->url_type);
        
        this->url_input = new QLineEdit;
        this->hlayout->addWidget(this->url_input);
        
        this->url_parse = new QPushButton("开始解析");
        this->hlayout->addWidget(this->url_parse);
        
        this->url_config = new QStackedWidget;
        this->vlayout->addWidget(this->url_config);
        this->cfg_file = new FileConfig;
        this->cfg_rtsp = new RTSPConfig;
        this->cfg_http = new HTTPConfig;
        
        this->url_type->addItem("本地文件");
        this->url_config->addWidget(this->cfg_file);
        this->url_type->addItem("RTSP");
        this->url_config->addWidget(this->cfg_rtsp);
        this->url_type->addItem("HTTP");
        this->url_config->addWidget(this->cfg_http);
        
        connect(this->url_type, &QComboBox::currentIndexChanged, this->url_config, &QStackedWidget::setCurrentIndex);
    }
    
// private:
    QHBoxLayout *hlayout{};
    QVBoxLayout *vlayout{};
    QWidget *url_widget{};
    QComboBox *url_type{};
    QLineEdit *url_input{};
    QPushButton *url_parse{};
    QStackedWidget *url_config{};
    FileConfig *cfg_file{};
    RTSPConfig *cfg_rtsp{};
    HTTPConfig *cfg_http{};
};

class Analyzer : public QObject
{
    Q_OBJECT
    
signals:
    void send_log(const QString &s);
    
public:
    Analyzer(Player &p, QObject *parent = nullptr)
        : QObject(parent)
        , player_{p}
    {
        this->timer_ = new QTimer(this);
        connect(this->timer_, &QTimer::timeout, this, [this](){
            auto cnt = this->player_.fps_cnt_.load(std::memory_order_acquire);
            send_log(QString::fromStdString(std::format("帧率: {:.2f}", double(cnt) / 5)));
            this->player_.fps_cnt_.store(0, std::memory_order_release);
        });
        this->timer_->start(5000);
    }
    ~Analyzer() = default;
    
private:
    QTimer *timer_{};
    Player &player_;
};

class PageDisplayer : public QWidget
{
    Q_OBJECT
    
signals:
    void send_log(const QString &s);
    
public:
    PageDisplayer(QWidget *parent = nullptr) : QWidget(parent)
    {
        this->vlayout_ = new QVBoxLayout;
        this->setLayout(this->vlayout_);
        
        this->player_ = new Player;
        this->vlayout_->addWidget(this->player_, 4);

        this->control_widget_ = new QWidget;
        this->vlayout_->addWidget(this->control_widget_, 1);
        this->hlayout_ = new QHBoxLayout;
        this->control_widget_->setLayout(this->hlayout_);
        
        this->start_button_ = new QPushButton("开始");
        this->hlayout_->addWidget(this->start_button_);
        this->pause_button_ = new QPushButton("暂停");
        this->hlayout_->addWidget(this->pause_button_);
        this->stop_button_ = new QPushButton("停止");
        this->hlayout_->addWidget(this->stop_button_);
        
        connect(this->start_button_, &QPushButton::clicked, this->player_, [this](){
            send_log("开始播放");
            this->player_->start();
        });
        connect(this->pause_button_, &QPushButton::clicked, this->player_, [this](){
            send_log("暂停播放");
            this->player_->pause();
        });
        connect(this->stop_button_, &QPushButton::clicked, this->player_, [this](){
            send_log("停止播放");
            this->player_->stop();
        });
        
        this->ana_ = new Analyzer(*this->player_);
        this->t_ = new QThread(this);
        this->ana_->moveToThread(this->t_);
        connect(this->ana_, &Analyzer::send_log, this, &PageDisplayer::send_log);
        this->t_->start();
    }
    
// private:
    QHBoxLayout *hlayout_{};
    QVBoxLayout *vlayout_{};
    QWidget *control_widget_{};
    Player *player_{};
    QPushButton *start_button_{}, *pause_button_{}, *stop_button_{};
    
    Analyzer *ana_{};
    QThread *t_{};
};

class PagePlay : public QWidget
{
    Q_OBJECT
    
signals:
    void send_log(const QString &s);
    
public:
    explicit PagePlay(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        this->page_url_ = new PageURL;
        this->page_displayer_ = new PageDisplayer;
        
        this->vlayout_ = new QVBoxLayout;
        this->setLayout(this->vlayout_);
        this->stacked_widget_ = new QStackedWidget;
        this->vlayout_->addWidget(this->stacked_widget_);

        this->stacked_widget_->addWidget(this->page_url_);
        this->stacked_widget_->addWidget(this->page_displayer_);
        
        connect(this->page_displayer_, &PageDisplayer::send_log, this, &PagePlay::send_log);

        connect(this->page_url_->url_parse, &QPushButton::clicked, this, [this](){
            this->stacked_widget_->setCurrentWidget(this->page_displayer_);
    
            auto url = this->page_url_->url_input->text().toStdString();
            auto w = this->page_url_->url_config->currentWidget();
            auto opt = static_cast<BaseConfig*>(w)->get_json();
    
            send_log(QString::fromStdString(std::format("开始解析[{}]", url)));
            send_log(QString::fromStdString(std::format("生效参数有:\n{}", opt.dump(4))));
            
            if (this->page_displayer_->player_->parse(url.c_str(), opt.dump().c_str()))
                send_log("解析成功");
            else
                send_log("解析失败");
        });
        connect(this->page_displayer_->stop_button_, &QPushButton::clicked, this, [this](){
            this->stacked_widget_->setCurrentWidget(this->page_url_);
        });
    }

private:
    QVBoxLayout *vlayout_{};
    QStackedWidget *stacked_widget_{};
    
    PageURL *page_url_{};
    PageDisplayer *page_displayer_{};
};

#endif // PAGE_PLAY_H
