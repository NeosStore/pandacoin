#include "sendcoinstargetwidget.h"
#include "ui_sendcoinstargetwidget.h"
#include "walletmodel.h"

SendCoinsTargetWidget::SendCoinsTargetWidget(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SendCoinsTargetWidget)
{
    ui->setupUi(this);

    connect(ui->delete_button, SIGNAL(pressed()), this, SIGNAL(deleteButtonPressed()));
    connect(ui->paste_button, SIGNAL(pressed()), this, SIGNAL(pasteButtonPressed()));
    connect(ui->address_book_button, SIGNAL(pressed()), this, SIGNAL(addressBookButtonPressed()));
    connect(ui->add_another_button, SIGNAL(clicked()), this, SIGNAL(addAnotherButtonPressed()));
    connect(ui->next_button, SIGNAL(pressed()), this, SIGNAL(confirmButtonPressed()));
    connect(ui->pay_to_address, SIGNAL(textChanged(QString)), this, SIGNAL(payToAddressChanged(QString)));
    connect(this, SIGNAL(deleteButtonPressed()), parent, SLOT(deleteButtonPressed()));
    connect(this, SIGNAL(pasteButtonPressed()), parent, SLOT(pasteButtonPressed()));
    connect(this, SIGNAL(addressBookButtonPressed()), parent, SLOT(addressBookButtonPressed()));
    connect(this, SIGNAL(addAnotherButtonPressed()), parent, SLOT(addAnotherButtonPressed()));
    connect(this, SIGNAL(confirmButtonPressed()), parent, SLOT(confirmButtonPressed()));
    connect(this, SIGNAL(payToAddressChanged(QString)), parent, SLOT(payToAddressChanged(QString)));
}

SendCoinsTargetWidget::~SendCoinsTargetWidget()
{
    delete ui;
}


void SendCoinsTargetWidget::setRemoveEnabled(bool enabled)
{
    ui->delete_button->setEnabled(enabled);
}


void SendCoinsTargetWidget::setRecipient(const QString& recipient)
{
    ui->pay_to_address->setText(recipient);
    ui->pay_amount->setFocus();
}

void SendCoinsTargetWidget::setRecipientName(const QString& name)
{
    ui->pay_to_name->setText(name);
    ui->pay_amount->setFocus();
}

bool SendCoinsTargetWidget::validate(WalletModel *model)
{
    // Check input validity
    bool retval = true;

    if(!ui->pay_amount->validate())
    {
        retval = false;
    }
    else
    {
        if(ui->pay_amount->value() <= 0)
        {
            // Cannot send 0 coins or less
            ui->pay_amount->setValid(false);
            retval = false;
        }
    }

    if(!ui->pay_to_address->hasAcceptableInput() || (model && !model->validateAddress(ui->pay_to_address->text().toStdString())))
    {
        ui->pay_to_address->setValid(false);
        retval = false;
    }
    return retval;
}

bool SendCoinsTargetWidget::isClear()
{
    if(ui->pay_to_address->text() == "" && ui->pay_amount->value() == 0 && ui->pay_to_name->text() == "")
        return true;
    return false;
}

void SendCoinsTargetWidget::setFocus()
{
    ui->pay_to_address->setFocus();
}

// Update payAmount with the current unit
void SendCoinsTargetWidget::setDisplayUnit(int unit)
{
    ui->pay_amount->setDisplayUnit(unit);
}

SendCoinsRecipient SendCoinsTargetWidget::getValue()
{
    SendCoinsRecipient rv;

    rv.address = ui->pay_to_address->text().toStdString();
    rv.label = ui->pay_to_name->text().toStdString();
    rv.amount = ui->pay_amount->value();

    return rv;
}


void SendCoinsTargetWidget::setValue(const SendCoinsRecipient &value)
{
    ui->pay_to_address->setText(QString(value.address.c_str()));
    ui->pay_to_name->setText(QString(value.label.c_str()));
    ui->pay_amount->setValue(value.amount);
}
