#include "lockbar.h"
#include "ui_lockbar.h"
#include "walletmodel.h"
#include <QIcon>

LockBar::LockBar(QWidget *parent)
: QFrame(parent)
, ui(new Ui::LockBar)
{
    ui->setupUi(this);

    connect(ui->LockButton, SIGNAL(pressed()), this, SLOT(lockButtonPressed()));
}

LockBar::~LockBar()
{
    delete ui;
}

void LockBar::setModel(WalletModel *model_)
{
    model=model_;
    if(model)
    {
        connect(model, SIGNAL(encryptionStatusChanged(int)), this, SLOT(encryptionStatusChanged(int)));
        currentStatus = model->getEncryptionStatus();
    }
}


void LockBar::encryptionStatusChanged(int newStatus)
{
    currentStatus = newStatus;
    switch(newStatus)
    {
        case WalletModel::Unencrypted:
            ui->LockButton->setText(tr("Lock"));
            ui->LockButton->setIcon(QIcon(":/icons/lock_closed_2"));
            ui->LockButton->setToolTip(tr("Wallet is <b>not encrypted</b> and currently <b>unlocked</b> click to encrypt and lock."));
            break;
        case WalletModel::Unlocked:
            ui->LockButton->setText(tr("Lock"));
            ui->LockButton->setIcon(QIcon(":/icons/lock_closed_2"));
            ui->LockButton->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b> click to lock."));
            break;
        case WalletModel::Locked:
            ui->LockButton->setText(tr("Unlock"));
            ui->LockButton->setIcon(QIcon(":/icons/unlock2"));
            ui->LockButton->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b> click to unlock."));
            break;
    }
}

void LockBar::lockButtonPressed()
{
    switch(currentStatus)
    {
        case WalletModel::Unencrypted:
            emit(requestEncrypt(true));
            break;
        case WalletModel::Unlocked:
            emit(requestLock());
            break;
        case WalletModel::Locked:
            emit(requestUnlock(true));
            break;
    }
}
