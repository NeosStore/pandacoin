#ifndef MAINFRAME_H
#define MAINFRAME_H

#include <QFrame>

namespace Ui
{
    class MainFrame;
}

class MenuBar;
class TabBar;
class LockBar;

class MainFrame : public QFrame
{
    Q_OBJECT

public:
    explicit MainFrame(QWidget *parent = 0);
    ~MainFrame();

    void addTab(QWidget* widget,QString name);
    void setActiveTab(QWidget* widget);
    TabBar* getTabBar();
    MenuBar* getMenuBar();
    LockBar* getLockBar();

private:
    Ui::MainFrame *ui;
    MenuBar* m_menuBar;
};

#endif // MAINFRAME_H
