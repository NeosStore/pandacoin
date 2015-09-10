#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>
#include <QModelIndex>

namespace Ui
{
    class OverviewPage;
}
class WalletModel;
class QMenu;

/** Overview ("home") page widget */
class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = 0);
    ~OverviewPage();

    void setModel(WalletModel *model);
    void showOutOfSyncWarning(bool fShow);

public slots:
    void setBalance(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance);

signals:
    void accountClicked(const QString &accountName);
    void requestGotoTransactionPage();

private:
    Ui::OverviewPage *ui;
    WalletModel *model;
    qint64 currentBalance;
    qint64 currentStake;
    qint64 currentUnconfirmedBalance;
    qint64 currentImmatureBalance;
    QMenu *contextMenu;
    QModelIndex contextMenuTriggerIndex;

private slots:
    void updateDisplayUnit();
    void handleAccountClicked(const QModelIndex &index);
    void on_quick_transfer_next_button_clicked();
    void contextualMenu(const QPoint &point);
    void copyAddress();
    void copyLabel();
    void copyAmount();
};

#endif // OVERVIEWPAGE_H
