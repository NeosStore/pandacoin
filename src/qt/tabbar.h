#ifndef PANDA_TAB_BAR_H
#define PANDA_TAB_BAR_H

#include <QTabWidget>
#include "main.h" // ClientMode

/** A skinned tab bar to be shown at top of the panda dialogs for navigation. */
class TabBar : public QTabWidget
{
    Q_OBJECT

public:
    explicit TabBar(QWidget *parent = 0);
    ~TabBar();

signals:

public slots:
    void clientModeChanged(ClientMode);

private slots:

private:
    void paintEvent(QPaintEvent* event);
};

#endif
