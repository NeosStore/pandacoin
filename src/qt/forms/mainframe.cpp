#include "mainframe.h"
#include "ui_mainframe.h"
#include "menubar.h"

MainFrame::MainFrame(QWidget *parent)
: QFrame(parent)
, ui(new Ui::MainFrame)
{
    ui->setupUi(this);

    m_menuBar=new MenuBar(this);
    ui->main_tab_nav_bar->setCornerWidget(m_menuBar,Qt::BottomRightCorner);
}

MainFrame::~MainFrame()
{
    delete ui;
}

void MainFrame::addTab(QWidget* widget,QString name)
{
    ui->main_tab_nav_bar->addTab(widget,name);
}

void MainFrame::setActiveTab(QWidget* widget)
{
    ui->main_tab_nav_bar->setCurrentWidget(widget);
}

TabBar* MainFrame::getTabBar()
{
    return ui->main_tab_nav_bar;
}

MenuBar* MainFrame::getMenuBar()
{
    return m_menuBar;
}

LockBar* MainFrame::getLockBar()
{
    return ui->lockBar;
}
