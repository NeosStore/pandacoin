#include "transactionviewtable.h"
#include "transactiontablemodel.h"

#include <QResizeEvent>
#include <QHeaderView>
#include <QVector>
#include <numeric>
#include <QScrollBar>



TransactionViewTable::TransactionViewTable(QWidget *parent)
: QTableView(parent)
{

    setSelectionBehavior(QAbstractItemView::SelectItems);
    setSelectionMode(QAbstractItemView::NoSelection);
    setSortingEnabled(true);
    sortByColumn(TransactionTableModel::Date, Qt::DescendingOrder);
    verticalHeader()->hide();
    setShowGrid(false);

    // Turn manual resize grab handles off.
    horizontalHeader()->setResizeMode(QHeaderView::Fixed);
}

void TransactionViewTable::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);
    connect(model, SIGNAL(layoutChanged()), this, SLOT(updateColumns()));
}

void TransactionViewTable::updateColumns()
{
    int columnCount = model()->columnCount();
    if(columnCount == 0)
        return;

    int marginRight = 4;
    int totalWidthToFill = size().width() - ( verticalScrollBar() ? verticalScrollBar()->width() : 0 ) - marginRight;

    QVector<int> columnWidths;
    for(int i = 0; i < columnCount; i++)
    {
        columnWidths.push_back(0);
    }

    // First three columns are special always allow them a fixed width.
    columnWidths[0] = 20;
    columnWidths[1] = 120;
    columnWidths[2] = 145;


    setColumnWidth(0, columnWidths[0]);
    setColumnWidth(1, columnWidths[1]);
    setColumnWidth(2, columnWidths[2]);

    // Distribute rest of the space evenly.
    int filledWidth = std::accumulate(columnWidths.begin(),columnWidths.begin()+3,0);
    int remWidth = (totalWidthToFill - filledWidth) / (columnCount - 3);
    for(int i = 3; i < columnCount; i++)
    {
        setColumnWidth(i, remWidth);
        filledWidth += remWidth;
    }

    // Account for rounding error.
    if(filledWidth < totalWidthToFill)
    {
        setColumnWidth(columnCount-1, remWidth + (totalWidthToFill - filledWidth));
    }
    return;
}

void TransactionViewTable::resizeEvent(QResizeEvent *event)
{
    QTableView::resizeEvent(event);
    updateColumns();
}
