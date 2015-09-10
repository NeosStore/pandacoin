#ifndef TABBEDDATEWIDGET_H
#define TABBEDDATEWIDGET_H

#include <QWidget>
#include <QDate>
#include <map>

class WalletModel;
class QPushButton;
class TransactionFilterProxy;

namespace Ui
{
    class TabbedDateWidget;
}

class TabbedDateWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TabbedDateWidget(QWidget *parent = 0);
    ~TabbedDateWidget();
    void setModel(TransactionFilterProxy *model);
    void clearDateSelections();

signals:
    void tabPressed();
    void tabReleased();


private slots:
    void on_push_button_next_pressed();
    void on_push_button_prev_pressed();
    void scrollContentsUpdated();
    void date_button_pressed();

private:
    Ui::TabbedDateWidget *ui;
    TransactionFilterProxy* model;
    std::map<QPushButton*,QDate> dateButtonDateMapping;
};

#endif // TABBEDDATEWIDGET_H
