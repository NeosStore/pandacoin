#ifndef ACCOUNTPAGE_H
#define ACCOUNTPAGE_H

#include <QFrame>
#include <QTableView>
#include <map>

class WalletModel;
class TransactionFilterProxy;
class CWallet;
class SingleColumnAccountModel;
class QLabel;

namespace Ui
{
    class AccountPage;
}

class AccountPage : public QFrame
{
    Q_OBJECT

public:
    explicit AccountPage(CWallet *wallet_ ,QWidget *parent = 0);
    ~AccountPage();
    void setModel(WalletModel *model);
    void setActiveAccount(const QString& accountName);
    void setActivePane(int paneIndex);

public slots:
    void update();

private slots:
    void on_TransactionTabAccountSelection_currentIndexChanged(int index);
    void on_sign_message_button_pressed();
    void on_show_qrcode_button_pressed();
    void on_copy_address_button_pressed();
    void transaction_table_cellClicked(const QModelIndex &);
    void cancelAccountCreation();

signals:
    void signMessage(QString);

private:
    void setSelectedAccountFromName(const QString &accountName);
    void resizeEvent(QResizeEvent *);
    void showEvent(QShowEvent *);
    void updateHeaderWidths();
    Ui::AccountPage *ui;
    WalletModel *model;
    CWallet *wallet;
    SingleColumnAccountModel* accountListModel;
    TransactionFilterProxy* proxyModel;
    std::map<QLabel*,QLabel*> mapInterestLabels;
};

#endif // ACCOUNTPAGE_H
