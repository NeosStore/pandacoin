#include "transactionfilterwidget.h"
#include "ui_transactionfilterwidget.h"
#include "walletmodel.h"
#include "transactionfilterproxy.h"

TransactionFilterWidget::TransactionFilterWidget(QWidget *parent)
: QFrame(parent)
, ui(new Ui::TransactionFilterWidget)
{
    ui->setupUi(this);

    ui->recent_transaction_button->setCheckable(true);
    ui->recent_transaction_button->setChecked(true);

    connect(ui->tabbed_date_widget, SIGNAL(tabPressed()), this, SLOT(dateTabPressed()));
    connect(ui->tabbed_date_widget, SIGNAL(tabReleased()), this, SLOT(dateTabReleased()));
    connect(ui->recent_transaction_button, SIGNAL(pressed()), this, SLOT(recentTransactionButtonPressed()));
    connect(ui->transaction_export_button,SIGNAL(pressed()),this,SIGNAL(exportClicked()));
}

TransactionFilterWidget::~TransactionFilterWidget()
{
    delete ui;
}

void TransactionFilterWidget::setModel(TransactionFilterProxy *model_)
{
    model = model_;
    ui->tabbed_date_widget->setModel(model);
}

void TransactionFilterWidget::dateTabPressed()
{
    ui->recent_transaction_button->setChecked(false);
    ui->recent_transaction_button_arrow->setPixmap(QPixmap(":/icons/down_arrow5"));
}

void TransactionFilterWidget::dateTabReleased()
{
    ui->recent_transaction_button->setChecked(true);
    ui->recent_transaction_button_arrow->setPixmap(QPixmap(":/icons/down_arrow3"));
}

void TransactionFilterWidget::recentTransactionButtonPressed()
{
    if(ui->recent_transaction_button->isChecked())
        ui->recent_transaction_button->setChecked(false);
    ui->tabbed_date_widget->clearDateSelections();
    ui->recent_transaction_button_arrow->setPixmap(QPixmap(":/icons/down_arrow3"));
}

void TransactionFilterWidget::on_recent_transaction_searchbox_returnPressed()
{
    model->setAddressPrefix(ui->recent_transaction_searchbox->text());
}

void TransactionFilterWidget::on_transaction_search_button_pressed()
{
    model->setAddressPrefix(ui->recent_transaction_searchbox->text());
}
