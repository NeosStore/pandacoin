// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "bitcoinamountfield.h"
#include "qvaluecombobox.h"
#include "bitcoinunits.h"

#include "guiconstants.h"

#include <QLabel>
#include <QLineEdit>
#include <QRegExpValidator>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QApplication>
#include <QListView>
#include <qmath.h>

BitcoinAmountField::BitcoinAmountField(QWidget *parent)
: QWidget(parent)
, amount(0)
, currentUnit(-1)
, maximumValue(0)
{
    amount = new QDoubleSpinBox(this);
    amount->setLocale(QLocale::c());
    amount->setDecimals(2);
    amount->installEventFilter(this);
    amount->setSingleStep(1);
    amount->setAlignment(Qt::AlignRight);



    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(amount);
    unit = new QValueComboBox(this);
    unit->setModel(new BitcoinUnits(this, 4));
    unit->setView(new QListView(this));

    layout->addWidget(unit);
    layout->setSpacing(0);
    layout->setContentsMargins(0,0,0,0);
    layout->setStretch(0, 1);

    setLayout(layout);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(amount);

    // If one if the widgets changes, the combined content changes as well
    connect(amount, SIGNAL(valueChanged(QString)), this, SIGNAL(textChanged()));
    connect(unit, SIGNAL(currentIndexChanged(int)), this, SLOT(unitChanged(int)));

    // Set default based on configuration
    unitChanged(unit->currentIndex());
}

void BitcoinAmountField::setText(const QString &text)
{
    if (text.isEmpty())
    {
        amount->setValue(0.0);
        amount->clear();
    }
    else
    {
        amount->setValue(text.toDouble());
    }
}

void BitcoinAmountField::clear()
{
    amount->setValue(0.0);
    amount->clear();
    unit->setCurrentIndex(0);
}

bool BitcoinAmountField::validate(bool zeroIsValid)
{
    bool valid = true;
    if (amount->value() == 0.0)
    {
         if(!zeroIsValid)
            valid = false;
    }
    else if (valid && !BitcoinUnits::parse(currentUnit, text(), 0))
        valid = false;

    setValid(valid);

    return valid;
}

void BitcoinAmountField::setValid(bool valid)
{
    if (valid)
        amount->setStyleSheet("");
    else
        amount->setStyleSheet(STYLE_INVALID);
}

QString BitcoinAmountField::text() const
{
    if (amount->text().isEmpty())
        return QString();
    else
        return amount->text();
}

bool BitcoinAmountField::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() == QEvent::FocusIn)
    {
        // Clear invalid flag on focus
        setValid(true);
    }
    else if (event->type() == QEvent::FocusOut)
    {
        if (text() == "")
        {
            clear();
        }
    }
    else if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Comma)
        {
            // Translate a comma into a period
            QKeyEvent periodKeyEvent(event->type(), Qt::Key_Period, keyEvent->modifiers(), ".", keyEvent->isAutoRepeat(), keyEvent->count());
            qApp->sendEvent(object, &periodKeyEvent);
            return true;
        }
    }
    return QWidget::eventFilter(object, event);
}

QWidget *BitcoinAmountField::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, amount);
    return amount;
}

qint64 BitcoinAmountField::value(bool *valid_out) const
{
    qint64 val_out = 0;
    bool valid = BitcoinUnits::parse(currentUnit, text(), &val_out);
    if(valid_out)
    {
        *valid_out = valid;
    }
    return val_out;
}

void BitcoinAmountField::setValue(qint64 value)
{
    setText(BitcoinUnits::format(currentUnit, value, false, false));
}

void BitcoinAmountField::setMaximumValue(double maximumValue_)
{
    maximumValue = maximumValue_;

    qint64 val_out = 0;
    BitcoinUnits::parse(BitcoinUnits::BTC, QString::number(maximumValue), &val_out);
    amount->setMaximum(val_out / BitcoinUnits::factor(currentUnit));
}

void BitcoinAmountField::unitChanged(int idx)
{
    // Use description tooltip for current unit for the combobox
    unit->setToolTip(unit->itemData(idx, Qt::ToolTipRole).toString());

    // Determine new unit ID
    int newUnit = unit->itemData(idx, BitcoinUnits::UnitRole).toInt();

    // Parse current value and convert to new unit
    bool valid = false;
    qint64 currentValue = value(&valid);

    currentUnit = newUnit;

    // Set max length after retrieving the value, to prevent truncation
    amount->setDecimals(2);
    if(maximumValue == 0)
    {
        amount->setMaximum(qPow(10, BitcoinUnits::amountDigits(newUnit)) - qPow(10, -amount->decimals()));
    }
    else
    {
        qint64 val_out = 0;
        BitcoinUnits::parse(BitcoinUnits::BTC, QString::number(maximumValue), &val_out);
        amount->setMaximum(val_out * (BitcoinUnits::factor(BitcoinUnits::BTC)) / BitcoinUnits::factor(newUnit));
    }

    if(valid)
    {
        // If value was valid, re-place it in the widget with the new unit
        setValue(currentValue);
    }
    else
    {
        // If current value is invalid, just clear field
        setText("");
    }
    setValid(true);
}

void BitcoinAmountField::setDisplayUnit(int newUnit)
{
    unit->setValue(newUnit);
}
