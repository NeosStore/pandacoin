// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactiontablemodel.h"
#include "guiutil.h"
#include "transactionrecord.h"
#include "guiconstants.h"
#include "transactiondesc.h"
#include "walletmodel.h"
#include "optionsmodel.h"
#include "addresstablemodel.h"
#include "bitcoinunits.h"

#include "wallet.h"
#include "ui_interface.h"

#include <QLocale>
#include <QList>
#include <QColor>
#include <QTimer>
#include <QIcon>
#include <QDateTime>
#include <QtAlgorithms>
#include <map>

// Amount column is right-aligned it contains numbers
// Balance column is right-aligned it contains numbers
static int column_alignments[] = {
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignLeft|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter
    };

static int column_header_alignments[] = {
        Qt::AlignHCenter|Qt::AlignVCenter,
        Qt::AlignHCenter|Qt::AlignVCenter,
        Qt::AlignHCenter|Qt::AlignVCenter,
        Qt::AlignHCenter|Qt::AlignVCenter,
        Qt::AlignHCenter|Qt::AlignVCenter,
        Qt::AlignHCenter|Qt::AlignVCenter,
        Qt::AlignHCenter|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter,
        Qt::AlignRight|Qt::AlignVCenter
    };

// Comparison operator for sort/binary search of model tx list
struct TxLessThan
{
    bool operator()(const TransactionRecord &a, const TransactionRecord &b) const
    {
        return a.hash < b.hash;
    }
    bool operator()(const TransactionRecord &a, const uint256 &b) const
    {
        return a.hash < b;
    }
    bool operator()(const uint256 &a, const TransactionRecord &b) const
    {
        return a < b.hash;
    }
};

// Private implementation
class TransactionTablePriv
{
public:
    TransactionTablePriv(CWallet *wallet, TransactionTableModel *parent)
    : wallet(wallet)
    , parent(parent)
    {
    }
    CWallet *wallet;
    TransactionTableModel *parent;

    /* Local cache of wallet.
     * As it is in the same order as the CWallet, by definition
     * this is sorted by sha256.
     */
    QList<TransactionRecord> cachedWallet;

    std::multimap<QDateTime, TransactionRecord*, std::greater<QDateTime> > dateMapCachedWallet;

    /* Query entire wallet anew from core.
     */
    void refreshWallet()
    {
        OutputDebugStringF("refreshWallet\n");
        cachedWallet.clear();
        dateMapCachedWallet.clear();
        {
            LOCK(wallet->cs_wallet);
            for(std::map<uint256, CWalletTx>::iterator it = wallet->mapWallet.begin(); it != wallet->mapWallet.end(); ++it)
            {
                if(TransactionRecord::showTransaction(it->second))
                {

                    QList<TransactionRecord> toInsert = TransactionRecord::decomposeTransaction(wallet, it->second);
                    foreach(const TransactionRecord &rec, toInsert)
                    {
                        cachedWallet.append(rec);
                        dateMapCachedWallet.insert(std::make_pair(QDateTime::fromTime_t(rec.time),&cachedWallet[cachedWallet.size()-1]));
                    }
                }
            }
        }
    }

    /* Update our model of the wallet incrementally, to synchronize our model of the wallet
       with that of the core.

       Call with transaction that was added, removed or changed.
     */
    void updateWallet(const uint256 &hash, int status)
    {
        if(currentLoadState == LoadState_SyncHeadersFromEpoch)
            return;

        OutputDebugStringF("updateWallet %s %i\n", hash.ToString().c_str(), status);
        {
            LOCK(wallet->cs_wallet);

            // Find transaction in wallet
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(hash);
            bool inWallet = mi != wallet->mapWallet.end();

            // Find bounds of this transaction in model
            QList<TransactionRecord>::iterator lower = qLowerBound(cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
            QList<TransactionRecord>::iterator upper = qUpperBound(cachedWallet.begin(), cachedWallet.end(), hash, TxLessThan());
            int lowerIndex = (lower - cachedWallet.begin());
            int upperIndex = (upper - cachedWallet.begin());
            bool inModel = (lower != upper);

            // Determine whether to show transaction or not
            bool showTransaction = (inWallet && TransactionRecord::showTransaction(mi->second));

            if(status == CT_UPDATED)
            {
                if(showTransaction && !inModel)
                    status = CT_NEW; /* Not in model, but want to show, treat as new */
                if(!showTransaction && inModel)
                    status = CT_DELETED; /* In model, but want to hide, treat as deleted */
            }

            OutputDebugStringF("   inWallet=%i inModel=%i Index=%i-%i showTransaction=%i derivedStatus=%i\n",
                     inWallet, inModel, lowerIndex, upperIndex, showTransaction, status);

            switch(status)
            {
            case CT_NEW:
                if(inModel)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_NEW, but transaction is already in model\n");
                    break;
                }
                if(!inWallet)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_NEW, but transaction is not in wallet\n");
                    break;
                }
                if(showTransaction)
                {
                    // Added -- insert at the right position
                    QList<TransactionRecord> toInsert = TransactionRecord::decomposeTransaction(wallet, mi->second);
                    if(!toInsert.isEmpty()) /* only if something to insert */
                    {
                        parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex+toInsert.size()-1);
                        int insert_idx = lowerIndex;
                        foreach(const TransactionRecord &rec, toInsert)
                        {
                            cachedWallet.insert(insert_idx, rec);
                            dateMapCachedWallet.insert(std::make_pair(QDateTime::fromTime_t(rec.time),&cachedWallet[insert_idx]));
                            insert_idx += 1;
                        }
                        parent->endInsertRows();
                    }
                }
                break;
            case CT_DELETED:
                if(!inModel)
                {
                    OutputDebugStringF("Warning: updateWallet: Got CT_DELETED, but transaction is not in model\n");
                    break;
                }
                // Removed -- remove entire transaction from table
                parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
                dateMapCachedWallet.erase(dateMapCachedWallet.lower_bound(QDateTime::fromTime_t(lower->time)),dateMapCachedWallet.upper_bound(QDateTime::fromTime_t(lower->time)));
                cachedWallet.erase(lower, upper);
                parent->endRemoveRows();
                break;
            case CT_UPDATED:
                // Miscellaneous updates -- nothing to do, status update will take care of this, and is only computed for
                // visible transactions.
                break;
            }
        }
    }

    int size()
    {
        return cachedWallet.size();
    }

    TransactionRecord *index(int idx)
    {
        if(idx >= 0 && idx < cachedWallet.size())
        {
            TransactionRecord *rec = &cachedWallet[idx];

            // If a status update is needed (blocks came in since last check),
            //  update the status of this transaction from the wallet. Otherwise,
            // simply re-use the cached status.
            if(rec->statusUpdateNeeded())
            {
                {
                    LOCK(wallet->cs_wallet);
                    std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);

                    if(mi != wallet->mapWallet.end())
                    {
                        rec->updateStatus(mi->second);
                    }
                }
            }
            return rec;
        }
        else
        {
            return 0;
        }
    }

    //fixme: A bit gross for the underlying model to know about the filter account like this - in an ideal world this would be taken care of by the proxy filter model somehow, but because of the way it has to work this is a bit tricky
    //This is the best way to do it for now, if time/budget allows at some point a more complicated but technically cleaner solution is possible.
    void setCurrentAccountPrefix(QString prefix)
    {
        balanceAccountprefix = prefix;
    }
    QString balanceAccountprefix;

    qint64 getBalance(const TransactionRecord *wtx, WalletModel* walletModel)
    {
        LOCK(wallet->cs_wallet);

        std::multimap<QDateTime, TransactionRecord*>::reverse_iterator finditer(dateMapCachedWallet.upper_bound(QDateTime::fromTime_t(wtx->time)));
        std::multimap<QDateTime, TransactionRecord*>::reverse_iterator iter = finditer;
        while(finditer->second != wtx && finditer != dateMapCachedWallet.rend())
            finditer++;


        if(finditer != dateMapCachedWallet.rend())
        {
            if((*finditer->second).getCachedBalance(balanceAccountprefix.toStdString()) == -1)
            {
                while(iter != dateMapCachedWallet.rbegin() && (*iter->second).getCachedBalance(balanceAccountprefix.toStdString()) == -1)
                {
                    --iter;
                }
                qint64 balance=0;
                if(iter != dateMapCachedWallet.rbegin())
                {
                    balance += (*iter->second).getCachedBalance(balanceAccountprefix.toStdString());
                    ++iter;
                }
                ++finditer;
                while(iter != finditer)
                {
                    if((*iter->second).statusUpdateNeeded())
                    {
                        {
                            LOCK(wallet->cs_wallet);
                            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find((*iter->second).hash);

                            if(mi != wallet->mapWallet.end())
                            {
                                (*iter->second).updateStatus(mi->second);
                            }
                        }
                    }

                    if((*iter->second).status.countsForBalance)
                    {
                        if(balanceAccountprefix.isEmpty() || QString((*iter->second).address.c_str()).contains(balanceAccountprefix) || walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString((*iter->second).address)).contains(balanceAccountprefix))
                        {
                            balance += (*iter->second).credit;
                        }
                        if(balanceAccountprefix.isEmpty() || QString((*iter->second).fromAddress.c_str()).contains(balanceAccountprefix) || walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString((*iter->second).fromAddress)).contains(balanceAccountprefix))
                        {
                            balance += (*iter->second).debit;
                        }
                    }
                    (*iter->second).setCachedBalance(balanceAccountprefix.toStdString(), balance);
                    ++iter;
                }
            }
            return wtx->getCachedBalance(balanceAccountprefix.toStdString());
        }
        //fixme: error.
        return 0;
    }

    QString describe(TransactionRecord *rec)
    {
        {
            LOCK(wallet->cs_wallet);
            std::map<uint256, CWalletTx>::iterator mi = wallet->mapWallet.find(rec->hash);
            if(mi != wallet->mapWallet.end())
            {
                return TransactionDesc::toHTML(wallet, mi->second);
            }
        }
        return QString("");
    }

};

TransactionTableModel::TransactionTableModel(CWallet* wallet, WalletModel *parent)
: QAbstractTableModel(parent)
, wallet(wallet)
, walletModel(parent)
, priv(new TransactionTablePriv(wallet, this))
, cachedNumBlocks(0)
{
    columns << QString() << tr("Date") << tr("Type") << tr("From account") << tr("To account") << tr("Account Name") << tr("Account Name") << tr("Amount") << tr("Balance");

    priv->refreshWallet();

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateConfirmations()));
    timer->start(MODEL_UPDATE_DELAY);

    connect(walletModel->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
}

TransactionTableModel::~TransactionTableModel()
{
    delete priv;
}

void TransactionTableModel::updateTransaction(const QString &hash, int status)
{
    if(currentLoadState == LoadState_SyncHeadersFromEpoch)
        return;

    uint256 updated;
    updated.SetHex(hash.toStdString());

    priv->updateWallet(updated, status);
}

void TransactionTableModel::updateConfirmations()
{
    if (currentClientMode == ClientFull || currentLoadState > LoadState_SyncBlocksFromEpoch)
    {
        if(nBestHeight != cachedNumBlocks)
        {
            cachedNumBlocks = nBestHeight;
            // Blocks came in since last poll.
            // Invalidate status (number of confirmations) and (possibly) description
            //  for all rows. Qt is smart enough to only actually request the data for the
            //  visible rows.
            emit dataChanged(index(0, Status), index(priv->size()-1, Status));
            emit dataChanged(index(0, ToAddress), index(priv->size()-1, ToAddress));
            emit dataChanged(index(0, FromAddress), index(priv->size()-1, FromAddress));
        }
    }
}

int TransactionTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return priv->size();
}

int TransactionTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return columns.length();
}

QString TransactionTableModel::formatTxStatus(const TransactionRecord *wtx) const
{
    QString status;

    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
        status = tr("Open for %n more block(s)","",wtx->status.open_for);
        break;
    case TransactionStatus::OpenUntilDate:
        status = tr("Open until %1").arg(GUIUtil::dateTimeStr(wtx->status.open_for));
        break;
    case TransactionStatus::Offline:
        status = tr("Offline");
        break;
    case TransactionStatus::Unconfirmed:
        status = tr("Unconfirmed");
        break;
    case TransactionStatus::Confirming:
        status = tr("Confirming (%1 of %2 recommended confirmations)").arg(wtx->status.depth).arg(TransactionRecord::RecommendedNumConfirmations);
        break;
    case TransactionStatus::Confirmed:
        status = tr("Confirmed (%1 confirmations)").arg(wtx->status.depth);
        break;
    case TransactionStatus::Conflicted:
        status = tr("Conflicted");
        break;
    case TransactionStatus::Immature:
        status = tr("Immature (%1 confirmations, will be available after %2)").arg(wtx->status.depth).arg(wtx->status.depth + wtx->status.matures_in);
        break;
    case TransactionStatus::MaturesWarning:
        status = tr("This block was not received by any other nodes and will probably not be accepted!");
        break;
    case TransactionStatus::NotAccepted:
        status = tr("Generated but not accepted");
        break;
    }

    return status;
}

QString TransactionTableModel::formatTxDate(const TransactionRecord *wtx) const
{
    if(wtx->time)
    {
        return GUIUtil::dateTimeStr(wtx->time);
    }
    else
    {
        return QString();
    }
}

/* Look up address in address book, if found return label (address)
   otherwise just return (address)
 */
QString TransactionTableModel::lookupAddress(const std::string &address, bool tooltip) const
{
    QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(address));
    QString description;
    if(!label.isEmpty())
    {
        description += label + QString(" ");
    }
    if(label.isEmpty() || walletModel->getOptionsModel()->getDisplayAddresses() || tooltip)
    {
        description += QString::fromStdString(address);
    }
    return description;
}

QString TransactionTableModel::formatTxType(const TransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case TransactionRecord::RecvWithAddress:
        return tr("Received");
    case TransactionRecord::RecvFromOther:
        return tr("Received");
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
        return tr("Sent");
    case TransactionRecord::InternalSend:
    case TransactionRecord::InternalReceive:
        return tr("Internal Transfer");
    case TransactionRecord::Generated:
        return tr("Interest");
    case TransactionRecord::Fee:
        return tr("Fee");
    default:
        return QString();
    }
}

QVariant TransactionTableModel::txAddressDecoration(const TransactionRecord *wtx) const
{
    switch(wtx->type)
    {
    case TransactionRecord::Generated:
        return QIcon(":/icons/tx_mined");
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::RecvFromOther:
        return QIcon(":/icons/tx_input");
    case TransactionRecord::SendToAddress:
    case TransactionRecord::SendToOther:
    case TransactionRecord::Fee:
        return QIcon(":/icons/tx_output");
    default:
        return QIcon(":/icons/tx_inout");
    }
    return QVariant();
}

QString TransactionTableModel::formatTxToAddress(const TransactionRecord *wtx, bool tooltip) const
{
    switch(wtx->type)
    {
    case TransactionRecord::RecvFromOther:
        return QString::fromStdString(wtx->address);
    case TransactionRecord::RecvWithAddress:
    case TransactionRecord::SendToAddress:
    case TransactionRecord::Generated:
    case TransactionRecord::SendToOther:
    case TransactionRecord::InternalSend:
    case TransactionRecord::InternalReceive:
        return lookupAddress(wtx->address, tooltip);
    case TransactionRecord::Fee:
    default:
        return tr("(n/a)");
    }
}


QString TransactionTableModel::formatTxFromAddress(const TransactionRecord *wtx, bool tooltip) const
{
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::SendToOther:
        case TransactionRecord::InternalSend:
        case TransactionRecord::InternalReceive:
        case TransactionRecord::Fee:
            return lookupAddress(wtx->fromAddress, tooltip);
        case TransactionRecord::RecvFromOther:
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::Generated:
        default:
            return tr("(n/a)");
    }
}

QString TransactionTableModel::formatTxOurAddress(const TransactionRecord *wtx, bool tooltip) const
{
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::SendToOther:
        case TransactionRecord::InternalSend:
        case TransactionRecord::Fee:
            return lookupAddress(wtx->fromAddress, tooltip);
        case TransactionRecord::RecvFromOther:
        case TransactionRecord::InternalReceive:
        case TransactionRecord::Generated:
        case TransactionRecord::RecvWithAddress:
            return lookupAddress(wtx->address, tooltip);
        default:
            return tr("(n/a)");
    }
}

QString TransactionTableModel::formatTxOtherAddress(const TransactionRecord *wtx, bool tooltip) const
{
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::SendToOther:
        case TransactionRecord::InternalSend:
            return lookupAddress(wtx->address, tooltip);
        case TransactionRecord::InternalReceive:
            return lookupAddress(wtx->fromAddress, tooltip);
        case TransactionRecord::RecvFromOther:
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::Generated:
        case TransactionRecord::Fee:
        default:
            return tr("(n/a)");
    }
}

QVariant TransactionTableModel::fromAddressColor(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::InternalSend:
        case TransactionRecord::InternalReceive:
        case TransactionRecord::Fee:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->fromAddress));
            if(label.isEmpty())
            return COLOR_BAREADDRESS;
        }
        break;
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::Generated:
            return COLOR_BAREADDRESS;
        default:
            break;
    }
    return QVariant();
}

QVariant TransactionTableModel::toAddressColor(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::SendToAddress:
        case TransactionRecord::Generated:
        case TransactionRecord::InternalSend:
        case TransactionRecord::InternalReceive:
        case TransactionRecord::Fee:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
            if(label.isEmpty())
                return COLOR_BAREADDRESS;
        }
        break;
        default:
            break;
    }
    return QVariant();
}


QVariant TransactionTableModel::ourAddressColor(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::InternalSend:
        case TransactionRecord::SendToOther:
        case TransactionRecord::Fee:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->fromAddress));
            if(label.isEmpty())
                return COLOR_BAREADDRESS;
        }
        break;
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::Generated:
        case TransactionRecord::InternalReceive:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
            if(label.isEmpty())
                return COLOR_BAREADDRESS;
        }
        break;
        default:
            break;
    }
    return QVariant();
}

QVariant TransactionTableModel::otherAddressColor(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::InternalSend:
        case TransactionRecord::SendToOther:
        case TransactionRecord::Fee:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
            if(label.isEmpty())
                return COLOR_BAREADDRESS;
        }
        break;
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::InternalReceive:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->fromAddress));
            if(label.isEmpty())
                return COLOR_BAREADDRESS;
        }
        break;
        case TransactionRecord::RecvFromOther:
        case TransactionRecord::Generated:
            return COLOR_BAREADDRESS;
        default:
            break;
    }
    return QVariant();
}

QVariant TransactionTableModel::fromAddressFont(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::InternalSend:
        case TransactionRecord::InternalReceive:
        case TransactionRecord::SendToOther:
        case TransactionRecord::Fee:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->fromAddress));
            if(!label.isEmpty())
            {
                QFont underlineFont;
                underlineFont.setUnderline(true);
                return underlineFont;
            }
        }
        default:
            break;
    }
    return QVariant();
}

QVariant TransactionTableModel::toAddressFont(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::InternalSend:
        case TransactionRecord::InternalReceive:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
            if(!label.isEmpty())
            {
                QFont underlineFont;
                underlineFont.setUnderline(true);
                return underlineFont;
            }
        }
        case TransactionRecord::Fee:
        default:
            break;
    }
    return QVariant();
}

QVariant TransactionTableModel::ourAddressFont(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::InternalSend:
        case TransactionRecord::SendToOther:
        case TransactionRecord::Fee:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->fromAddress));
            if(!label.isEmpty())
            {
                QFont underlineFont;
                underlineFont.setUnderline(true);
                return underlineFont;
            }
        }
        break;
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::InternalReceive:
        case TransactionRecord::Generated:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
            if(!label.isEmpty())
            {
                QFont underlineFont;
                underlineFont.setUnderline(true);
                return underlineFont;
            }
        }
        break;
        default:
            break;
    }
    return QVariant();
}

QVariant TransactionTableModel::otherAddressFont(const TransactionRecord *wtx) const
{
    // Show addresses without label in a less visible color
    switch(wtx->type)
    {
        case TransactionRecord::SendToAddress:
        case TransactionRecord::InternalSend:
        case TransactionRecord::SendToOther:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->address));
            if(!label.isEmpty())
            {
                QFont underlineFont;
                underlineFont.setUnderline(true);
                return underlineFont;
            }
        }
        break;
        case TransactionRecord::RecvWithAddress:
        case TransactionRecord::InternalReceive:
        {
            QString label = walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(wtx->fromAddress));
            if(!label.isEmpty())
            {
                QFont underlineFont;
                underlineFont.setUnderline(true);
                return underlineFont;
            }
        }
        break;
        case TransactionRecord::Generated:
        case TransactionRecord::Fee:
        default:
            break;
    }
    return QVariant();
}


QString TransactionTableModel::formatTxAmount(const TransactionRecord *wtx, bool showUnconfirmed) const
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();

    QString str;

    str = BitcoinUnits::formatWithUnit(unit, wtx->credit + wtx->debit, true, false);
    if(showUnconfirmed)
    {
        if(!wtx->status.countsForBalance)
        {
            str = QString("[") + str + QString("]");
        }
    }
    return QString(str);
}



QString TransactionTableModel::formatTxBalance(const TransactionRecord *wtx, int row, bool showUnconfirmed) const
{
    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    QString str = BitcoinUnits::formatWithUnit(unit, priv->getBalance(wtx, walletModel), true, false);
    return QString(str);
}

QVariant TransactionTableModel::txStatusDecoration(const TransactionRecord *wtx) const
{
    switch(wtx->status.status)
    {
    case TransactionStatus::OpenUntilBlock:
    case TransactionStatus::OpenUntilDate:
        return QColor(64,64,255);
    case TransactionStatus::Offline:
        return QColor(192,192,192);
    case TransactionStatus::Unconfirmed:
        return QIcon(":/icons/transaction_0");
    case TransactionStatus::Confirming:
        switch(wtx->status.depth)
        {
        case 1: return QIcon(":/icons/transaction_1");
        case 2: return QIcon(":/icons/transaction_2");
        case 3: return QIcon(":/icons/transaction_3");
        case 4: return QIcon(":/icons/transaction_4");
        default: return QIcon(":/icons/transaction_5");
        };
    case TransactionStatus::Confirmed:
        return QIcon(":/icons/transaction_confirmed");
    case TransactionStatus::Conflicted:
        return QIcon(":/icons/transaction_conflicted");
    case TransactionStatus::Immature: {
        int total = wtx->status.depth + wtx->status.matures_in;
        int part = (wtx->status.depth * 4 / total) + 1;
        return QIcon(QString(":/icons/transaction_%1").arg(part));
        }
    case TransactionStatus::MaturesWarning:
    case TransactionStatus::NotAccepted:
        return QIcon(":/icons/transaction_0");
    }
    return QColor(0,0,0);
}

QString TransactionTableModel::formatTooltip(const TransactionRecord *rec) const
{
    QString tooltip = formatTxStatus(rec) + QString("\n") + formatTxType(rec);
    if(rec->type==TransactionRecord::RecvFromOther || rec->type==TransactionRecord::SendToOther ||
       rec->type==TransactionRecord::SendToAddress || rec->type==TransactionRecord::RecvWithAddress)
    {
        tooltip += QString(" ") + formatTxToAddress(rec, true);
    }
    return tooltip;
}

QVariant TransactionTableModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    TransactionRecord *rec = static_cast<TransactionRecord*>(index.internalPointer());

    switch(role)
    {
    case Qt::DecorationRole:
        switch(index.column())
        {
        case Status:
            return txStatusDecoration(rec);
        case Type:
            return txAddressDecoration(rec);
        }
        break;
    case Qt::DisplayRole:
        switch(index.column())
        {
        case Date:
            return formatTxDate(rec);
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, false);
        case FromAddress:
            return formatTxFromAddress(rec, false);
        case OurAddress:
            return formatTxOurAddress(rec, false);
        case OtherAddress:
            return formatTxOtherAddress(rec, false);
        case Amount:
            return formatTxAmount(rec);
        case Balance:
            return formatTxBalance(rec, index.row());
        }
        break;
    case Qt::EditRole:
        // Edit role is used for sorting, so return the unformatted values
        switch(index.column())
        {
        case Status:
            return QString::fromStdString(rec->status.sortKey);
        case Date:
            return rec->time;
        case Type:
            return formatTxType(rec);
        case ToAddress:
            return formatTxToAddress(rec, true);
        case FromAddress:
            return formatTxFromAddress(rec, true);
        case OurAddress:
            return formatTxOurAddress(rec, true);
        case OtherAddress:
            return formatTxOtherAddress(rec, true);
        case Amount:
            return rec->credit + rec->debit;
        case Balance:
            return priv->getBalance(rec, walletModel);
        }
        break;
    case Qt::ToolTipRole:
        return formatTooltip(rec);
    case Qt::TextAlignmentRole:
        return column_alignments[index.column()];
    case Qt::ForegroundRole:
        // Non-confirmed (but not immature) as transactions are grey
        if(!rec->status.countsForBalance && rec->status.status != TransactionStatus::Immature)
        {
            return COLOR_UNCONFIRMED;
        }
        else if(index.column() == Amount && (rec->credit+rec->debit) < 0)
        {
            return COLOR_NEGATIVE;
        }
        else if(index.column() == ToAddress)
        {
            return toAddressColor(rec);
        }
        else if(index.column() == FromAddress)
        {
            return fromAddressColor(rec);
        }
        else if(index.column() == OurAddress)
        {
            return ourAddressColor(rec);
        }
        else if(index.column() == OtherAddress)
        {
            return otherAddressColor(rec);
        }
        break;
    case TypeRole:
        return rec->type;
    case DateRole:
        return QDateTime::fromTime_t(static_cast<uint>(rec->time));
    case LongDescriptionRole:
        return priv->describe(rec);
    case AddressRole:
        return QString::fromStdString(rec->address);
    case FromAddressRole:
        return QString::fromStdString(rec->fromAddress);
    case LabelRole:
        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->address));
    case FromLabelRole:
        return walletModel->getAddressTableModel()->labelForAddress(QString::fromStdString(rec->fromAddress));
    case AmountRole:
        return rec->credit + rec->debit;
    case TxIDRole:
        return QString::fromStdString(rec->getTxID());
    case ConfirmedRole:
        return rec->status.countsForBalance;
    case FormattedAmountRole:
        return formatTxAmount(rec, false);
    case StatusRole:
        return rec->status.status;
    case Qt::FontRole:
        switch(index.column())
        {
            case FromAddress:
                return fromAddressFont(rec);
            case ToAddress:
                return toAddressFont(rec);
            case OurAddress:
                return ourAddressFont(rec);
            case OtherAddress:
                return otherAddressFont(rec);
        }
    }
    return QVariant();
}

QVariant TransactionTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(section < 0)
        return QVariant();

    if(orientation == Qt::Horizontal)
    {
        if(role == Qt::DisplayRole)
        {
            return columns[section];
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return column_header_alignments[section];
        }
        else if (role == Qt::ToolTipRole)
        {
            switch(section)
            {
            case Status:
                return tr("Transaction status. Hover over this field to show number of confirmations.");
            case Date:
                return tr("Date and time that the transaction was received.");
            case Type:
                return tr("Type of transaction.");
            case ToAddress:
                return tr("Destination account of transaction.");
            case FromAddress:
                return tr("Source account of transaction.");
            case OurAddress:
                return tr("Account for transaction.");
            case OtherAddress:
                return tr("Other account for transaction.");
            case Amount:
                return tr("Amount removed from or added to balance.");
            case Balance:
                return tr("Account balance at end of transaction.");
            }
        }
    }
    return QVariant();
}

QModelIndex TransactionTableModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    TransactionRecord *data = priv->index(row);
    if(data)
    {
        return createIndex(row, column, priv->index(row));
    }
    else
    {
        return QModelIndex();
    }
}

void TransactionTableModel::setCurrentAccountPrefix(QString prefix)
{
    priv->setCurrentAccountPrefix(prefix);
}

int64_t TransactionTableModel::getInterestGenerated(QString forAddress)
{
    //checkme: Paranoid? Not sure if entirely necessary.
    LOCK(wallet->cs_wallet);

    int64_t ret = 0;
    for(int i=0; i < priv->size(); i++)
    {
        if(priv->cachedWallet[i].type == TransactionRecord::Generated)
        {
            if(priv->cachedWallet[i].status.status == TransactionStatus::Confirmed)
            {
                if(forAddress.isEmpty() || forAddress == priv->cachedWallet[i].address.c_str())
                {
                    ret += priv->cachedWallet[i].credit - priv->cachedWallet[i].debit;
                }
            }
        }
    }
    return ret;
}

void TransactionTableModel::updateDisplayUnit()
{
    // emit dataChanged to update Amount column with the current unit
    emit dataChanged(index(0, Amount), index(priv->size()-1, Amount));
}
