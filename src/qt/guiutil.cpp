// Copyright (c) 2011-2013 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "guiutil.h"
#include "guiconstants.h"
#include "walletmodel.h"
#include "coincontrol.h"
#include "util.h"
#include "init.h"
#include "bitcoinrpc.h"

#ifndef HEADLESS
#include "coincontroldialog.h"
#include "bitcoinunits.h"
#include "optionsmodel.h"
#include "bitcoinaddressvalidator.h"
#include <QString>
#include <QDateTime>
#include <QDoubleValidator>
#include <QFont>
#include <QLineEdit>
#include <QUrl>
#include <QTextDocument> // For Qt::escape
#include <QAbstractItemView>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QDesktopServices>
#include <QThread>
#include <QTranslator>
#include <QLocale>
#endif

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#ifdef WIN32
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0501
#ifdef _WIN32_IE
#undef _WIN32_IE
#endif
#define _WIN32_IE 0x0501
#define WIN32_LEAN_AND_MEAN 1
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "shlwapi.h"
#include "shlobj.h"
#include "shellapi.h"
#endif

namespace GUIUtil {

#ifndef HEADLESS
bool IsAmountStringValid(int currentUnit, const QString &amountString)
{
    bool valid = true;
    if (amountString.toDouble() == 0.0)
        valid = false;
    if (valid && !BitcoinUnits::parse(currentUnit, amountString, 0))
        valid = false;

    return valid;
}

qint64 AmountStringToBitcoinUnits(int currentUnit, const QString &amountString)
{
    qint64 ret;
    BitcoinUnits::parse(currentUnit, amountString, &ret);
    return ret;
}

void SetControlValidity(QWidget* widget, bool valid)
{
    if (valid)
        widget->setStyleSheet("");
    else
        widget->setStyleSheet(STYLE_INVALID);
}
#endif

#ifdef HEADLESS
bool SendCoinsHelper(const std::vector<SendCoinsRecipient>& recipients, WalletModel* model, const std::string& sendAccountAddress, bool ignoreCoinControl, std::string& transactionHash)
#else
bool SendCoinsHelper(QWidget* parent, const std::vector<SendCoinsRecipient>& recipients, WalletModel* model, const std::string& sendAccountAddress, bool ignoreCoinControl, std::string& transactionHash)
#endif
{
    #ifndef HEADLESS
    // If parent is NULL then no UI.
    if(parent)
    {
        // Format confirmation message
        QStringList formatted;
        foreach(const SendCoinsRecipient &rcp, recipients)
        {
            formatted.append(QObject::tr("<b>%1</b> to %2 (%3)").arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, rcp.amount, true, false), Qt::escape(rcp.label.c_str()), rcp.address.c_str()));
        }

        QMessageBox::StandardButton retval = QMessageBox::question(parent, QObject::tr("Confirm send coins"), QObject::tr("Are you sure you want to send %1?").arg(formatted.join(QObject::tr(" and "))), QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
        if(retval != QMessageBox::Yes)
        {
            return false;
        }
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return false;
    }
    #endif

    WalletModel::SendCoinsReturn sendstatus;
#ifndef HEADLESS
    if (ignoreCoinControl || !model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
#endif
    {
        CCoinControl control;

        map<std::string, vector<COutput> > mapCoins;
        model->listCoins(mapCoins);
        BOOST_FOREACH(PAIRTYPE(std::string, vector<COutput>) coins, mapCoins)
        {
            std::string sWalletAddress = coins.first;
            if(sWalletAddress == sendAccountAddress)
            {
                BOOST_FOREACH(const COutput& out, coins.second)
                {
                    CTxDestination outputAddress;
                    std::string sAddress = "";
                    if(ExtractDestination(out.tx->vout[out.i].scriptPubKey, outputAddress))
                    {
                        sAddress = CBitcoinAddress(outputAddress).ToString().c_str();
                    }

                    // transaction hash
                    COutPoint outpt(out.tx->GetHash(), out.i);
                    control.Select(outpt);
                }
            }
        }
#ifdef HEADLESS
        sendstatus = model->sendCoins(recipients, &control);
#else
        sendstatus = model->sendCoins(recipients, &control, (parent != NULL));
#endif
    }
#ifndef HEADLESS
    else
    {
        sendstatus = model->sendCoins(recipients, CoinControlDialog::coinControl, (parent != NULL));
    }
#endif

    switch(sendstatus.status)
    {
    case WalletModel::InvalidAddress:
#ifndef HEADLESS
        if(parent)
        {
            QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("The recipient address is not valid, please recheck."), QMessageBox::Ok, QMessageBox::Ok);
        }
        else
#endif
        {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "The recipient address is not valid, please recheck.");
        }
        break;
    case WalletModel::InvalidAmount:
#ifndef HEADLESS
        if(parent)
        {
            QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("The amount to pay must be larger than 0."), QMessageBox::Ok, QMessageBox::Ok);
        }
        else
#endif
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "The amount to pay must be larger than 0.");
        }
        break;
    case WalletModel::AmountExceedsBalance:
#ifndef HEADLESS
        if(parent)
        {
            if (model->getUnconfirmedBalance() > 0)
            {
                QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("The amount exceeds your balance."), QMessageBox::Ok, QMessageBox::Ok);
            }
            else
            {
                QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("The amount exceeds your available balance, some of your Pandacoins are currently being used to earn you interest."), QMessageBox::Ok, QMessageBox::Ok);
            }
        }
        else
#endif
        {
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "The amount exceeds your balance.");
        }
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
#ifndef HEADLESS
        if(parent)
        {
            if (model->getUnconfirmedBalance() > 0)
            {
                QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("The total exceeds your balance when the %1 transaction fee is included."). arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, sendstatus.fee, true, false)), QMessageBox::Ok, QMessageBox::Ok);
            }
            else
            {
                QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("The amount exceeds your available balance when the %1 transaction fee is included, some of your Pandacoins are currently being used to earn you interest."). arg(BitcoinUnits::formatWithUnit(BitcoinUnits::BTC, sendstatus.fee, true, false)), QMessageBox::Ok, QMessageBox::Ok);
            }
        }
        else
#endif
        {
            throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "The amount exceeds your balance when the transaction fee is added.");
        }
        break;
    case WalletModel::DuplicateAddress:
#ifndef HEADLESS
        if(parent)
        {
            QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("Duplicate address found, can only send to each address once per send operation."), QMessageBox::Ok, QMessageBox::Ok);
        }
        else
#endif
        {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Duplicate address found, can only send to each address once per send operation.");
        }
        break;
    case WalletModel::TransactionTooBig:
#ifndef HEADLESS
        if(parent)
        {
            QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("Error: Transaction creation failed because transaction size (in Kb) too large."), QMessageBox::Ok, QMessageBox::Ok);
        }
        else
#endif
        {
            throw JSONRPCError(RPC_WALLET_ERROR, "Error: Transaction creation failed because transaction size (in Kb) too large.");
        }
        break;
    case WalletModel::TransactionCreationFailed:
#ifndef HEADLESS
        if(parent)
        {
            QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("Error: Transaction creation failed."), QMessageBox::Ok, QMessageBox::Ok);
        }
        else
#endif
        {
            throw JSONRPCError(RPC_WALLET_ERROR, "Error: Transaction creation failed.");
        }
        break;
    case WalletModel::TransactionCommitFailed:
#ifndef HEADLESS
        if(parent)
        {
            QMessageBox::warning(parent, QObject::tr("Send Coins"), QObject::tr("Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here."), QMessageBox::Ok, QMessageBox::Ok);
        }
        else
#endif
        {
            throw JSONRPCError(RPC_WALLET_ERROR, "Error: The transaction was rejected. This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        }
        break;
    case WalletModel::Aborted: // User aborted, nothing to do
        break;
    case WalletModel::OK:
        transactionHash = sendstatus.hex;
        return true;
    }
    return false;
}

#ifndef HEADLESS
QString dateTimeStr(const QDateTime &date)
{
    return date.date().toString(Qt::SystemLocaleShortDate) + QString(" ") + date.toString("hh:mm");
}

QString dateTimeStr(qint64 nTime)
{
    return QDateTime::fromTime_t((qint32)nTime).toString("d/M/yyyy hh:mm");
}

QDateTime dateFromString(const QString &dateString)
{
    return QDateTime::fromString(dateString,"d/M/yyyy hh:mm");
}

QFont bitcoinAddressFont()
{
    QFont font("Monospace");
#if QT_VERSION >= 0x040800
    font.setStyleHint(QFont::Monospace);
#else
    font.setStyleHint(QFont::TypeWriter);
#endif
    return font;
}

void setupAddressWidget(QLineEdit *widget, QWidget *parent)
{
    widget->setMaxLength(BitcoinAddressValidator::MaxAddressLength);
    widget->setValidator(new BitcoinAddressValidator(parent));
    widget->setFont(bitcoinAddressFont());
}

void setupAmountWidget(QLineEdit *widget, QWidget *parent)
{
    QDoubleValidator *amountValidator = new QDoubleValidator(parent);
    amountValidator->setDecimals(8);
    amountValidator->setBottom(0.0);
    widget->setValidator(amountValidator);
    widget->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
}

bool parseBitcoinURI(const QUrl &uri, SendCoinsRecipient *out)
{
    // NovaCoin: check prefix
    if(uri.scheme() != QString("pandacoin"))
        return false;

    SendCoinsRecipient rv;
    rv.address = uri.path().toStdString();
    rv.amount = 0;
    QList<QPair<QString, QString> > items = uri.queryItems();
    for (QList<QPair<QString, QString> >::iterator i = items.begin(); i != items.end(); i++)
    {
        bool fShouldReturnFalse = false;
        if (i->first.startsWith("req-"))
        {
            i->first.remove(0, 4);
            fShouldReturnFalse = true;
        }

        if (i->first == "label")
        {
            rv.label = i->second.toStdString();
            fShouldReturnFalse = false;
        }
        else if (i->first == "amount")
        {
            if(!i->second.isEmpty())
            {
                if(!BitcoinUnits::parse(BitcoinUnits::BTC, i->second, (qint64*)&rv.amount))
                {
                    return false;
                }
            }
            fShouldReturnFalse = false;
        }

        if (fShouldReturnFalse)
            return false;
    }
    if(out)
    {
        *out = rv;
    }
    return true;
}

bool parseBitcoinURI(QString uri, SendCoinsRecipient *out)
{
    // Convert pandacoin:// to pandacoin:
    //
    //    Cannot handle this later, because bitcoin:// will cause Qt to see the part after // as host,
    //    which will lower-case it (and thus invalidate the address).
    if(uri.startsWith("pandacoin://"))
    {
        uri.replace(0, 12, "pandacoin:");
    }
    QUrl uriInstance(uri);
    return parseBitcoinURI(uriInstance, out);
}

QString HtmlEscape(const QString& str, bool fMultiLine)
{
    QString escaped = Qt::escape(str);
    if(fMultiLine)
    {
        escaped = escaped.replace("\n", "<br>\n");
    }
    return escaped;
}

QString HtmlEscape(const std::string& str, bool fMultiLine)
{
    return HtmlEscape(QString::fromStdString(str), fMultiLine);
}

void copyEntryData(QAbstractItemView *view, int row, int column, int role)
{
    QApplication::clipboard()->setText(view->model()->index(row, column).data(role).toString());
}

QString getSaveFileName(QWidget *parent, const QString &caption,
                                 const QString &dir,
                                 const QString &filter,
                                 QString *selectedSuffixOut)
{
    QString selectedFilter;
    QString myDir;
    if(dir.isEmpty()) // Default to user documents location
    {
        myDir = QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation);
    }
    else
    {
        myDir = dir;
    }
    //We purposefully don't pass a parent here to avoid stylesheet propagating - users will except a system default file dialog.
    QString result = QFileDialog::getSaveFileName(NULL, caption, myDir, filter, &selectedFilter);

    /* Extract first suffix from filter pattern "Description (*.foo)" or "Description (*.foo *.bar ...) */
    QRegExp filter_re(".* \\(\\*\\.(.*)[ \\)]");
    QString selectedSuffix;
    if(filter_re.exactMatch(selectedFilter))
    {
        selectedSuffix = filter_re.cap(1);
    }

    /* Add suffix if needed */
    QFileInfo info(result);
    if(!result.isEmpty())
    {
        if(info.suffix().isEmpty() && !selectedSuffix.isEmpty())
        {
            /* No suffix specified, add selected suffix */
            if(!result.endsWith("."))
                result.append(".");
            result.append(selectedSuffix);
        }
    }

    /* Return selected suffix if asked to */
    if(selectedSuffixOut)
    {
        *selectedSuffixOut = selectedSuffix;
    }
    return result;
}

Qt::ConnectionType blockingGUIThreadConnection()
{
    if(QThread::currentThread() != QCoreApplication::instance()->thread())
    {
        return Qt::BlockingQueuedConnection;
    }
    else
    {
        return Qt::DirectConnection;
    }
}

bool checkPoint(const QPoint &p, const QWidget *w)
{
    QWidget *atW = qApp->widgetAt(w->mapToGlobal(p));
    if (!atW) return false;
    return atW->topLevelWidget() == w;
}

bool isObscured(QWidget *w)
{
    return !(checkPoint(QPoint(0, 0), w)
        && checkPoint(QPoint(w->width() - 1, 0), w)
        && checkPoint(QPoint(0, w->height() - 1), w)
        && checkPoint(QPoint(w->width() - 1, w->height() - 1), w)
        && checkPoint(QPoint(w->width() / 2, w->height() / 2), w));
}

void openDebugLogfile()
{
    boost::filesystem::path pathDebug = GetDataDir() / "debug.log";

    /* Open debug.log with the associated application */
    if (boost::filesystem::exists(pathDebug))
        QDesktopServices::openUrl(QUrl::fromLocalFile(QString::fromStdString(pathDebug.string())));
}

ToolTipToRichTextFilter::ToolTipToRichTextFilter(int size_threshold, QObject *parent) :
    QObject(parent), size_threshold(size_threshold)
{

}

bool ToolTipToRichTextFilter::eventFilter(QObject *obj, QEvent *evt)
{
    if(evt->type() == QEvent::ToolTipChange)
    {
        QWidget *widget = static_cast<QWidget*>(obj);
        QString tooltip = widget->toolTip();
        if(tooltip.size() > size_threshold && !tooltip.startsWith("<qt>") && !Qt::mightBeRichText(tooltip))
        {
            // Prefix <qt/> to make sure Qt detects this as rich text
            // Escape the current message as HTML and replace \n by <br>
            tooltip = "<qt>" + HtmlEscape(tooltip, true) + "<qt/>";
            widget->setToolTip(tooltip);
            return true;
        }
    }
    return QObject::eventFilter(obj, evt);
}

#ifdef WIN32
boost::filesystem::path static StartupShortcutPath()
{
    return GetSpecialFolderPath(CSIDL_STARTUP) / "Pandacoin.lnk";
}

bool GetStartOnSystemStartup()
{
    // check for Bitcoin.lnk
    return boost::filesystem::exists(StartupShortcutPath());
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    // If the shortcut exists already, remove it for updating
    boost::filesystem::remove(StartupShortcutPath());

    if (fAutoStart)
    {
        CoInitialize(NULL);

        // Get a pointer to the IShellLink interface.
        IShellLink* psl = NULL;
        HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL,
                                CLSCTX_INPROC_SERVER, IID_IShellLink,
                                reinterpret_cast<void**>(&psl));

        if (SUCCEEDED(hres))
        {
            // Get the current executable path
            TCHAR pszExePath[MAX_PATH];
            GetModuleFileName(NULL, pszExePath, sizeof(pszExePath));

            TCHAR pszArgs[5] = TEXT("-min");

            // Set the path to the shortcut target
            psl->SetPath(pszExePath);
            PathRemoveFileSpec(pszExePath);
            psl->SetWorkingDirectory(pszExePath);
            psl->SetShowCmd(SW_SHOWMINNOACTIVE);
            psl->SetArguments(pszArgs);

            // Query IShellLink for the IPersistFile interface for
            // saving the shortcut in persistent storage.
            IPersistFile* ppf = NULL;
            hres = psl->QueryInterface(IID_IPersistFile,
                                       reinterpret_cast<void**>(&ppf));
            if (SUCCEEDED(hres))
            {
                WCHAR pwsz[MAX_PATH];
                // Ensure that the string is ANSI.
                MultiByteToWideChar(CP_ACP, 0, StartupShortcutPath().string().c_str(), -1, pwsz, MAX_PATH);
                // Save the link by calling IPersistFile::Save.
                hres = ppf->Save(pwsz, TRUE);
                ppf->Release();
                psl->Release();
                CoUninitialize();
                return true;
            }
            psl->Release();
        }
        CoUninitialize();
        return false;
    }
    return true;
}

#elif defined(LINUX)

// Follow the Desktop Application Autostart Spec:
//  http://standards.freedesktop.org/autostart-spec/autostart-spec-latest.html

boost::filesystem::path static GetAutostartDir()
{
    namespace fs = boost::filesystem;

    char* pszConfigHome = getenv("XDG_CONFIG_HOME");
    if (pszConfigHome) return fs::path(pszConfigHome) / "autostart";
    char* pszHome = getenv("HOME");
    if (pszHome) return fs::path(pszHome) / ".config" / "autostart";
    return fs::path();
}

boost::filesystem::path static GetAutostartFilePath()
{
    return GetAutostartDir() / "pandacoin.desktop";
}

bool GetStartOnSystemStartup()
{
    boost::filesystem::ifstream optionFile(GetAutostartFilePath());
    if (!optionFile.good())
        return false;
    // Scan through file for "Hidden=true":
    std::string line;
    while (!optionFile.eof())
    {
        getline(optionFile, line);
        if (line.find("Hidden") != std::string::npos &&
            line.find("true") != std::string::npos)
            return false;
    }
    optionFile.close();

    return true;
}

bool SetStartOnSystemStartup(bool fAutoStart)
{
    if (!fAutoStart)
        boost::filesystem::remove(GetAutostartFilePath());
    else
    {
        char pszExePath[MAX_PATH+1];
        memset(pszExePath, 0, sizeof(pszExePath));
        if (readlink("/proc/self/exe", pszExePath, sizeof(pszExePath)-1) == -1)
            return false;

        boost::filesystem::create_directories(GetAutostartDir());

        boost::filesystem::ofstream optionFile(GetAutostartFilePath(), std::ios_base::out|std::ios_base::trunc);
        if (!optionFile.good())
            return false;
        // Write a bitcoin.desktop file to the autostart directory:
        optionFile << "[Desktop Entry]\n";
        optionFile << "Type=Application\n";
        optionFile << "Name=Pandacoin\n";
        optionFile << "Exec=" << pszExePath << " -min\n";
        optionFile << "Terminal=false\n";
        optionFile << "Hidden=false\n";
        optionFile.close();
    }
    return true;
}
#else

// TODO: OSX startup stuff; see:
// https://developer.apple.com/library/mac/#documentation/MacOSX/Conceptual/BPSystemStartup/Articles/CustomLogin.html

bool GetStartOnSystemStartup() { return false; }
bool SetStartOnSystemStartup(bool fAutoStart) { return false; }

#endif

HelpMessageBox::HelpMessageBox(QWidget *parent) :
    QMessageBox(parent)
{
    header = tr("Pandacoin-Qt") + " " + tr("version") + " " +
        QString::fromStdString(FormatFullVersion()) + "\n\n" +
        tr("Usage:") + "\n" +
        "  pandacoin-qt [" + tr("command-line options") + "]                     " + "\n";

    coreOptions = QString::fromStdString(HelpMessage());

    uiOptions = tr("UI options") + ":\n" +
        "  -lang=<lang>           " + tr("Set language, for example \"de_DE\" (default: system locale)") + "\n" +
        "  -min                   " + tr("Start minimized") + "\n" +
        "  -splash                " + tr("Show splash screen on startup (default: 1)") + "\n";

    setWindowTitle(tr("Pandacoin-Qt"));
    setTextFormat(Qt::PlainText);
    // setMinimumWidth is ignored for QMessageBox so put in non-breaking spaces to make it wider.
    setText(header + QString(QChar(0x2003)).repeated(50));
    setDetailedText(coreOptions + "\n" + uiOptions);
}

void HelpMessageBox::printToConsole()
{
    // On other operating systems, the expected action is to print the message to the console.
    QString strUsage = header + "\n" + coreOptions + "\n" + uiOptions;
    fprintf(stdout, "%s", strUsage.toStdString().c_str());
}

void HelpMessageBox::showOrPrint()
{
#if defined(WIN32)
        // On Windows, show a message box, as there is no stderr/stdout in windowed applications
        exec();
#else
        // On other operating systems, print help text to console
        printToConsole();
#endif
}
#endif //HEADLESS
} // namespace GUIUtil

