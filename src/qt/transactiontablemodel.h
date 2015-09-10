#ifndef TRANSACTIONTABLEMODEL_H
#define TRANSACTIONTABLEMODEL_H

#include <QAbstractTableModel>
#include <QStringList>
#ifdef __MINGW32__
#include <stdint.h>
#endif

class CWallet;
class TransactionTablePriv;
class TransactionRecord;
class WalletModel;

/** UI model for the transaction table of a wallet.
 */
class TransactionTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit TransactionTableModel(CWallet* wallet, WalletModel *parent = 0);
    ~TransactionTableModel();

    enum ColumnIndex {
        Status = 0,
        Date = 1,
        Type = 2,
        FromAddress = 3, // Sender address.
        ToAddress = 4, // Recipient address.
        OurAddress = 5, // Relevant address i.e. automatically picks sender or receiver based on transaction type.
        OtherAddress = 6, // Opposite of 'OurAddress' - get the 'other' side of the transaction i.e. receiver if a send transaction etc.
        Amount = 7,
        Balance = 8
    };

    /** Roles to get specific information from a transaction row.
        These are independent of column.
    */
    enum RoleIndex {
        /** Type of transaction */
        TypeRole = Qt::UserRole,
        /** Date and time this transaction was created */
        DateRole,
        /** Long description (HTML format) */
        LongDescriptionRole,
        /** Address of transaction */
        FromAddressRole,
        /** Address of transaction */
        AddressRole,
        /** Label of transaction */
        LabelRole,
        /** Label of address related to transaction */
        FromLabelRole,
        /** Net amount of transaction */
        AmountRole,
        /** Unique identifier */
        TxIDRole,
        /** Is transaction confirmed? */
        ConfirmedRole,
        /** Formatted amount, without brackets when unconfirmed */
        FormattedAmountRole,
        /** Transaction status (TransactionRecord::Status) */
        StatusRole
    };

    int rowCount(const QModelIndex &parent) const;
    int columnCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    QModelIndex index(int row, int column, const QModelIndex & parent = QModelIndex()) const;
    void setCurrentAccountPrefix(QString prefix);

    //Get total interest gained for address - leave blank to get total of all addresses
    int64_t getInterestGenerated(QString forAddress="");

private:
    CWallet* wallet;
    WalletModel *walletModel;
    QStringList columns;
    TransactionTablePriv *priv;
    int cachedNumBlocks;

    QString lookupAddress(const std::string &address, bool tooltip) const;
    QVariant toAddressColor(const TransactionRecord *wtx) const;
    QVariant fromAddressColor(const TransactionRecord *wtx) const;
    QVariant ourAddressColor(const TransactionRecord *wtx) const;
    QVariant otherAddressColor(const TransactionRecord *wtx) const;
    QVariant fromAddressFont(const TransactionRecord *wtx) const;
    QVariant toAddressFont(const TransactionRecord *wtx) const;
    QVariant ourAddressFont(const TransactionRecord *wtx) const;
    QVariant otherAddressFont(const TransactionRecord *wtx) const;
    QString formatTxStatus(const TransactionRecord *wtx) const;
    QString formatTxDate(const TransactionRecord *wtx) const;
    QString formatTxType(const TransactionRecord *wtx) const;
    QString formatTxToAddress(const TransactionRecord *wtx, bool tooltip) const;
    QString formatTxFromAddress(const TransactionRecord *wtx, bool tooltip) const;
    QString formatTxOurAddress(const TransactionRecord *wtx, bool tooltip) const;
    QString formatTxOtherAddress(const TransactionRecord *wtx, bool tooltip) const;
    QString formatTxAmount(const TransactionRecord *wtx, bool showUnconfirmed=true) const;
    QString formatTxBalance(const TransactionRecord *wtx, int row, bool showUnconfirmed=true) const;
    QString formatTooltip(const TransactionRecord *rec) const;
    QVariant txStatusDecoration(const TransactionRecord *wtx) const;
    QVariant txAddressDecoration(const TransactionRecord *wtx) const;

public slots:
    void updateTransaction(const QString &hash, int status);
    void updateConfirmations();
    void updateDisplayUnit();

    friend class TransactionTablePriv;
};

#endif

