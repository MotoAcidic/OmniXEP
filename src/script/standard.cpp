// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2020-2024 The Xep Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <script/standard.h>

#include <crypto/sha256.h>
#include <pubkey.h>
#include <script/script.h>

typedef std::vector<unsigned char> valtype;

bool fAcceptDatacarrier = DEFAULT_ACCEPT_DATACARRIER;
unsigned nMaxDatacarrierBytes = MAX_OP_RETURN_RELAY;

CScriptID::CScriptID(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

ScriptHash::ScriptHash(const CScript& in) : uint160(Hash160(in.begin(), in.end())) {}

PKHash::PKHash(const CPubKey& pubkey) : uint160(pubkey.GetID()) {}

WitnessV0ScriptHash::WitnessV0ScriptHash(const CScript& in)
{
    CSHA256().Write(in.data(), in.size()).Finalize(begin());
}

const char* GetTxnOutputType(txnouttype t)
{
    switch (t)
    {
    case TX_NONSTANDARD: return "nonstandard";
    case TX_PUBKEY: return "pubkey";
    case TX_PUBKEY_REPLAY: return "pubkey_replay";
    case TX_PUBKEY_DATA_REPLAY: return "pubkey_data_replay";
    case TX_PUBKEYHASH: return "pubkeyhash";
    case TX_PUBKEYHASH_REPLAY: return "pubkeyhash_replay";
    case TX_SCRIPTHASH: return "scripthash";
    case TX_SCRIPTHASH_REPLAY: return "scripthash_replay";
    case TX_MULTISIG: return "multisig";
    case TX_MULTISIG_REPLAY: return "multisig_replay";
    case TX_MULTISIG_DATA: return "multisig_data";
    case TX_MULTISIG_DATA_REPLAY: return "multisig_data_replay";
    case TX_NULL_DATA: return "nulldata";
    case TX_WITNESS_V0_KEYHASH: return "witness_v0_keyhash";
    case TX_WITNESS_V0_SCRIPTHASH: return "witness_v0_scripthash";
    case TX_WITNESS_V1_TAPROOT: return "witness_v1_taproot";
    case TX_WITNESS_UNKNOWN: return "witness_unknown";
    } // no default case, so the compiler can warn about missing cases
    assert(false);
}

static bool MatchPayToPubkey(const CScript& script, valtype& pubkey)
{
    if (script.size() == CPubKey::SIZE + 2 && script[0] == CPubKey::SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    if (script.size() == CPubKey::COMPRESSED_SIZE + 2 && script[0] == CPubKey::COMPRESSED_SIZE && script.back() == OP_CHECKSIG) {
        pubkey = valtype(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_SIZE + 1);
        return CPubKey::ValidSize(pubkey);
    }
    return false;
}

static bool MatchPayToPubkeyHash(const CScript& script, valtype& pubkeyhash)
{
    if (script.size() == 25 && script[0] == OP_DUP && script[1] == OP_HASH160 && script[2] == 20 && script[23] == OP_EQUALVERIFY && script[24] == OP_CHECKSIG) {
        pubkeyhash = valtype(script.begin() + 3, script.begin() + 23);
        return true;
    }
    return false;
}

/** Test for "small positive integer" script opcodes - OP_1 through OP_16. */
static constexpr bool IsSmallInteger(opcodetype opcode)
{
    return opcode >= OP_1 && opcode <= OP_16;
}

static bool IsMinimalPush(const valtype& data, opcodetype opcode) {
    // Excludes OP_1NEGATE, OP_1-16 since they are by definition minimal
    if (0 > opcode || opcode > OP_PUSHDATA4) {
        return false;
    } else if (data.size() == 0) {
        // Should have used OP_0.
        return opcode == OP_0;
    } else if (data.size() == 1 && data[0] >= 1 && data[0] <= 16) {
        // Should have used OP_1 .. OP_16.
        return false;
    } else if (data.size() == 1 && data[0] == 0x81) {
        // Should have used OP_1NEGATE.
        return false;
    } else if (data.size() <= 75) {
        // Must have used a direct push (opcode indicating number of bytes pushed + those bytes).
        return opcode == data.size();
    } else if (data.size() <= 255) {
        // Must have used OP_PUSHDATA.
        return opcode == OP_PUSHDATA1;
    } else if (data.size() <= 65535) {
        // Must have used OP_PUSHDATA2.
        return opcode == OP_PUSHDATA2;
    }
    return true;
}

static bool IsMinimallyEncoded(const valtype& vch)
{
    if (vch.size() > 0) {
        // Check that the number is encoded with the minimum possible
        // number of bytes.
        //
        // If the most-significant-byte - excluding the sign bit - is zero
        // then we're not minimal. Note how this test also rejects the
        // negative-zero encoding, 0x80.
        if ((vch.back() & 0x7f) == 0) {
            // One exception: if there's more than one byte and the most
            // significant bit of the second-most-significant-byte is set
            // it would conflict with the sign bit. An example of this case
            // is +-255, which encode to 0xff00 and 0xff80 respectively.
            // (big-endian).
            if (vch.size() <= 1 || (vch[vch.size() - 2] & 0x80) == 0) {
                return false;
            }
        }
        return true;
    } else
        return false;
}

static bool MatchPayToPubkeyReplay(const CScript& script, std::vector<valtype>& txData)
{
    const unsigned int scriptSize = script.size();
    if (scriptSize < (CPubKey::COMPRESSED_SIZE + 6) || scriptSize > (CPubKey::COMPRESSED_SIZE + 42) || script[0] != CPubKey::COMPRESSED_SIZE ||
        script[CPubKey::COMPRESSED_SIZE + 1] != OP_CHECKSIG || script[scriptSize - 2] != OP_CHECKBLOCKATHEIGHTVERIFY || script.back() != OP_2DROP) return false;
    txData.emplace_back(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_SIZE + 1);

    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin() + CPubKey::COMPRESSED_SIZE + 2;

    if (!script.GetOp(it, opcode, data) || data.size() > 32 /* uint256 size */) return false;
    // Optionally ensure leading zeroes are trimmed from the block hash
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) /*|| data.size() == 0 || (data.back() & 0xff) == 0*/)) return false;
    txData.push_back(data);
    if (!script.GetOp(it, opcode, data) || data.size() > 4 /* int32_t size */) return false;
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) || !IsMinimallyEncoded(data))) return false;
    txData.push_back(data);

    return CPubKey::ValidSize(txData[0]);
}

static bool MatchPayToPubkeyDataReplay(const CScript& script, std::vector<valtype>& txData)
{
    const unsigned int scriptSize = script.size();
    if (scriptSize < (CPubKey::COMPRESSED_SIZE + 8) || scriptSize > (CPubKey::COMPRESSED_SIZE + 125) || script[0] != CPubKey::COMPRESSED_SIZE ||
        script[CPubKey::COMPRESSED_SIZE + 1] != OP_CHECKSIG || script[scriptSize - 2] != OP_CHECKBLOCKATHEIGHTVERIFY || script.back() != OP_2DROP) return false;
    txData.emplace_back(script.begin() + 1, script.begin() + CPubKey::COMPRESSED_SIZE + 1);

    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin() + CPubKey::COMPRESSED_SIZE + 2;

    if (!script.GetOp(it, opcode, data) || data.size() < 1 || data.size() > MAX_MULTISIG_DATA_OP_DROP_SIZE || !IsMinimalPush(data, opcode)) return false;
    if (!script.GetOp(it, opcode, data) || opcode != OP_DROP) return false;

    if (!script.GetOp(it, opcode, data) || data.size() > 32 /* uint256 size */) return false;
    // Optionally ensure leading zeroes are trimmed from the block hash
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) /*|| data.size() == 0 || (data.back() & 0xff) == 0*/)) return false;
    txData.push_back(data);
    if (!script.GetOp(it, opcode, data) || data.size() > 4 /* int32_t size */) return false;
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) || !IsMinimallyEncoded(data))) return false;
    txData.push_back(data);

    return CPubKey::ValidSize(txData[0]);
}

static bool MatchPayToScriptHashReplay(const CScript& script, std::vector<valtype>& txData)
{
    const unsigned int scriptSize = script.size();
    if (scriptSize < 27 || scriptSize > 63 || script[0] != OP_HASH160 || script[1] != 20 || script[22] != OP_EQUAL ||
        script[scriptSize - 2] != OP_CHECKBLOCKATHEIGHTVERIFY || script.back() != OP_2DROP) return false;

    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin() + 23;

    if (!script.GetOp(it, opcode, data) || data.size() > 32 /* uint256 size */) return false;
    // Optionally ensure leading zeroes are trimmed from the block hash
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) /*|| data.size() == 0 || (data.back() & 0xff) == 0*/)) return false;
    txData.push_back(data);
    if (!script.GetOp(it, opcode, data) || data.size() > 4 /* int32_t size */) return false;
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) || !IsMinimallyEncoded(data))) return false;
    txData.push_back(data);

    return true;
}

static bool MatchPayToPubkeyHashReplay(const CScript& script, std::vector<valtype>& txData)
{
    const unsigned int scriptSize = script.size();
    if (scriptSize < 29 || scriptSize > 65 || script[0] != OP_DUP || script[1] != OP_HASH160 || script[2] != 20 || script[23] != OP_EQUALVERIFY ||
        script[24] != OP_CHECKSIG || script[scriptSize - 2] != OP_CHECKBLOCKATHEIGHTVERIFY || script.back() != OP_2DROP) return false;
    txData.emplace_back(script.begin() + 3, script.begin() + 23);

    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin() + 25;

    if (!script.GetOp(it, opcode, data) || data.size() > 32 /* uint256 size */) return false;
    // Optionally ensure leading zeroes are trimmed from the block hash
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) /*|| data.size() == 0 || (data.back() & 0xff) == 0*/)) return false;
    txData.push_back(data);
    if (!script.GetOp(it, opcode, data) || data.size() > 4 /* int32_t size */) return false;
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) || !IsMinimallyEncoded(data))) return false;
    txData.push_back(data);

    return true;
}

static bool MatchMultisig(const CScript& script, unsigned int& required, std::vector<valtype>& pubkeys)
{
    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_CHECKMULTISIG) return false;

    if (!script.GetOp(it, opcode, data) || !IsSmallInteger(opcode)) return false;
    required = CScript::DecodeOP_N(opcode);
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    if (!IsSmallInteger(opcode)) return false;
    unsigned int keys = CScript::DecodeOP_N(opcode);
    if (pubkeys.size() != keys || keys < required) return false;
    return (it + 1 == script.end());
}

static bool MatchMultisigReplay(const CScript& script, unsigned int& required, std::vector<valtype>& pubkeys)
{
    const unsigned int scriptSize = script.size();
    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin();
    if (scriptSize < 1 || script[scriptSize - 2] != OP_CHECKBLOCKATHEIGHTVERIFY || script.back() != OP_2DROP) return false;

    if (!script.GetOp(it, opcode, data) || !IsSmallInteger(opcode)) return false;
    required = CScript::DecodeOP_N(opcode);
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    if (!IsSmallInteger(opcode)) return false;
    unsigned int keys = CScript::DecodeOP_N(opcode);
    if (pubkeys.size() != keys || keys < required) return false;

    if (!script.GetOp(it, opcode, data) || opcode != OP_CHECKMULTISIG) return false;

    if (!script.GetOp(it, opcode, data) || data.size() > 32 /* uint256 size */) return false;
    // Optionally ensure leading zeroes are trimmed from the block hash
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) /*|| data.size() == 0 || (data.back() & 0xff) == 0*/)) return false;
    if (!script.GetOp(it, opcode, data) || data.size() > 4 /* int32_t size */) return false;
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) || !IsMinimallyEncoded(data))) return false;

    return (it + 2 == script.end());
}

static bool MatchMultisigData(const CScript& script, unsigned int& required, std::vector<valtype>& pubkeys)
{
    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin();
    if (script.size() < 1 || script.back() != OP_DROP) return false;

    if (!script.GetOp(it, opcode, data) || !IsSmallInteger(opcode)) return false;
    required = CScript::DecodeOP_N(opcode);
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    if (!IsSmallInteger(opcode)) return false;
    unsigned int keys = CScript::DecodeOP_N(opcode);
    if (pubkeys.size() != keys || keys < required) return false;

    if (!script.GetOp(it, opcode, data) || opcode != OP_CHECKMULTISIG) return false;
    if (!script.GetOp(it, opcode, data) || data.size() < 1 || data.size() > MAX_MULTISIG_DATA_OP_DROP_SIZE || !IsMinimalPush(data, opcode)) return false;

    return (it + 1 == script.end());
}

static bool MatchMultisigDataReplay(const CScript& script, unsigned int& required, std::vector<valtype>& pubkeys)
{
    const unsigned int scriptSize = script.size();
    opcodetype opcode;
    valtype data;
    CScript::const_iterator it = script.begin();
    if (scriptSize < 1 || script[scriptSize - 2] != OP_CHECKBLOCKATHEIGHTVERIFY || script.back() != OP_2DROP) return false;

    if (!script.GetOp(it, opcode, data) || !IsSmallInteger(opcode)) return false;
    required = CScript::DecodeOP_N(opcode);
    while (script.GetOp(it, opcode, data) && CPubKey::ValidSize(data)) {
        pubkeys.emplace_back(std::move(data));
    }
    if (!IsSmallInteger(opcode)) return false;
    unsigned int keys = CScript::DecodeOP_N(opcode);
    if (pubkeys.size() != keys || keys < required) return false;

    if (!script.GetOp(it, opcode, data) || opcode != OP_CHECKMULTISIG) return false;
    if (!script.GetOp(it, opcode, data) || data.size() < 1 || data.size() > MAX_MULTISIG_DATA_OP_DROP_SIZE || !IsMinimalPush(data, opcode)) return false;
    if (!script.GetOp(it, opcode, data) || opcode != OP_DROP) return false;

    if (!script.GetOp(it, opcode, data) || data.size() > 32 /* uint256 size */) return false;
    // Optionally ensure leading zeroes are trimmed from the block hash
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) /*|| data.size() == 0 || (data.back() & 0xff) == 0*/)) return false;
    if (!script.GetOp(it, opcode, data) || data.size() > 4 /* int32_t size */) return false;
    if (!IsSmallInteger(opcode) && (!IsMinimalPush(data, opcode) || !IsMinimallyEncoded(data))) return false;

    return (it + 2 == script.end());
}

txnouttype Solver(const CScript& scriptPubKey, std::vector<std::vector<unsigned char>>& vSolutionsRet)
{
    vSolutionsRet.clear();

    // Shortcut for pay-to-script-hash, which are more constrained than the other types:
    // it is always OP_HASH160 20 [20 byte hash] OP_EQUAL
    if (scriptPubKey.IsPayToScriptHash()) {
        std::vector<unsigned char> hashBytes(scriptPubKey.begin() + 2, scriptPubKey.begin() + 22);
        vSolutionsRet.push_back(hashBytes);
        std::vector<std::vector<unsigned char>> txData;
        if (MatchPayToScriptHashReplay(scriptPubKey, txData)) {
            vSolutionsRet.insert(vSolutionsRet.end(), txData.begin(), txData.end());
            return TX_SCRIPTHASH_REPLAY;
        } else {
            return TX_SCRIPTHASH;
        }
    }

    int witnessversion;
    std::vector<unsigned char> witnessprogram;
    if (scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_KEYHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TX_WITNESS_V0_KEYHASH;
        }
        if (witnessversion == 0 && witnessprogram.size() == WITNESS_V0_SCRIPTHASH_SIZE) {
            vSolutionsRet.push_back(witnessprogram);
            return TX_WITNESS_V0_SCRIPTHASH;
        }
        if (witnessversion != 0) {
            vSolutionsRet.push_back(std::vector<unsigned char>{(unsigned char)witnessversion});
            vSolutionsRet.push_back(std::move(witnessprogram));
            return TX_WITNESS_UNKNOWN;
        }
        return TX_NONSTANDARD;
    }

    // Provably prunable, data-carrying output
    //
    // So long as script passes the IsUnspendable() test and all but the first
    // byte passes the IsPushOnly() test we don't care what exactly is in the
    // script.
    if (scriptPubKey.size() >= 1 && scriptPubKey[0] == OP_RETURN && scriptPubKey.IsPushOnly(scriptPubKey.begin() + 1)) {
        return TX_NULL_DATA;
    }

    std::vector<unsigned char> data;
    if (MatchPayToPubkey(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TX_PUBKEY;
    }

    std::vector<std::vector<unsigned char>> txData;
    if (MatchPayToPubkeyReplay(scriptPubKey, txData)) {
        vSolutionsRet.insert(vSolutionsRet.end(), txData.begin(), txData.end());
        return TX_PUBKEY_REPLAY;
    }

    txData.clear();
    if (MatchPayToPubkeyDataReplay(scriptPubKey, txData)) {
        vSolutionsRet.insert(vSolutionsRet.end(), txData.begin(), txData.end());
        return TX_PUBKEY_DATA_REPLAY;
    }

    if (MatchPayToPubkeyHash(scriptPubKey, data)) {
        vSolutionsRet.push_back(std::move(data));
        return TX_PUBKEYHASH;
    }

    txData.clear();
    if (MatchPayToPubkeyHashReplay(scriptPubKey, txData)) {
        vSolutionsRet.insert(vSolutionsRet.end(), txData.begin(), txData.end());
        return TX_PUBKEYHASH_REPLAY;
    }

    unsigned int required;
    std::vector<std::vector<unsigned char>> keys;
    if (MatchMultisig(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..16
        return TX_MULTISIG;
    }

    keys.clear();
    if (MatchMultisigReplay(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..16
        return TX_MULTISIG_REPLAY;
    }

    keys.clear();
    if (MatchMultisigData(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..16
        return TX_MULTISIG_DATA;
    }

    keys.clear();
    if (MatchMultisigDataReplay(scriptPubKey, required, keys)) {
        vSolutionsRet.push_back({static_cast<unsigned char>(required)}); // safe as required is in range 1..16
        vSolutionsRet.insert(vSolutionsRet.end(), keys.begin(), keys.end());
        vSolutionsRet.push_back({static_cast<unsigned char>(keys.size())}); // safe as size is in range 1..16
        return TX_MULTISIG_DATA_REPLAY;
    }

    vSolutionsRet.clear();
    return TX_NONSTANDARD;
}

bool ExtractDestination(const CScript& scriptPubKey, CTxDestination& addressRet)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType = Solver(scriptPubKey, vSolutions);

    if (whichType == TX_PUBKEY || whichType == TX_PUBKEY_REPLAY || whichType == TX_PUBKEY_DATA_REPLAY) {
        CPubKey pubKey(vSolutions[0]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    } else if (whichType == TX_PUBKEYHASH || whichType == TX_PUBKEYHASH_REPLAY) {
        addressRet = PKHash(uint160(vSolutions[0]));
        return true;
    } else if (whichType == TX_SCRIPTHASH || whichType == TX_SCRIPTHASH_REPLAY) {
        addressRet = ScriptHash(uint160(vSolutions[0]));
        return true;
    } else if (whichType == TX_WITNESS_V0_KEYHASH) {
        WitnessV0KeyHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TX_WITNESS_V0_SCRIPTHASH) {
        WitnessV0ScriptHash hash;
        std::copy(vSolutions[0].begin(), vSolutions[0].end(), hash.begin());
        addressRet = hash;
        return true;
    } else if (whichType == TX_WITNESS_UNKNOWN) {
        WitnessUnknown unk;
        unk.version = vSolutions[0][0];
        std::copy(vSolutions[1].begin(), vSolutions[1].end(), unk.program);
        unk.length = vSolutions[1].size();
        addressRet = unk;
        return true;
    } else if (whichType == TX_MULTISIG || whichType == TX_MULTISIG_REPLAY || whichType == TX_MULTISIG_DATA || whichType == TX_MULTISIG_DATA_REPLAY) {
        if (vSolutions.size() != 3 || vSolutions.front()[0] != 1 || vSolutions.back()[0] != 1)
            return false;
        CPubKey pubKey(vSolutions[1]);
        if (!pubKey.IsValid())
            return false;

        addressRet = PKHash(pubKey);
        return true;
    }
    // Multisig txns have more than one address...
    return false;
}

bool ExtractDestinations(const CScript& scriptPubKey, txnouttype& typeRet, std::vector<CTxDestination>& addressRet, int& nRequiredRet)
{
    addressRet.clear();
    std::vector<valtype> vSolutions;
    typeRet = Solver(scriptPubKey, vSolutions);
    if (typeRet == TX_NONSTANDARD) {
        return false;
    } else if (typeRet == TX_NULL_DATA) {
        // This is data, not addresses
        return false;
    }

    if (typeRet == TX_MULTISIG || typeRet == TX_MULTISIG_REPLAY || typeRet == TX_MULTISIG_DATA || typeRet == TX_MULTISIG_DATA_REPLAY) {
        nRequiredRet = vSolutions.front()[0];
        for (unsigned int i = 1; i < vSolutions.size() - 1; i++) {
            CPubKey pubKey(vSolutions[i]);
            if (!pubKey.IsValid())
                continue;

            CTxDestination address = PKHash(pubKey);
            addressRet.push_back(address);
        }

        if (addressRet.empty())
            return false;
    } else {
        nRequiredRet = 1;
        CTxDestination address;
        if (!ExtractDestination(scriptPubKey, address))
            return false;
        addressRet.push_back(address);
    }

    return true;
}

namespace {
class CScriptVisitor : public boost::static_visitor<bool>
{
private:
    CScript* script;

public:
    explicit CScriptVisitor(CScript* scriptin) { script = scriptin; }

    bool operator()(const CNoDestination& dest) const
    {
        script->clear();
        return false;
    }

    bool operator()(const PKHash& keyID) const
    {
        script->clear();
        *script << OP_DUP << OP_HASH160 << ToByteVector(keyID) << OP_EQUALVERIFY << OP_CHECKSIG;
        return true;
    }

    bool operator()(const ScriptHash& scriptID) const
    {
        script->clear();
        *script << OP_HASH160 << ToByteVector(scriptID) << OP_EQUAL;
        return true;
    }

    bool operator()(const WitnessV0KeyHash& id) const
    {
        script->clear();
        *script << OP_0 << ToByteVector(id);
        return true;
    }

    bool operator()(const WitnessV0ScriptHash& id) const
    {
        script->clear();
        *script << OP_0 << ToByteVector(id);
        return true;
    }

    bool operator()(const WitnessUnknown& id) const
    {
        script->clear();
        *script << CScript::EncodeOP_N(id.version) << std::vector<unsigned char>(id.program, id.program + id.length);
        return true;
    }
};
} // namespace

CScript GetScriptForDestination(const CTxDestination& dest)
{
    CScript script;

    boost::apply_visitor(CScriptVisitor(&script), dest);
    return script;
}

CScript GetScriptForRawPubKey(const CPubKey& pubKey)
{
    return CScript() << std::vector<unsigned char>(pubKey.begin(), pubKey.end()) << OP_CHECKSIG;
}

CScript GetScriptForMultisig(int nRequired, const std::vector<CPubKey>& keys)
{
    CScript script;

    script << CScript::EncodeOP_N(nRequired);
    for (const CPubKey& key : keys)
        script << ToByteVector(key);
    script << CScript::EncodeOP_N(keys.size()) << OP_CHECKMULTISIG;
    return script;
}

CScript GetScriptForWitness(const CScript& redeemscript)
{
    std::vector<std::vector<unsigned char>> vSolutions;
    txnouttype typ = Solver(redeemscript, vSolutions);
    if (typ == TX_PUBKEY) {
        return GetScriptForDestination(WitnessV0KeyHash(Hash160(vSolutions[0].begin(), vSolutions[0].end())));
    } else if (typ == TX_PUBKEYHASH) {
        return GetScriptForDestination(WitnessV0KeyHash(vSolutions[0]));
    }
    return GetScriptForDestination(WitnessV0ScriptHash(redeemscript));
}

bool IsValidDestination(const CTxDestination& dest)
{
    return dest.which() != 0;
}


valtype DataVisitor::operator()(const CNoDestination& noDest) const { return valtype(); }
valtype DataVisitor::operator()(const CKeyID& keyID) const { return valtype(keyID.begin(), keyID.end()); }
valtype DataVisitor::operator()(const CScriptID& scriptID) const { return valtype(scriptID.begin(), scriptID.end()); }
valtype DataVisitor::operator()(const WitnessV0ScriptHash& witnessScriptHash) const { return valtype(witnessScriptHash.begin(), witnessScriptHash.end()); }
valtype DataVisitor::operator()(const WitnessV0KeyHash& witnessKeyHash) const { return valtype(witnessKeyHash.begin(), witnessKeyHash.end()); }
valtype DataVisitor::operator()(const WitnessUnknown&) const { return valtype(); }
