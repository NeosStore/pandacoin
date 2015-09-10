// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionfilterproxy.h"

#include "transactiontablemodel.h"
#include "transactionrecord.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include <QDateTime>

#include <cstdlib>

// Earliest date that can be represented (far in the past)
const QDateTime TransactionFilterProxy::MIN_DATE = QDateTime::fromTime_t(0);
// Last date that can be represented (far in the future)
const QDateTime TransactionFilterProxy::MAX_DATE = QDateTime::fromTime_t(0xFFFFFFFF);

TransactionFilterProxy::TransactionFilterProxy(WalletModel* walletModel_, QObject *parent) :
    QSortFilterProxyModel(parent),
    dateFrom(MIN_DATE),
    dateTo(MAX_DATE),
    addrPrefix(),
    typeFilter(ALL_TYPES),
    minAmount(0),
    limitRows(-1),
    showInactive(true),
    walletModel(walletModel_)
{
}

bool TransactionFilterProxy::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    int type = index.data(TransactionTableModel::TypeRole).toInt();
    QDateTime datetime = index.data(TransactionTableModel::DateRole).toDateTime();
    QString toAddress = index.data(TransactionTableModel::AddressRole).toString();
    QString fromAddress = index.data(TransactionTableModel::FromAddressRole).toString();
    QString toLabel = index.data(TransactionTableModel::LabelRole).toString();
    QString fromLabel = index.data(TransactionTableModel::FromLabelRole).toString();
    qint64 amount = llabs(index.data(TransactionTableModel::AmountRole).toLongLong());
    int status = index.data(TransactionTableModel::StatusRole).toInt();

    if(!showInactive && (status == TransactionStatus::Conflicted || status == TransactionStatus::NotAccepted))
        return false;
    if(!(TYPE(type) & typeFilter))
        return false;
    if(datetime < dateFrom || datetime > dateTo)
        return false;
    if(amount < minAmount)
        return false;

    if (addrPrefix.isEmpty())
        return true;

    if(toAddress.contains(addrPrefix, Qt::CaseInsensitive) || toLabel.contains(addrPrefix, Qt::CaseInsensitive))
    {
        if(type != TransactionRecord::InternalSend)
        {
            return true;
        }
    }
    if(fromAddress.contains(addrPrefix, Qt::CaseInsensitive) || fromLabel.contains(addrPrefix, Qt::CaseInsensitive))
    {
        if(type != TransactionRecord::InternalReceive)
        {
            return true;
        }
    }
    return false;
}


bool TransactionFilterProxy::filterAcceptsColumn(int source_column, const QModelIndex & source_parent) const
{
    // Always hide 'from address' and 'to address' columns
    if(source_column == TransactionTableModel::ToAddress || source_column == TransactionTableModel::FromAddress)
    {
        return false;
    }

    // Don't show 'account address' column when we are filtering by account
    if(addrPrefix != "")
    {
        if(source_column == TransactionTableModel::OurAddress)
        {
            return false;
        }
    }
    else
    {
        if(source_column == TransactionTableModel::OtherAddress)
        {
            return false;
        }
    }
    return true;
}

void TransactionFilterProxy::setDateRange(const QDateTime &from, const QDateTime &to)
{
    this->dateFrom = from;
    this->dateTo = to;
    invalidateFilter();
    emit(layoutChanged());
}

void TransactionFilterProxy::setAddressPrefix(const QString &addrPrefix)
{
    this->addrPrefix = addrPrefix;
    invalidateFilter();
    emit(layoutChanged());
    emit(reset());
}

void TransactionFilterProxy::setTypeFilter(quint32 modes)
{
    this->typeFilter = modes;
    invalidateFilter();
    emit(layoutChanged());
}

void TransactionFilterProxy::setMinAmount(qint64 minimum)
{
    this->minAmount = minimum;
    invalidateFilter();
    emit(layoutChanged());
}

void TransactionFilterProxy::setLimit(int limit)
{
    this->limitRows = limit;
}

void TransactionFilterProxy::setShowInactive(bool showInactive)
{
    this->showInactive = showInactive;
    invalidateFilter();
    emit(layoutChanged());
}

int TransactionFilterProxy::rowCount(const QModelIndex &parent) const
{
    if(limitRows != -1)
    {
        return std::min(QSortFilterProxyModel::rowCount(parent), limitRows);
    }
    else
    {
        return QSortFilterProxyModel::rowCount(parent);
    }
}

int TransactionFilterProxy::columnCount(const QModelIndex &parent) const
{
    return sourceModel()->columnCount()-3;
}

void TransactionFilterProxy::getLast30DaysInOut(qint64& in, qint64& out)
{
    QDate today = QDate::currentDate();
    QDate cutoff = today.addDays(-30);
    in = out = 0;

    for(int i=0; i < rowCount(); i++)
    {
        QModelIndex idx = mapToSource(index(i, 0));
        TransactionRecord *rec = static_cast<TransactionRecord*>(idx.internalPointer());
        if(QDateTime::fromTime_t(static_cast<uint>(rec->time)).date() > cutoff)
        {
            if(rec->type != TransactionRecord::InternalSend && rec->type != TransactionRecord::InternalReceive)
            {
                in += rec->credit;
                out += rec->debit;
            }
            // Don't include internal transfers for 'all accounts' summary
            else if(addrPrefix != "")
            {
                // If it is a transaction from an account to the same account (Yes this is possible - because of 'sub' accounts for change etc.) then exclude it.
                if(rec->fromAddress != rec->address)
                {
                    QString toAddress = QString::fromStdString(rec->address);
                    QString fromAddress = QString::fromStdString(rec->fromAddress);
                    QString toLabel = walletModel->getAddressTableModel()->labelForAddress(toAddress);
                    QString fromLabel = walletModel->getAddressTableModel()->labelForAddress(fromAddress);
                    if (toAddress.contains(addrPrefix, Qt::CaseInsensitive) ||  toLabel.contains(addrPrefix, Qt::CaseInsensitive))
                    {
                        in += rec->credit;
                    }
                    else if (fromAddress.contains(addrPrefix, Qt::CaseInsensitive) ||  toLabel.contains(addrPrefix, Qt::CaseInsensitive))
                    {
                        out += rec->debit;
                    }
                }
            }
        }
    }
}
