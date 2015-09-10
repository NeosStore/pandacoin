#ifndef LOCKBAR_H
#define LOCKBAR_H

#include <QFrame>

namespace Ui
{
    class LockBar;
}
class WalletModel;

class LockBar : public QFrame
{
    Q_OBJECT

public:
    explicit LockBar(QWidget *parent = 0);
    ~LockBar();
    void setModel(WalletModel *model);

private slots:
    void encryptionStatusChanged(int newStatus);
    void lockButtonPressed();

signals:
    void requestEncrypt(bool);
    void requestLock();
    void requestUnlock(bool);

private:
    Ui::LockBar *ui;
    WalletModel *model;
    int currentStatus;
};

#endif // LOCKBAR_H
