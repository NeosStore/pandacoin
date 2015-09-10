#ifndef TRANSACTIONFILTERWIDGET_H
#define TRANSACTIONFILTERWIDGET_H

#include <QFrame>
#include "walletmodel.h"

class TransactionFilterProxy;
namespace Ui
{
class TransactionFilterWidget;
}

class TransactionFilterWidget : public QFrame
{
    Q_OBJECT

public:
    explicit TransactionFilterWidget(QWidget *parent = 0);
    ~TransactionFilterWidget();
    void setModel(TransactionFilterProxy *model);

private:
    Ui::TransactionFilterWidget *ui;
    TransactionFilterProxy *model;

signals:
    void exportClicked();

private slots:
    void dateTabPressed();
    void dateTabReleased();
    void recentTransactionButtonPressed();
    void on_recent_transaction_searchbox_returnPressed();
    void on_transaction_search_button_pressed();
};

#endif // TRANSACTIONFILTERWIDGET_H
