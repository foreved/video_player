#ifndef PAGE_LOG_H
#define PAGE_LOG_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class PageLog : public QWidget
{
    Q_OBJECT
    
public:
    explicit PageLog(QWidget *parent = nullptr) : QWidget(parent)
    {
        this->vlayout = new QVBoxLayout;
        this->setLayout(this->vlayout);
        
        this->logger = new QPlainTextEdit;
        this->vlayout->addWidget(this->logger);
        this->logger->setReadOnly(true);
        
        this->button_clear = new QPushButton("清空日志");
        this->vlayout->addWidget(this->button_clear);
        
        connect(this->button_clear, &QPushButton::clicked, this,
                [this]()
                {
                    this->logger->clear();
                });
        
    }
    
    void receive_log(const QString &s)
    {
        this->logger->appendPlainText(s);    
    }
    
private:
    QVBoxLayout *vlayout{};
    QPlainTextEdit *logger{};
    QPushButton *button_clear{};
};


#endif // PAGE_LOG_H
