#include "portfoliooverviewtable.h"
#include <QResizeEvent>
#include <QHeaderView>

PortfolioOverviewTable::PortfolioOverviewTable(QWidget *parent)
: QTableView(parent)
{
    // Turn manual resize grab handles off.
    horizontalHeader()->setResizeMode(QHeaderView::Fixed);

    // Elide in the middle instead of on the right.
    setTextElideMode(Qt::ElideMiddle);
}

void PortfolioOverviewTable::resizeEvent(QResizeEvent *event)
{
    int marginRight = 8;
    int totalWidthToFill = viewport()->width() - marginRight;

    // Split the width equally in three
    int width1, width2, width3;
    width1 = width2 = width3 = totalWidthToFill/3;
    width2 += totalWidthToFill%3;

    // Once we go below 590 px we no longer have the luxury of doing the below, rather let column 2 ellipses so that others don't.
    if(totalWidthToFill > 580)
    {
        // Ensure that width2 is wide enough not to have ellipses
        if(width2 < 280)
        {
            int diff = 280 - width2;
            width2 = 280;
            width1 -= diff/2;
            width3 -= diff/2+diff%2;
        }
    }

    setColumnWidth(0, width1);
    setColumnWidth(1, width2);
    setColumnWidth(2, width3);
}

// 'Shrink to fit' vertical data.
void PortfolioOverviewTable::layoutChanged()
{
    int bestHeight = (model()->rowCount()+1) * horizontalHeader()->height();
    setMaximumHeight(bestHeight);
}
