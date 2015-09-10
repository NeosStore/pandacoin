// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sendcoinsentry.h"
#include "ui_sendcoinsentry.h"
#include "guiutil.h"
#include "bitcoinunits.h"
#include "bitcoinamountfield.h"
#include "addressbookpage.h"
#include "walletmodel.h"
#include "wallet.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "accountmodel.h"
#include "forms/sendcoinstargetwidget.h"
#include "richtextdelegate.h"

#include <QApplication>
#include <QMessageBox>
#include <QLocale>
#include <QTextDocument>
#include <QScrollBar>
#include <QClipboard>
#include <QTimer>
#include <QGridLayout>
#include <QListView>
#include "coincontroldialog.h"
#include "coincontrol.h"
#include "main.h"

MyAccountEntry::MyAccountEntry(const QString& accountLabel_, const QString& accountAddress_, const QString& accountBalance_)
: accountLabel(new QLabel(accountLabel_))
, accountAddress(new QLabel(accountAddress_))
, accountBalance(new QLabel(accountBalance_))
, amt(new BitcoinAmountField())
{
    amt->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
}

void MyAccountEntry::addToLayout(QGridLayout* layout, int row)
{
    layout->addWidget(accountLabel,row,1);
    layout->addWidget(accountAddress,row,2);
    layout->addWidget(accountBalance,row,3,Qt::AlignRight);
    layout->addWidget(amt,row,4);
}

void MyAccountEntry::update(const QString& accountLabel_, const QString& accountBalance_)
{
    accountLabel->setText(accountLabel_);
    accountBalance->setText(accountBalance_);
}

QString MyAccountEntry::getAccountAddress()
{
    return accountAddress->text();
}

void MyAccountEntry::clear()
{
    amt->clear();
}

bool MyAccountEntry::validate()
{
    bool retval = amt->validate(true);
    if(!retval)
    {
        amt->setValid(false);
    }
    return retval;
}

SendCoinsRecipient MyAccountEntry::getValue()
{
    SendCoinsRecipient rv;

    rv.address = accountAddress->text().toStdString();
    rv.label = accountLabel->text().toStdString();
    rv.amount = amt->value();

    return rv;
}

qint64 MyAccountEntry::getAmount()
{
    return amt->value();
}

void MyAccountEntry::setDisplayUnit(int unit)
{
    amt->setDisplayUnit(unit);
}


SendCoinsEntry::SendCoinsEntry(QWidget *parent)
: QFrame(parent)
, ui(new Ui::SendCoinsEntry)
, model(NULL)
, accountListModel(NULL)
, newRecipientAllowed(false)
{
    ui->setupUi(this);

    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(coinControlChangeEdited(const QString &)));
    connect(ui->send_coins_myaccount_next, SIGNAL(pressed()), this, SLOT(myAccountNextButtonPressed()));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    addAnotherButtonPressed();

    newRecipientAllowed = true;
}


SendCoinsEntry::~SendCoinsEntry()
{
    for(QList<MyAccountEntry*>::Iterator iter = mapMyAccounts.begin(); iter != mapMyAccounts.end(); iter++)
    {
        delete *iter;
    }
    mapMyAccounts.clear();
    delete ui;
}


void SendCoinsEntry::updateRemoveEnabled()
{
    // Delete button should only be visible when more than one recipient is present.
    bool enabled = mapTargetWidgets.size() > 1;

    QList<SendCoinsTargetWidget*>::iterator iter = mapTargetWidgets.begin();
    for(; iter != mapTargetWidgets.end(); iter++)
    {
        (*iter)->setRemoveEnabled(enabled);
    }
}

void SendCoinsEntry::clearTargetWidgets()
{
    QList<SendCoinsTargetWidget*>::iterator iter = mapTargetWidgets.begin();
    for(; iter != mapTargetWidgets.end(); iter++)
    {
        (*iter)->deleteLater();
    }
    mapTargetWidgets.clear();
    addAnotherButtonPressed();
}

void SendCoinsEntry::deleteButtonPressed()
{
    mapTargetWidgets.removeOne(dynamic_cast<SendCoinsTargetWidget*>(sender()));
    sender()->deleteLater();
    updateRemoveEnabled();
}

void SendCoinsEntry::pasteButtonPressed()
{
    // Paste text from clipboard into recipient field
    dynamic_cast<SendCoinsTargetWidget*>(sender())->setRecipient(QApplication::clipboard()->text());
}

void SendCoinsEntry::addressBookButtonPressed()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSending, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        dynamic_cast<SendCoinsTargetWidget*>(sender())->setRecipient(dlg.getReturnValue());
    }
}

void SendCoinsEntry::addAnotherButtonPressed()
{
    // Temporarily disable UI updating to prevent flicker - we enable it again once we have scrolled into position.
    setUpdatesEnabled(false);

    // Insert widget for new recipient
    SendCoinsTargetWidget* recipient = new SendCoinsTargetWidget(this);
    mapTargetWidgets.push_back(recipient);
    ui->send_coin_layout->insertWidget(ui->send_coin_layout->count()-3,recipient);

    // Ensure remove button is enabled/disabled appropriately depending on number of widgets present
    updateRemoveEnabled();

    // Ensure widget visible on screen and focused - have to do this in a timer because QScrollArea is badly designed.
    QTimer::singleShot(200, this, SLOT(scrollToLastRecipient()));
    recipient->setFocus();

}

void SendCoinsEntry::scrollToLastRecipient()
{
    ui->scrollArea->ensureWidgetVisible(mapTargetWidgets[mapTargetWidgets.size()-1]);
    setUpdatesEnabled(true);
}


void SendCoinsEntry::confirmButtonPressed()
{
    if(ui->from_account_list->currentIndex() == 0)
    {
        if(!model->getOptionsModel()->getCoinControlFeatures())
        {
            QMessageBox::information(this, tr("No from account selected"), tr("You have not selected an account from which to make the payment.\nPlease select the \"from\" address at the top of this page."), QMessageBox::Ok);
            return;
        }
    }

    GUIUtil::flagLocker locker(newRecipientAllowed);

    std::vector<SendCoinsRecipient> recipients;
    bool valid = true;

    if(!model)
        return;

    for(int i = 0; i < mapTargetWidgets.size(); ++i)
    {
        if(mapTargetWidgets[i]->validate(model))
        {
            recipients.push_back(mapTargetWidgets[i]->getValue());
        }
        else
        {
            valid = false;
        }
    }

    if(!valid || recipients.empty())
    {
        return;
    }

    std::string sendAccountAddress=model->getMyAccountModel()->data(1,ui->from_account_list->currentIndex() - 1).toString().trimmed().toStdString();
    std::string transactionHash;
    if(GUIUtil::SendCoinsHelper(this, recipients, model, sendAccountAddress, false, transactionHash))
    {
        clearTargetWidgets();
        CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
    }
}

void SendCoinsEntry::myAccountNextButtonPressed()
{
    if(ui->from_account_list->currentIndex() == 0)
    {
        if(!model->getOptionsModel()->getCoinControlFeatures())
        {
            QMessageBox::information(this, tr("No from account selected"), tr("You have not selected an account from which to make the payment.\nPlease select the \"from\" address at the top of this page."), QMessageBox::Ok);
            return;
        }
    }

    GUIUtil::flagLocker locker(newRecipientAllowed);

    std::vector<SendCoinsRecipient> recipients;
    bool valid = true;

    if(!model)
        return;

    for(int i = 0; i < mapMyAccounts.size(); ++i)
    {
        if(mapMyAccounts[i]->validate())
        {
            if(mapMyAccounts[i]->getAmount() > 0)
            {
                recipients.push_back(mapMyAccounts[i]->getValue());
            }
        }
        else
        {
            valid = false;
        }
    }

    if(!valid || recipients.empty())
    {
        return;
    }

    std::string sendAccountAddress=model->getMyAccountModel()->data(1,ui->from_account_list->currentIndex() - 1).toString().trimmed().toStdString();
    std::string transactionHash;
    if(GUIUtil::SendCoinsHelper(this, recipients, model, sendAccountAddress, false, transactionHash))
    {
        for(int i = 0; i < mapMyAccounts.size(); ++i)
        {
            mapMyAccounts[i]->clear();
        }
        CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
    }
}

void SendCoinsEntry::payToAddressChanged(QString address)
{
    if(!model)
        return;
    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
        dynamic_cast<SendCoinsTargetWidget*>(sender())->setRecipientName(associatedLabel);
}

void SendCoinsEntry::updateMyAccountList()
{
    // Generate/Update list of accounts
    int numRows = model->getMyAccountModel()->rowCount(QModelIndex());
    QList<MyAccountEntry*> newMyAccountsMap;
    for(int i = 0; i < numRows; i++)
    {
        QString accountAddress = model->getMyAccountModel()->data(AccountModel::Address,i).toString();
        QString accountLabel = model->getMyAccountModel()->data(AccountModel::Label,i).toString();
        QString accountBalance = model->getMyAccountModel()->data(AccountModel::Balance,i).toString();
        bool alreadyCreated = false;
        for(QList<MyAccountEntry*>::Iterator iter = mapMyAccounts.begin(); iter != mapMyAccounts.end(); iter++)
        {
            if((*iter)->getAccountAddress() == accountAddress)
            {
                (*iter)->update(accountLabel, accountBalance);
                newMyAccountsMap.push_back(*iter);
                mapMyAccounts.erase(iter);
                alreadyCreated = true;
                break;
            }
        }
        if(!alreadyCreated)
        {
            MyAccountEntry *newEntry = new MyAccountEntry(accountLabel, accountAddress, accountBalance);
            newMyAccountsMap.push_back(newEntry);
        }
    }
    for(QList<MyAccountEntry*>::Iterator iter = mapMyAccounts.begin(); iter != mapMyAccounts.end(); iter++)
    {
        delete *iter;
    }
    mapMyAccounts = newMyAccountsMap;

    // First shift next button to very end
    ui->send_coins_group_myaccounts_grid_layout->addWidget(ui->send_coins_myaccount_next,numRows+2,4,Qt::AlignRight|Qt::AlignHCenter);
    for(int i = 0; i < mapMyAccounts.size(); i++)
    {
        mapMyAccounts[i]->addToLayout(ui->send_coins_group_myaccounts_grid_layout,i+1);
    }
}

void SendCoinsEntry::setModel(WalletModel *model)
{
    GUIUtil::flagLocker locker(newRecipientAllowed);

    //fixme: LEAK LEAK
    RichTextDelegate* richTextDelegate = new RichTextDelegate(this);

    this->model = model;

    if(model && model->getOptionsModel())
    {
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        connect(model->getMyAccountModel(), SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SLOT(updateMyAccountList()));
        connect(model, SIGNAL(addressBookUpdated()), this, SLOT(updateMyAccountList()));
        connect(model, SIGNAL(balanceChanged(qint64,qint64,qint64,qint64)), this, SLOT(updateMyAccountList()));

        accountListModel = new SingleColumnAccountModel(model->getMyAccountModel(), true, false, tr("Search your accounts list..."));
        ui->from_account_list->setModel(accountListModel);
        ui->from_account_list->setItemDelegate(richTextDelegate);
        // Sadly the below is necessary in order to be able to style QComboBox pull down lists properly.
        ui->from_account_list->setView(new QListView(this));

        updateMyAccountList();

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        connect(model->getOptionsModel(), SIGNAL(transactionFeeChanged(qint64)), this, SLOT(coinControlUpdateLabels()));

        if(model->getOptionsModel()->getCoinControlFeatures())
        {
            ui->frameCoinControl->setVisible(true);
            ui->from_account_list->setVisible(false);
            ui->from_label->setVisible(false);
        }
        else
        {
            ui->frameCoinControl->setVisible(false);
            ui->from_account_list->setVisible(true);
            ui->from_label->setVisible(true);
        }
        coinControlUpdateLabels();
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}


bool SendCoinsEntry::validate()
{
    // Check input validity.
    bool retval = true;

    QList<SendCoinsTargetWidget*>::iterator iter = mapTargetWidgets.begin();
    for(; iter != mapTargetWidgets.end(); iter++)
    {
        if(!(*iter)->validate(model))
        {
            retval = false;
        }
    }

    return retval;
}



void SendCoinsEntry::setFocus()
{
    mapTargetWidgets[0]->setFocus();
}

void SendCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        int unit = model->getOptionsModel()->getDisplayUnit();

        for(QList<SendCoinsTargetWidget*>::iterator iter = mapTargetWidgets.begin(); iter != mapTargetWidgets.end(); iter++)
        {
            (*iter)->setDisplayUnit(unit);
        }

        for(QList<MyAccountEntry*>::Iterator iter = mapMyAccounts.begin(); iter != mapMyAccounts.end(); iter++)
        {
            (*iter)->setDisplayUnit(unit);
        }
        // Necessary for balance display to update properly.
        updateMyAccountList();
    }
}

void SendCoinsEntry::pasteEntry(const SendCoinsRecipient &rv)
{
    if(!newRecipientAllowed)
        return;

    GUIUtil::flagLocker locker(newRecipientAllowed);

    // Replace the first empty entry if there is one
    bool done=false;
    QList<SendCoinsTargetWidget*>::iterator iter = mapTargetWidgets.begin();
    for(; iter != mapTargetWidgets.end(); iter++)
    {
        if((*iter)->isClear())
        {
            (*iter)->setValue(rv);
        }
    }
    if(!done)
    {
        addAnotherButtonPressed();
        mapTargetWidgets[mapTargetWidgets.size()-1]->setValue(rv);
    }
}

bool SendCoinsEntry::handleURI(const QString &uri)
{
    SendCoinsRecipient rv;
    // URI has to be valid
    if (GUIUtil::parseBitcoinURI(uri, &rv))
    {
        CBitcoinAddress address(rv.address);
        if (!address.IsValid())
            return false;
        pasteEntry(rv);
        return true;
    }

    return false;
}


// Coin Control: copy label "Quantity" to clipboard
void SendCoinsEntry::coinControlClipboardQuantity()
{
    QApplication::clipboard()->setText(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void SendCoinsEntry::coinControlClipboardAmount()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void SendCoinsEntry::coinControlClipboardFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")));
}

// Coin Control: copy label "After fee" to clipboard
void SendCoinsEntry::coinControlClipboardAfterFee()
{
    QApplication::clipboard()->setText(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")));
}

// Coin Control: copy label "Bytes" to clipboard
void SendCoinsEntry::coinControlClipboardBytes()
{
    QApplication::clipboard()->setText(ui->labelCoinControlBytes->text());
}

// Coin Control: copy label "Priority" to clipboard
void SendCoinsEntry::coinControlClipboardPriority()
{
    QApplication::clipboard()->setText(ui->labelCoinControlPriority->text());
}

// Coin Control: copy label "Low output" to clipboard
void SendCoinsEntry::coinControlClipboardLowOutput()
{
    QApplication::clipboard()->setText(ui->labelCoinControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void SendCoinsEntry::coinControlClipboardChange()
{
    QApplication::clipboard()->setText(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")));
}

// Coin Control: settings menu - coin control enabled/disabled by user
void SendCoinsEntry::coinControlFeatureChanged(bool checked)
{
    if(checked)
    {
        ui->frameCoinControl->setVisible(true);
        ui->from_account_list->setVisible(false);
        ui->from_label->setVisible(false);
    }
    else
    {
        ui->frameCoinControl->setVisible(false);
        ui->from_account_list->setVisible(true);
        ui->from_label->setVisible(true);
    }

    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl->SetNull();
}

// Coin Control: button inputs -> show actual coin control dialog
void SendCoinsEntry::coinControlButtonClicked()
{
    CoinControlDialog dlg(this);
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: checkbox custom change address
void SendCoinsEntry::coinControlChangeChecked(int state)
{
    if (model)
    {
        if (state == Qt::Checked)
            CoinControlDialog::coinControl->destChange = CBitcoinAddress(ui->lineEditCoinControlChange->text().toStdString()).Get();
        else
            CoinControlDialog::coinControl->destChange = CNoDestination();
    }

    ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
    ui->labelCoinControlChangeLabel->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void SendCoinsEntry::coinControlChangeEdited(const QString & text)
{
    if (model)
    {
        CoinControlDialog::coinControl->destChange = CBitcoinAddress(text.toStdString()).Get();

        // label for the change address
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");
        if (text.isEmpty())
            ui->labelCoinControlChangeLabel->setText("");
        else if (!CBitcoinAddress(text.toStdString()).IsValid())
        {
            ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");
            ui->labelCoinControlChangeLabel->setText(tr("WARNING: Invalid Pandacoin address"));
        }
        else
        {
            QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
            if (!associatedLabel.isEmpty())
                ui->labelCoinControlChangeLabel->setText(associatedLabel);
            else
            {
                CPubKey pubkey;
                CKeyID keyid;
                CBitcoinAddress(text.toStdString()).GetKeyID(keyid);
                if (model->getPubKey(keyid, pubkey))
                    ui->labelCoinControlChangeLabel->setText(tr("(no label)"));
                else
                {
                    ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");
                    ui->labelCoinControlChangeLabel->setText(tr("WARNING: unknown change address"));
                }
            }
        }
    }
}

// Coin Control: update labels
void SendCoinsEntry::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
        return;

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    for(int i = 0; i < mapTargetWidgets.size(); ++i)
    {
        CoinControlDialog::payAmounts.append(mapTargetWidgets[i]->getValue().amount);
    }

    if (CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}


