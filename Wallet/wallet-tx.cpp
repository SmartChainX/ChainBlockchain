#include "wallet/wallet.h"

#include "base58.h"
#include "checkpoints.h"
#include "chain.h"
#include "coincontrol.h"
#include "consensus/consensus.h"
#include "consensus/validation.h"
#include "key.h"
#include "keystore.h"
#include "main.h"
#include "net.h"
#include "policy/policy.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/sign.h"
#include "timedata.h"
#include "txmempool.h"
#include "util.h"
#include "utilmoneystr.h"

#include "darksend.h"
#include "governance.h"
#include "instantx.h"
#include "keepass.h"
#include "spork.h"

#include <assert.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>


using namespace std;

/** Transaction fee set by the user */
CFeeRate payTxFee(DEFAULT_TRANSACTION_FEE);
CAmount maxTxFee = DEFAULT_TRANSACTION_MAXFEE;
unsigned int nTxConfirmTarget = DEFAULT_TX_CONFIRM_TARGET;
bool bSpendZeroConfChange = DEFAULT_SPEND_ZEROCONF_CHANGE;
bool fSendFreeTransactions = DEFAULT_SEND_FREE_TRANSACTIONS;

/** 
 * Fees smaller than this (in duffs) are considered zero fee (for transaction creation)
 * Override with -mintxfee
 */
CFeeRate CWallet::minTxFee = CFeeRate(DEFAULT_TRANSACTION_MINFEE);
/**
 * If fee estimation does not have enough data to provide estimates, use this fee instead.
 * Has no effect if not using fee estimation
 * Override with -fallbackfee
 */
CFeeRate CWallet::fallbackFee = CFeeRate(DEFAULT_FALLBACK_FEE);

const uint256 CMerkleTx::ABANDON_HASH(uint256S("0000000000000000000000000000000000000000000000000000000000000001"));

/** @defgroup mapWallet
 *
 * @{
 */

struct CompareValueOnly
{
    bool operator()(const pair<CAmount, pair<const CWalletTx*, unsigned int> >& t1,
                    const pair<CAmount, pair<const CWalletTx*, unsigned int> >& t2) const
    {
        return t1.first < t2.first;
    }
};

std::string COutput::ToString() const
{
    return strprintf("COutput(%s, %d, %d) [%s]", tx->GetHash().ToString(), i, nDepth, FormatMoney(tx->vout[i].nValue));
}

int COutput::Priority() const
{
    BOOST_FOREACH(CAmount d, vecPrivateSendDenominations)
        if(tx->vout[i].nValue == d) return 10000;
    if(tx->vout[i].nValue < 1*COIN) return 20000;

    //nondenom return largest first
    return -(tx->vout[i].nValue/COIN);
}

const CWalletTx* CWallet::GetWalletTx(const uint256& hash) const
{
    LOCK(cs_wallet);
    std::map<uint256, CWalletTx>::const_iterator it = mapWallet.find(hash);
    if (it == mapWallet.end())
        return NULL;
    return &(it->second);
}

CPubKey CWallet::GenerateNewKey()
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    bool fCompressed = CanSupportFeature(FEATURE_COMPRPUBKEY); // default to compressed public keys if we want 0.6.0 wallets

    CKey secret;
    secret.MakeNewKey(fCompressed);

    // Compressed public keys were introduced in version 0.6.0
    if (fCompressed)
        SetMinVersion(FEATURE_COMPRPUBKEY);

    CPubKey pubkey = secret.GetPubKey();
    assert(secret.VerifyPubKey(pubkey));

    // Create new metadata
    int64_t nCreationTime = GetTime();
    mapKeyMetadata[pubkey.GetID()] = CKeyMetadata(nCreationTime);
    if (!nTimeFirstKey || nCreationTime < nTimeFirstKey)
        nTimeFirstKey = nCreationTime;

    if (!AddKeyPubKey(secret, pubkey))
        throw std::runtime_error("CWallet::GenerateNewKey(): AddKey failed");
    return pubkey;
}

bool CWallet::AddKeyPubKey(const CKey& secret, const CPubKey &pubkey)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (!CCryptoKeyStore::AddKeyPubKey(secret, pubkey))
        return false;

    // check if we need to remove from watch-only
    CScript script;
    script = GetScriptForDestination(pubkey.GetID());
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);
    script = GetScriptForRawPubKey(pubkey);
    if (HaveWatchOnly(script))
        RemoveWatchOnly(script);

    if (!fFileBacked)
        return true;
    if (!IsCrypted()) {
        return CWalletDB(strWalletFile).WriteKey(pubkey,
                                                 secret.GetPrivKey(),
                                                 mapKeyMetadata[pubkey.GetID()]);
    }
    return true;
}

bool CWallet::AddCryptedKey(const CPubKey &vchPubKey,
                            const vector<unsigned char> &vchCryptedSecret)
{
    if (!CCryptoKeyStore::AddCryptedKey(vchPubKey, vchCryptedSecret))
        return false;
    if (!fFileBacked)
        return true;
    {
        LOCK(cs_wallet);
        if (pwalletdbEncryption)
            return pwalletdbEncryption->WriteCryptedKey(vchPubKey,
                                                        vchCryptedSecret,
                                                        mapKeyMetadata[vchPubKey.GetID()]);
        else
            return CWalletDB(strWalletFile).WriteCryptedKey(vchPubKey,
                                                            vchCryptedSecret,
                                                            mapKeyMetadata[vchPubKey.GetID()]);
    }
    return false;
}

bool CWallet::LoadKeyMetadata(const CPubKey &pubkey, const CKeyMetadata &meta)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[pubkey.GetID()] = meta;
    return true;
}
