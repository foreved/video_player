#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include "page_play.h"
#include "page_log.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    
private:
    QTabWidget *widget_main{};
    PagePlay *page_play{};
    PageLog *page_log{};
};
#endif // MAINWINDOW_H
