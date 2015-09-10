#ifndef ADDRESSBOOKPAGE_NEW_H
#define ADDRESSBOOKPAGE_NEW_H

#include <QFrame>

namespace Ui
{
    class AddressBookPage_new;
}
class WalletModel;
class AddressFilterModel;

class AddressBookPage_new : public QFrame
{
    Q_OBJECT

public:
    explicit AddressBookPage_new(QWidget *parent = 0);
    ~AddressBookPage_new();
    void setModel(WalletModel* model);

private slots:
    void onSearch();
    void onSelectionChanged();
    void onAddressBookEdit();
    void onAddressBookSendCoins();
    void onAddressBookChangeDone();
    void addressBookUpdated();
    void onAddressBookDeletePressed();
    void onAddressBookCopyToClipboard();
    void onAddressBookShowQRCode();
    void onAddressBookVerifyMessage();
    void onAddressBookNewAddress();
    void updateDisplayUnit();

signals:
    void onVerifyMessage(QString);

private:
    Ui::AddressBookPage_new *ui;
    WalletModel* model;
    AddressFilterModel* filterModel;
};

#endif // ADDRESSBOOKPAGE_NEW_H
