#ifndef TRANSACTIONVIEW_H
#define TRANSACTIONVIEW_H

#include <QWidget>
#include "transactiontablemodel.h"

class WalletModel;
class TransactionFilterProxy;

QT_BEGIN_NAMESPACE
class QTableView;
class QComboBox;
class QLineEdit;
class QMenu;
class QFrame;
QT_END_NAMESPACE

/** Widget showing the transaction list for a wallet, including a filter row.
    Using the filter row, the user can view or export a subset of the transactions.
  */
class TransactionView : public QWidget
{
    Q_OBJECT
public:
    explicit TransactionView(QWidget *parent = 0);
    void setModel(WalletModel *model);

    int getBalanceColumnWidth();
    int getAmountColumnWidth();
    int getAccountColumnWidth();
private:
    WalletModel *model;
    TransactionFilterProxy *transactionProxyModel;
    QTableView *transactionView;
    QMenu *contextMenu;
    QModelIndex contextMenuTriggerIndex;

private slots:
    void contextualMenu(const QPoint &);
    void showDetails();
    void copyAddress();
    void editLabel();
    void copyLabel();
    void copyAmount();
    void copyTxID();
    void viewOnBlockExplorer();

signals:
    void doubleClicked(const QModelIndex&);

public slots:
    void exportClicked();
    void focusTransaction(const QModelIndex&);

    friend class AccountPage;
};

#endif // TRANSACTIONVIEW_H
