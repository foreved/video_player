#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    this->setMinimumSize(800, 600);
    this->widget_main = new QTabWidget(this);
    this->setCentralWidget(this->widget_main);
    
    this->page_play = new PagePlay;
    this->widget_main->addTab(this->page_play, "播放界面");
    
    this->page_log = new PageLog;
    this->widget_main->addTab(this->page_log, "日志界面");
    
    connect(this->page_play, &PagePlay::send_log, this->page_log, &PageLog::receive_log);
}

MainWindow::~MainWindow() = default;

