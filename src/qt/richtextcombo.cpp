#include "richtextcombo.h"
#include "richtextdelegate.h"

#include <QPaintEvent>
#include <QStylePainter>

#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QPainter>
#include <QLabel>


RichTextCombo::RichTextCombo(QWidget *parent)
: QComboBox(parent)
, renderControl(new QLabel())
{
}


void RichTextCombo::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e)
    QStyleOptionComboBox opt;
    initStyleOption(&opt);

    {
        QStylePainter painter(this);
        painter.setPen(palette().color(QPalette::Text));
        // draw the combobox frame, focusrect and selected etc.
        painter.drawComplexControl(QStyle::CC_ComboBox, opt);
    }

    QPainter painter(this);
    QRect rect = opt.rect.adjusted(5, 1, -20, -2); //compensate for frame and arrow
    painter.translate(rect.topLeft());

    renderControl->setPalette(palette());
    renderControl->setFont(font());
    renderControl->setAlignment(Qt::AlignVCenter);
    QString text = formatBitcoinAmountAsRichString(currentText());

    if(currentIndex() == 0)
    {
        QString hintText = model()->index(0,0).data(Qt::UserRole).toString();
        if(!hintText.isEmpty())
        {
            text = hintText;
            QPalette palette = renderControl->palette();
            palette.setColor(renderControl->foregroundRole(), Qt::gray);
            renderControl->setPalette(palette);
        }
    }

    renderControl->setText(text);
    renderControl->setFixedSize(rect.size());
    renderControl->render(&painter, QPoint(), QRegion(), 0);
}
