#ifndef SENDCOINSENTRY_H
#define SENDCOINSENTRY_H

#include <QFrame>

namespace Ui
{
    class SendCoinsEntry;
}
class WalletModel;
class SendCoinsRecipient;
class SingleColumnAccountModel;
class SendCoinsTargetWidget;
class QLabel;
class accountLabel;
class accountAddress;
class accountBalance;
class BitcoinAmountField;
class QGridLayout;

class MyAccountEntry
{
public:
    MyAccountEntry(const QString& accountLabel, const QString& accountAddress, const QString& accountBalance);
    void addToLayout(QGridLayout* layout, int row);
    void update(const QString& accountLabel_, const QString& accountBalance_);
    QString getAccountAddress();
    void clear();
    bool validate();
    SendCoinsRecipient getValue();
    qint64 getAmount();

    void setDisplayUnit(int unit);
private:
    QLabel* accountLabel;
    QLabel* accountAddress;
    QLabel* accountBalance;
    BitcoinAmountField* amt;
};

/** A single entry in the dialog for sending bitcoins. */
class SendCoinsEntry : public QFrame
{
    Q_OBJECT

public:
    explicit SendCoinsEntry(QWidget *parent = 0);
    ~SendCoinsEntry();

    void setModel(WalletModel *model);
    bool validate();
    void setFocus();
    void updateRemoveEnabled();
    void pasteEntry(const SendCoinsRecipient &rv);
    bool handleURI(const QString &uri);

public slots:
    void deleteButtonPressed();
    void pasteButtonPressed();
    void addressBookButtonPressed();
    void addAnotherButtonPressed();
    void confirmButtonPressed();
    void myAccountNextButtonPressed();
    void payToAddressChanged(QString address);
    void updateMyAccountList();

signals:

private slots:
    void updateDisplayUnit();
    void scrollToLastRecipient();

    void coinControlFeatureChanged(bool);
    void coinControlButtonClicked();
    void coinControlChangeChecked(int);
    void coinControlChangeEdited(const QString &);
    void coinControlUpdateLabels();
    void coinControlClipboardQuantity();
    void coinControlClipboardAmount();
    void coinControlClipboardFee();
    void coinControlClipboardAfterFee();
    void coinControlClipboardBytes();
    void coinControlClipboardPriority();
    void coinControlClipboardLowOutput();
    void coinControlClipboardChange();

private:
    void clearTargetWidgets();
    QList<SendCoinsTargetWidget*> mapTargetWidgets;
    QList<MyAccountEntry*> mapMyAccounts;
    Ui::SendCoinsEntry *ui;
    WalletModel *model;
    SingleColumnAccountModel *accountListModel;
    bool newRecipientAllowed;
};

#endif // SENDCOINSENTRY_H
