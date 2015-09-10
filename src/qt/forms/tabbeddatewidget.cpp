#include "tabbeddatewidget.h"
#include "ui_tabbeddatewidget.h"
#include "walletmodel.h"
#include "transactiontablemodel.h"
#include "transactionfilterproxy.h"
#include "guiutil.h"
#include <QPushButton>
#include <QScrollBar>
#include <QTimer>
#include <QDateTime>

#include <map>
#include <set>

TabbedDateWidget::TabbedDateWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TabbedDateWidget),
    model(NULL)
{
    ui->setupUi(this);

    ui->push_button_next->setAutoRepeat(true);
    ui->push_button_prev->setAutoRepeat(true);
}

TabbedDateWidget::~TabbedDateWidget()
{
    delete ui;
}


void TabbedDateWidget::setModel(TransactionFilterProxy *model_)
{
    model = model_;

    dateButtonDateMapping.clear();

    std::set<QDate> allDates;
    for(int i=0; i<model->rowCount(); i++)
    {
        QVariant varData = model->data(model->index(i,1));
        QString dateString = varData.toString();

        QDate date = GUIUtil::dateFromString(dateString).date();
        allDates.insert(QDate(date.year(),date.month(),1));
    }

    for(std::set<QDate>::iterator iter = allDates.begin(); iter != allDates.end(); ++iter)
    {
        QPushButton* dateButton = new QPushButton(QDate::shortMonthName(iter->month()) + " " + QString::number(iter->year()),this);
        connect(dateButton, SIGNAL(pressed()), this, SLOT(date_button_pressed()));
        dateButtonDateMapping[dateButton]=*iter;
        ui->horizontal_scroll_layout->insertWidget(0,dateButton);
        dateButton->setCheckable(true);
    }

    scrollContentsUpdated();
}

void TabbedDateWidget::clearDateSelections()
{
    for(std::map<QPushButton*,QDate>::const_iterator iter = dateButtonDateMapping.begin(); iter != dateButtonDateMapping.end(); iter++)
    {
        iter->first->setChecked(false);
    }
    model->setDateRange(TransactionFilterProxy::MIN_DATE,TransactionFilterProxy::MAX_DATE);
}

void TabbedDateWidget::on_push_button_next_pressed()
{
    ui->horizontal_scroll_area->horizontalScrollBar()->setValue(ui->horizontal_scroll_area->horizontalScrollBar()->value() + 50);
    if(ui->horizontal_scroll_area->horizontalScrollBar()->value() == ui->horizontal_scroll_area->horizontalScrollBar()->maximum())
        ui->push_button_next->setDisabled(true);
    ui->push_button_prev->setDisabled(false);
}

void TabbedDateWidget::on_push_button_prev_pressed()
{
    ui->horizontal_scroll_area->horizontalScrollBar()->setValue(ui->horizontal_scroll_area->horizontalScrollBar()->value() - 50);
    if(ui->horizontal_scroll_area->horizontalScrollBar()->value() == 0)
        ui->push_button_prev->setDisabled(true);
    ui->push_button_next->setDisabled(false);
}

void TabbedDateWidget::date_button_pressed()
{
    QPushButton* pushButton = dynamic_cast<QPushButton*>(sender());
    QDate date = dateButtonDateMapping[pushButton];

    for(std::map<QPushButton*,QDate>::const_iterator iter = dateButtonDateMapping.begin(); iter != dateButtonDateMapping.end(); iter++)
    {
        if(iter->first != pushButton)
        {
            iter->first->setChecked(false);
        }
    }

    if(pushButton->isChecked())
    {
        model->setDateRange(TransactionFilterProxy::MIN_DATE,TransactionFilterProxy::MAX_DATE);
        emit(tabReleased());
    }
    else
    {
        QDateTime fromDate;
        QDateTime toDate;

        fromDate.setDate(QDate(date.year(),date.month(),1));
        toDate.setDate(QDate(date.year(),date.month(),date.daysInMonth()));

        model->setDateRange(fromDate,toDate);
        emit(tabPressed());
    }
}

void TabbedDateWidget::scrollContentsUpdated()
{
    // Ensure rightmost date button is visible by default, disable scroll right button by default as we won't need it unless we scroll left first.
    ui->horizontal_scroll_area->horizontalScrollBar()->setValue(ui->horizontal_scroll_area->horizontalScrollBar()->maximum());
    ui->push_button_next->setDisabled(true);
}
