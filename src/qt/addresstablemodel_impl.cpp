#include "addresstablemodel_impl.h"
#include "wallet.h"
#include <QList>
#include "guiutil.h"
#include "walletmodel.h"
#include "wallet.h"
#include "base58.h"
#include <QFont>
#include <QColor>
#include <QAbstractTableModel>

struct AddressTableEntryLessThan
{
    bool operator()(const AddressTableEntry &a, const AddressTableEntry &b) const
    {
        return a.address < b.address;
    }
    bool operator()(const AddressTableEntry &a, const QString &b) const
    {
        return a.address < b;
    }
    bool operator()(const QString &a, const AddressTableEntry &b) const
    {
        return a < b.address;
    }
};


AddressTable_impl::AddressTable_impl(CWallet *wallet, QAbstractAddressTableModel *parent, bool includeExternalAccounts_, bool includeMyAccounts_)
: wallet(wallet)
, parent(parent)
, includeExternalAccounts(includeExternalAccounts_)
, includeMyAccounts(includeMyAccounts_)
{

}

void AddressTable_impl::refreshAddressTable()
{
    cachedAddressTable.clear();
    {
        LOCK(wallet->cs_wallet);
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& item, wallet->mapAddressBook)
        {
            const CBitcoinAddress& address = item.first;
            const std::string& strName = item.second.name;
            bool fMine = IsMine(*wallet, address.Get());
            if( (includeExternalAccounts && !fMine) || (includeMyAccounts && fMine) )
            {
                cachedAddressTable.append(AddressTableEntry(fMine ? AddressTableEntry::Receiving : AddressTableEntry::Sending,
                              item.first,
                              QString::fromStdString(strName),
                              QString::fromStdString(address.ToString())));
            }
        }
    }
    // qLowerBound() and qUpperBound() require our cachedAddressTable list to be sorted in asc order
    qSort(cachedAddressTable.begin(), cachedAddressTable.end(), AddressTableEntryLessThan());
}

void AddressTable_impl::updateEntry(const QString &address, const QString &label, bool isMine, int status)
{
    {
        LOCK(wallet->cs_wallet);
        CBitcoinAddress baddress(address.toStdString());
        bool fMine = IsMine(*wallet, baddress.Get());
        if(fMine)
        {
            if(!includeMyAccounts)
            {
                return;
            }
        }
        else
        {
            if(!includeExternalAccounts)
            {
                return;
            }
        }
    }

    // Find address / label in model
    QList<AddressTableEntry>::iterator lower = qLowerBound(
        cachedAddressTable.begin(), cachedAddressTable.end(), address, AddressTableEntryLessThan());
    QList<AddressTableEntry>::iterator upper = qUpperBound(
        cachedAddressTable.begin(), cachedAddressTable.end(), address, AddressTableEntryLessThan());
    int lowerIndex = (lower - cachedAddressTable.begin());
    int upperIndex = (upper - cachedAddressTable.begin());
    bool inModel = (lower != upper);
    AddressTableEntry::Type newEntryType = isMine ? AddressTableEntry::Receiving : AddressTableEntry::Sending;

    switch(status)
    {
    case CT_NEW:
        if(inModel)
        {
            OutputDebugStringF("Warning: AddressTablePriv::updateEntry: Got CT_NEW, but entry is already in model\n");
            break;
        }
        parent->beginInsertRows(QModelIndex(), lowerIndex, lowerIndex);
        cachedAddressTable.insert(lowerIndex, AddressTableEntry(newEntryType, label, address));
        parent->endInsertRows();
        break;
    case CT_UPDATED:
        if(!inModel)
        {
            OutputDebugStringF("Warning: AddressTablePriv::updateEntry: Got CT_UPDATED, but entry is not in model\n");
            break;
        }
        lower->type = newEntryType;
        lower->label = label;
        parent->emitDataChanged(lowerIndex);
        break;
    case CT_DELETED:
        if(!inModel)
        {
            OutputDebugStringF("Warning: AddressTablePriv::updateEntry: Got CT_DELETED, but entry is not in model\n");
            break;
        }
        parent->beginRemoveRows(QModelIndex(), lowerIndex, upperIndex-1);
        cachedAddressTable.erase(lower, upper);
        parent->endRemoveRows();
        break;
    }
}

int AddressTable_impl::size()
{
    return cachedAddressTable.size();
}

QString AddressTable_impl::getLabel(int idx)
{
    if(idx >= 0 && idx < cachedAddressTable.size())
    {
        return cachedAddressTable[idx].label;
    }
    else
    {
        return "";
    }
}

QString AddressTable_impl::getAddress(int idx)
{
    if(idx >= 0 && idx < cachedAddressTable.size())
    {
        return cachedAddressTable[idx].address;
    }
    else
    {
        return "";
    }
}

qint64 AddressTable_impl::getBalance(int idx)
{
    if(idx >= 0 && idx < cachedAddressTable.size())
    {
        int64_t accountInterest = 0;
        int64_t accountBalance = 0;
        int64_t accountPending = 0;
        wallet->GetBalanceForAddress(cachedAddressTable[idx].address.toStdString(), accountInterest, accountPending, accountBalance);
        //checkme:
        return accountInterest + accountBalance + accountPending;
    }
    return 0;
}

AddressTableEntry *AddressTable_impl::index(int idx)
{
    if(idx >= 0 && idx < cachedAddressTable.size())
    {
        return &cachedAddressTable[idx];
    }
    else
    {
        return 0;
    }
}

