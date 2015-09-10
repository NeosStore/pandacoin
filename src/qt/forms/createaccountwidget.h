#ifndef CREATEACCOUNTWIDGET_H
#define CREATEACCOUNTWIDGET_H

#include <QFrame>

namespace Ui
{
    class CreateAccountWidget;
}
class WalletModel;

class CreateAccountWidget : public QFrame
{
    Q_OBJECT

public:
    explicit CreateAccountWidget(QWidget *parent = 0);
    ~CreateAccountWidget();
    void setModel(WalletModel *model);

private slots:
    void accountLabelChanged(const QString& newAccountLabel);
    void createAccount();

signals:
    void cancelAccountCreation();

private:
    Ui::CreateAccountWidget *ui;
    WalletModel *model;
};

#endif // CREATEACCOUNTWIDGET_H
