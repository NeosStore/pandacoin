#ifndef SENDCOINSTARGETWIDGET_H
#define SENDCOINSTARGETWIDGET_H

#include <QFrame>
#include "walletmodel.h"

namespace Ui
{
    class SendCoinsTargetWidget;
}
class WalletModel;

class SendCoinsTargetWidget : public QFrame
{
    Q_OBJECT

public:
    explicit SendCoinsTargetWidget(QWidget *parent = 0);
    ~SendCoinsTargetWidget();
    void setRemoveEnabled(bool enabled);
    void setRecipient(const QString& recipient);
    void setRecipientName(const QString& label);
    bool validate(WalletModel *model);
    bool isClear();
    void setFocus();
    void setDisplayUnit(int unit);
    SendCoinsRecipient getValue();
    void setValue(const SendCoinsRecipient &value);

signals:
    void deleteButtonPressed();
    void pasteButtonPressed();
    void addressBookButtonPressed();
    void addAnotherButtonPressed();
    void confirmButtonPressed();
    void payToAddressChanged(QString address);

private:
    Ui::SendCoinsTargetWidget *ui;
    int currentDisplayUnit;
};

#endif // SENDCOINSTARGETWIDGET_H
