#ifndef WALLETMODEL_H
#define WALLETMODEL_H

#ifndef HEADLESS
#include <QObject>
#endif

#include <vector>
#include <map>
#include "allocators.h" /* for SecureString */

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class AccountModel;
class CWallet;
class CKeyID;
class CPubKey;
class COutput;
class COutPoint;
class uint256;
class CCoinControl;

class SendCoinsRecipient
{
public:
    SendCoinsRecipient(double amt, std::string addr, std::string lbl)
    : address(addr)
    , label(lbl)
    , amount(amt)\
    {
    }
    SendCoinsRecipient()
    {
    }
    std::string address;
    std::string label;
    int64_t amount;
};

#ifndef HEADLESS
QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE
#endif

/** Interface to Bitcoin wallet from Qt view code. */
class WalletModel
#ifndef HEADLESS
: public QObject
#endif
{

#ifndef HEADLESS
    Q_OBJECT
#endif

public:
#ifdef HEADLESS
    explicit WalletModel(CWallet *wallet);
#else
    explicit WalletModel(CWallet *wallet, OptionsModel *optionsModel, QObject *parent = 0);
#endif
    ~WalletModel();

    enum StatusCode // Returned by sendCoins
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        DuplicateAddress,
        TransactionTooBig,
        TransactionCreationFailed, // Error returned when wallet is still locked
        TransactionCommitFailed,
        Aborted
    };

    enum EncryptionStatus
    {
        Unencrypted,  // !wallet->IsCrypted()
        Locked,       // wallet->IsCrypted() && wallet->IsLocked()
        Unlocked      // wallet->IsCrypted() && !wallet->IsLocked()
    };

#ifndef HEADLESS
    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    AccountModel* getExternalAccountModel();
    AccountModel* getMyAccountModel();
    AccountModel* getAllAccountModel();
    TransactionTableModel *getTransactionTableModel();

    qint64 getBalance() const;
    qint64 getStake() const;
    qint64 getUnconfirmedBalance() const;
    qint64 getImmatureBalance() const;
    int getNumTransactions() const;
    EncryptionStatus getEncryptionStatus() const;


    // Wallet encryption
    bool setWalletEncrypted(bool encrypted, const SecureString &passphrase);
    // Passphrase only needed when unlocking
    bool setWalletLocked(bool locked, const SecureString &passPhrase=SecureString());
    bool changePassphrase(const SecureString &oldPass, const SecureString &newPass);
    // Wallet backup
    bool backupWallet(const QString &filename);

    // RAI object for unlocking wallet, returned by requestUnlock()
    class UnlockContext
    {
    public:
        UnlockContext(WalletModel *wallet, bool valid, bool relock, bool wasUnlockedForStaking);
        ~UnlockContext();

        bool isValid() const { return valid; }

        // Copy operator and constructor transfer the context
        UnlockContext(const UnlockContext& obj) { CopyFrom(obj); }
        UnlockContext& operator=(const UnlockContext& rhs) { CopyFrom(rhs); return *this; }
    private:
        WalletModel *wallet;
        bool valid;
        mutable bool relock; // mutable, as it can be set to false by copying
        bool wasUnlockedForStaking;

        void CopyFrom(const UnlockContext& rhs);
    };

    UnlockContext requestUnlock();
#endif

    // Check address for validity
    bool validateAddress(const std::string &address);
    bool getPubKey(const CKeyID &address, CPubKey& vchPubKeyOut) const;
    void getOutputs(const std::vector<COutPoint>& vOutpoints, std::vector<COutput>& vOutputs);
    void listCoins(std::map<std::string, std::vector<COutput> >& mapCoins) const;
    bool isLockedCoin(uint256 hash, unsigned int n) const;
    void lockCoin(COutPoint& output);
    void unlockCoin(COutPoint& output);
    void listLockedCoins(std::vector<COutPoint>& vOutpts);

    // Return status record for SendCoins, contains error id + information
    struct SendCoinsReturn
    {
        SendCoinsReturn(StatusCode status=Aborted,
                         int64_t fee=0,
                         std::string hex=""):
            status(status), fee(fee), hex(hex) {}
        StatusCode status;
        int64_t fee; // is used in case status is "AmountWithFeeExceedsBalance"
        std::string hex; // is filled with the transaction hash if status is "OK"
    };

    // Send coins to a list of recipients
    SendCoinsReturn sendCoins(const std::vector<SendCoinsRecipient> &recipients, const CCoinControl *coinControl=NULL, bool promptForFee = true);

    CWallet *getWallet();

private:
    CWallet *wallet;

#ifndef HEADLESS
    // Wallet has an options model for wallet-specific options
    // (transaction fee, for example)
    OptionsModel *optionsModel;

    AddressTableModel *addressTableModel;

    TransactionTableModel *transactionTableModel;

    AccountModel* externalAccountModel;
    AccountModel* myAccountModel;
    AccountModel* allAccountModel;

    // Cache some values to be able to detect changes
    qint64 cachedBalance;
    qint64 cachedStake;
    qint64 cachedUnconfirmedBalance;
    qint64 cachedImmatureBalance;
    qint64 cachedNumTransactions;
    EncryptionStatus cachedEncryptionStatus;
    int cachedNumBlocks;

    QTimer *pollTimer;


    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
#endif
    void checkBalanceChanged();

#ifndef HEADLESS
public slots:
    /* Wallet status might have changed */
    void updateStatus();
    /* New transaction, or transaction changed status */
    void updateTransaction(const QString &hash, int status);
    /* New, updated or removed address book entry */
    void updateAddressBook(const QString &address, const QString &label, bool isMine, int status);
    /* Current, immature or unconfirmed balance might have changed - emit 'balanceChanged' if so */
    void pollBalanceChanged();

signals:
    // Signal that balance in wallet changed
    void balanceChanged(qint64 balance, qint64 stake, qint64 unconfirmedBalance, qint64 immatureBalance);

    // Number of transactions in wallet changed
    void numTransactionsChanged(int count);

    // Encryption status of wallet changed
    void encryptionStatusChanged(int status);

    // Signal emitted when wallet needs to be unlocked
    // It is valid behaviour for listeners to keep the wallet locked after this signal;
    // this means that the unlocking failed or was cancelled.
    void requireUnlock();

    // Signal emitted when address book has been updated
    void addressBookUpdated();

    // Asynchronous error notification
    void error(const QString &title, const QString &message, bool modal);
#endif
};

#endif // WALLETMODEL_H
