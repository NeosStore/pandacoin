// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "transactionrecord.h"

#include "wallet.h"
#include "base58.h"
#include <map>

/* Return positive answer if transaction should be shown in list.
 */
bool TransactionRecord::showTransaction(const CWalletTx &wtx)
{
    if (wtx.IsCoinBase())
    {
        // Ensures we show generated coins / mined transactions at depth 1
        if (!wtx.IsInMainChain())
        {
            return false;
        }
    }
    return true;
}

/*
 * Decompose CWallet transaction to model transaction records.
 */
QList<TransactionRecord> TransactionRecord::decomposeTransaction(const CWallet *wallet, const CWalletTx &wtx)
{
    QList<TransactionRecord> parts;
    int64_t nTime = wtx.GetTxTime();
    int64_t nCredit = wtx.GetCredit(true);
    int64_t nDebit = wtx.GetDebit();
    int64_t nNet = nCredit - nDebit;
    uint256 hash = wtx.GetHash(), hashPrev = 0;
    std::map<std::string, std::string> mapValue = wtx.mapValue;

    if (nNet > 0 || wtx.IsCoinBase() || wtx.IsCoinStake())
    {
        //
        // Credit
        //
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            if(wallet->IsMine(txout))
            {
                TransactionRecord sub(hash, nTime);
                sub.fromAddress = "";

                CTxDestination address;
                sub.credit = txout.nValue;
                if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*wallet, address))
                {
                    // Received by Bitcoin Address
                    sub.type = TransactionRecord::RecvWithAddress;
                    sub.address = wallet->GetPrimaryAddress(txout, wtx);
                }
                else
                {
                    // Received by IP connection (deprecated features), or a multisignature or other non-simple transaction
                    sub.type = TransactionRecord::RecvFromOther;
                    sub.address = mapValue["from"];
                }
                if (wtx.IsCoinStake())
                {
                    // Generated (proof-of-stake)

                    if (hashPrev == hash)
                        continue; // last coinstake output

                    sub.type = TransactionRecord::Generated;
                    sub.credit = nNet > 0 ? nNet : wtx.GetValueOut() - nDebit;
                    hashPrev = hash;

                    sub.idx = parts.size();
                    parts.append(sub);
                    continue;
                }
                else if (wtx.IsCoinBase())
                {
                    // Generated (proof-of-work)
                    sub.type = TransactionRecord::Generated;

                    sub.idx = parts.size();
                    parts.append(sub);
                    continue;
                }

                // Exclude change (must be done after we check for proof of work/stake as some of that is being detected as change.
                if(wallet->IsChange(txout))
                {
                    continue;
                }

                sub.idx = parts.size();
                parts.append(sub);
            }
        }
    }
    else
    {
        bool fAllFromMe = true;
        BOOST_FOREACH(const CTxIn& txin, wtx.vin)
            fAllFromMe = fAllFromMe && wallet->IsMine(txin);

        bool fAllToMe = true;
        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            fAllToMe = fAllToMe && wallet->IsMine(txout);

        TransactionRecord sub(hash, nTime);

        if (fAllFromMe && fAllToMe)
        {
            sub.address = "";

            // Receiver
            std::string toAddress = wallet->GetPrimaryAddress(wtx.vout[0], wtx);

            // Sender
            std::string fromAddress = wallet->GetPrimaryAddress(wtx.vin[0]);

            // Gather all transactions together
            std::map<std::string, int64_t> transactionMap;

            // Credit, including change
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            {
                transactionMap[wallet->GetPrimaryAddress(txout, wtx)] += txout.nValue;
            }

            // Debit
            BOOST_FOREACH(CTxIn partin, wtx.vin)
            {
                transactionMap[wallet->GetPrimaryAddress(partin)] += -(wallet->mapWallet.find(partin.prevout.hash)->second.vout[partin.prevout.n].nValue);
            }

            //fixme: FEE
            // Now output the transaction totals
            for(std::map<std::string, int64_t>::iterator iter = transactionMap.begin(); iter != transactionMap.end(); iter++)
            {
                // fixme: Try match the inputs and outputs.
                if(iter->second > 0)
                {
                    sub.debit=0;
                    sub.credit = iter->second;
                    sub.type = TransactionRecord::InternalReceive;
                    // fixme: Try match the inputs and outputs.
                    sub.fromAddress="";
                    sub.address=iter->first;
                }
                else
                {
                    sub.credit=0;
                    sub.debit = iter->second;
                    sub.type = TransactionRecord::InternalSend;
                    sub.fromAddress=iter->first;
                    // fixme: Try match the inputs and outputs.
                    sub.address="";
                }
                sub.idx = parts.size();
                parts.append(sub);
            }
            transactionMap.clear();
        }
        else if (fAllFromMe)
        {
            // Gather and sum all outgoing transactions for address together (change included)
            std::map<std::string, int64_t> transactionSenderMap;

            // Gather all recipients together (change included)
            std::map<std::string, int64_t> transactionRecipientMap;

            // Subtract change from outgoing transactions
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            {
                if(wallet->IsChange(txout))
                {
                    transactionSenderMap[wallet->GetPrimaryAddress(txout, wtx)] -= txout.nValue;
                }
            }

            // Outgoing transaction amounts
            std::map<std::string, int64_t> addressAmountMap;
            BOOST_FOREACH(CTxIn txin, wtx.vin)
            {
                transactionSenderMap[wallet->GetPrimaryAddress(txin)] += wallet->mapWallet.find(txin.prevout.hash)->second.vout[txin.prevout.n].nValue;
            }

            // Recipient amounts
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            {
                if(wallet->IsChange(txout))
                {
                    continue;
                }

                CTxDestination address;
                if (ExtractDestination(txout.scriptPubKey, address))
                {
                    // Sent to Bitcoin Address
                    if(wallet->IsMine(CBitcoinAddress(address).ToString()))
                    {
                        transactionRecipientMap[wallet->GetPrimaryAddress(txout, wtx)] += txout.nValue;
                    }
                    else
                    {
                        transactionRecipientMap[CBitcoinAddress(address).ToString()] += txout.nValue;
                    }
                }
                else
                {
                    //fixme: Suspect this probably no longer works - does panda even allow this??


                    // Sent to IP, or other non-address transaction like OP_EVAL
                    sub.type = TransactionRecord::SendToOther;
                    sub.address = mapValue["to"];

                    int64_t nValue = txout.nValue;
                    sub.debit = -nValue;

                    sub.idx = parts.size();
                    parts.append(sub);

                    // Transaction fee
                    int64_t nTxFee = nDebit - wtx.GetValueOut();
                    if (nTxFee > 0)
                    {
                        sub.type = TransactionRecord::Fee;
                        sub.idx = parts.size();
                        sub.debit = -nTxFee;
                        sub.address = "";
                        parts.append(sub);
                    }
                }

            }

            //fixme: (More intelligent mapping) - find best fit instead of taking first fit.
            std::vector<TransactionRecord> finalMapping;
            std::map<std::string, int64_t>::iterator sendIter;
            for(sendIter = transactionSenderMap.begin(); sendIter != transactionSenderMap.end(); sendIter++)
            {
                std::map<std::string, int64_t>::iterator rxIter = transactionRecipientMap.begin();
                for(; rxIter != transactionRecipientMap.end(); rxIter++)
                {
                    if(sendIter->second > 0 && rxIter->second > 0 && rxIter->second <= sendIter->second)
                    {
                        TransactionRecord sendTx;
                        sendTx.hash = sub.hash;
                        sendTx.time = sub.time;
                        sendTx.type = TransactionRecord::SendToAddress;
                        sendTx.address = rxIter->first;
                        sendTx.fromAddress = sendIter->first;
                        sendTx.debit = -rxIter->second;
                        //sendTx.credit = rxIter->second;

                        // Internal transaction
                        if(wallet->IsMine(rxIter->first))
                        {
                            // Send
                            sendTx.type = TransactionRecord::InternalSend;
                            finalMapping.push_back(sendTx);

                            // Receive
                            sendTx.type = TransactionRecord::InternalReceive;
                            sendTx.credit = -sendTx.debit;
                            sendTx.debit = 0;
                            finalMapping.push_back(sendTx);
                        }
                        else
                        {
                            finalMapping.push_back(sendTx);
                        }

                        sendIter->second -= rxIter->second;
                        rxIter->second=0;
                    }
                }
            }
            std::map<std::string, int64_t>::iterator rxIter;
            for(rxIter = transactionRecipientMap.begin(); rxIter != transactionRecipientMap.end(); rxIter++)
            {
                if(rxIter->second > 0)
                {
                    std::map<std::string, int64_t>::iterator sendIter = transactionSenderMap.begin();
                    for(; sendIter != transactionSenderMap.end(); sendIter++)
                    {
                        if(sendIter->second > 0)
                        {
                            if(sendIter->second <= rxIter->second)
                            {
                                TransactionRecord sendTx;
                                sendTx.hash = sub.hash;
                                sendTx.time = sub.time;
                                sendTx.type = TransactionRecord::SendToAddress;
                                sendTx.address = rxIter->first;
                                sendTx.fromAddress = sendIter->first;
                                sendTx.debit = -sendIter->second;

                                // Internal transaction
                                if(wallet->IsMine(rxIter->first))
                                {
                                    // Send
                                    sendTx.type = TransactionRecord::InternalSend;
                                    finalMapping.push_back(sendTx);

                                    // Receive
                                    sendTx.type = TransactionRecord::InternalReceive;
                                    sendTx.credit = -sendTx.debit;
                                    sendTx.debit = 0;
                                    finalMapping.push_back(sendTx);
                                }
                                else
                                {
                                    finalMapping.push_back(sendTx);
                                }

                                rxIter->second -= sendIter->second;
                                sendIter->second=0;
                            }
                        }
                    }
                }
                /*if(rxIter->second > 0)
                {
                    int bug = 0;
                }*/
            }

            // Anything that remains is a Fee.
            for(sendIter = transactionSenderMap.begin(); sendIter != transactionSenderMap.end(); sendIter++)
            {

                if(sendIter->second > 0)
                {
                    TransactionRecord sendTx;
                    sendTx.hash = sub.hash;
                    sendTx.time = sub.time;
                    sendTx.type = TransactionRecord::Fee;
                    sendTx.address = "";
                    sendTx.fromAddress = sendIter->first;
                    sendTx.debit = -sendIter->second;
                    finalMapping.push_back(sendTx);
                    sendIter->second = 0;
                }
            }

            // Now add the transactions all at once
            for(unsigned int i = 0; i < finalMapping.size(); i++)
            {
                finalMapping[i].idx = parts.size();
                parts.append(finalMapping[i]);
            }
        }
        else
        {
            //
            // Mixed debit transaction, can't break down payees
            //
            sub.idx = parts.size();
            parts.append(TransactionRecord(hash, nTime, TransactionRecord::Other, "", nNet, 0));
        }
    }

    return parts;
}

void TransactionRecord::updateStatus(const CWalletTx &wtx)
{
    //checkme: Not sure if this is strictly necessary or overkill?
    cachedBalance.clear();

    // Determine transaction status

    // Find the block the tx is in
    CBlockIndex* pindex = NULL;
    std::map<uint256, CBlockIndex*>::iterator mi = mapBlockIndex.find(wtx.hashBlock);
    if (mi != mapBlockIndex.end())
        pindex = (*mi).second;

    // Sort order, unrecorded transactions sort to the top
    status.sortKey = strprintf("%010d-%01d-%010u-%03d",
        (pindex ? pindex->nHeight : std::numeric_limits<int>::max()),
        (wtx.IsCoinBase() ? 1 : 0),
        wtx.nTimeReceived,
        idx);
    status.countsForBalance = wtx.IsTrusted() && !(wtx.GetBlocksToMaturity() > 0);
    status.depth = wtx.GetDepthInMainChain();
    status.cur_num_blocks = nBestHeight;

    if (!wtx.IsFinal())
    {
        if (wtx.nLockTime < LOCKTIME_THRESHOLD)
        {
            status.status = TransactionStatus::OpenUntilBlock;
            status.open_for = nBestHeight - wtx.nLockTime;
        }
        else
        {
            status.status = TransactionStatus::OpenUntilDate;
            status.open_for = wtx.nLockTime;
        }
    }

    // For generated transactions, determine maturity
    else if(type == TransactionRecord::Generated)
    {
        if (wtx.GetBlocksToMaturity() > 0)
        {
            status.status = TransactionStatus::Immature;

            if (wtx.IsInMainChain())
            {
                status.matures_in = wtx.GetBlocksToMaturity();

                // Check if the block was requested by anyone
                if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
                    status.status = TransactionStatus::MaturesWarning;
            }
            else
            {
                status.status = TransactionStatus::NotAccepted;
            }
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
    else
    {
        if (status.depth < 0)
        {
            status.status = TransactionStatus::Conflicted;
        }
        else if (GetAdjustedTime() - wtx.nTimeReceived > 2 * 60 && wtx.GetRequestCount() == 0)
        {
            status.status = TransactionStatus::Offline;
        }
        else if (status.depth == 0)
        {
            status.status = TransactionStatus::Unconfirmed;
        }
        else if (status.depth < RecommendedNumConfirmations)
        {
            status.status = TransactionStatus::Confirming;
        }
        else
        {
            status.status = TransactionStatus::Confirmed;
        }
    }
}

bool TransactionRecord::statusUpdateNeeded()
{
    return status.cur_num_blocks != nBestHeight;
}

std::string TransactionRecord::getTxID()
{
    return hash.ToString(); // + strprintf("-%03d", idx);
}


int64_t TransactionRecord::getCachedBalance(std::string accountAddress) const
{
    if(cachedBalance.empty())
        return -1;

    std::map<std::string, int64_t>::const_iterator iter = cachedBalance.find(accountAddress);
    if(iter != cachedBalance.end())
    {
        return iter->second;
    }
    return -1;
}


void TransactionRecord::setCachedBalance(std::string accountAddress, int64_t amount)
{
    cachedBalance[accountAddress] = amount;
}

