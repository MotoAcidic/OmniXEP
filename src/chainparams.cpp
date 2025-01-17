// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>

#include <arith_uint256.h>
#include <chainparamsseeds.h>
#include <consensus/merkle.h>
#include <tinyformat.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <versionbitsinfo.h>

#include <assert.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

static CBlock CreateGenesisBlock(const char* pszTimestamp, const std::vector<CScript>& genesisOutputScripts, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const std::vector<CAmount>& genesisRewards)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vin[0].scriptSig = CScript() << OP_0 << nBits << OP_4 << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    for (unsigned int i = 0; i < genesisOutputScripts.size(); i++)
        txNew.vout.emplace_back(genesisRewards[i], genesisOutputScripts[i]);

    CBlock genesis;
    genesis.nTime = nTime;
    genesis.nBits = nBits;
    genesis.nNonce = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);

    arith_uint256 hashTarget = arith_uint256().SetCompactBase256(std::min(genesis.nBits, (unsigned)0x1f00ffff));
    /*while (true) {
        arith_uint256 hash = UintToArith256(genesis.GetPoWHash());
        if (hash <= hashTarget) {
            // Found a solution
            printf("genesis block found\n   hash: %s\n target: %s\n   bits: %08x\n  nonce: %u\n", hash.ToString().c_str(), hashTarget.ToString().c_str(), genesis.nBits, genesis.nNonce);
            break;
        }
        genesis.nNonce += 1;
        if ((genesis.nNonce & 0x1ffff) == 0)
            printf("testing nonce: %u\n", genesis.nNonce);
    }*/
    uint256 hash = genesis.GetPoWHash();
    assert(UintToArith256(hash) <= hashTarget);

    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=000000000019d6, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=4a5e1e, nTime=1231006505, nBits=1d00ffff, nNonce=2083236893, vtx=1)
 *   CTransaction(hash=4a5e1e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0x5F1DF16B2B704C8A578D0B)
 *   vMerkleTree: 4a5e1e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const std::vector<CAmount>& genesisRewards)
{
    const char* pszTimestamp = "Electra Protocol is reborn from block 970621533f14eb1453e36b9862f0b766b4a3e0a98486bd6de2a7d265a22bcb18";
    std::vector<CScript> genesisOutputScripts;
    genesisOutputScripts.emplace_back(CScript() << OP_0 << ParseHex("b7ab61f3f8f36f98177aee6ee0b5b051a9e53471")); // ep1qk74krulc7dhes9m6aehwpdds2x572dr3zne8mz
    genesisOutputScripts.emplace_back(CScript() << OP_0 << ParseHex("978a5064cd1fdf8c2510fe3fcbd65eaa5e98b32d")); // ep1qj799qexdrl0ccfgslcluh4j74f0f3vedatcv0k
    genesisOutputScripts.emplace_back(CScript() << OP_0 << ParseHex("c64fc6777dcffc027ebcfc80d4a91b7304cf798d")); // ep1qce8uvamael7qyl4uljqdf2gmwvzv77vdh852h9
    genesisOutputScripts.emplace_back(CScript() << OP_0 << ParseHex("4536e905b8c5bbc163137fed4cde7d12f0de010f")); // ep1qg5mwjpdcckauzccn0lk5ehnaztcduqg09g6jgu
    genesisOutputScripts.emplace_back(CScript() << OP_0 << ParseHex("5417a551f0989b8a3b00257645cb1e3d2884ca64")); // ep1q2st6250snzdc5wcqy4mytjc7855gfjnyhxyu4f
    assert(genesisOutputScripts.size() == genesisRewards.size());
    return CreateGenesisBlock(pszTimestamp, genesisOutputScripts, nTime, nNonce, nBits, nVersion, genesisRewards);
}

/**
 * Main network
 */
class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = CBaseChainParams::MAIN;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nBudgetPaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.nPoSStartBlock = 0;
        consensus.nLastPoWBlock = 150000;
        consensus.nMandatoryUpgradeBlock = 150000;
        consensus.nTreasuryPaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x000000954c02f260a6db02c712557adcb5a7a8a0a9acfd3d3c2b3a427376c56f");
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;                                                                                                         // segwit activation height + miner confirmation window
        consensus.powLimit[CBlockHeader::AlgoType::ALGO_POS] = uint256S("000000ffff000000000000000000000000000000000000000000000000000000");        // 0x1e00ffff
        consensus.powLimit[CBlockHeader::AlgoType::ALGO_POW_SHA256] = uint256S("000000ffff000000000000000000000000000000000000000000000000000000"); // 0x1e00ffff
        consensus.nPowTargetTimespan = 12 * 60 * 60;                                                                                                // 12 hours
        consensus.nPowTargetSpacing = 80;                                                                                                           // 80-second block spacing - must be divisible by (nStakeTimestampMask+1)
        consensus.nStakeTimestampMask = 0xf;                                                                                                        // 16 second time slots
        consensus.nStakeMinDepth = 600;
        consensus.nStakeMinAge = 12 * 60 * 60;      // current minimum age for coin age is 12 hours
        consensus.nStakeMaxAge = 30 * 24 * 60 * 60; // 30 days
        consensus.nModifierInterval = 1 * 60;       // Modifier interval: time to elapse before new modifier is computed
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = (14 * 24 * 60 * 60 * 90) / (100 * consensus.nPowTargetSpacing); // 90% of the blocks in the past two weeks
        consensus.nMinerConfirmationWindow = 14 * 24 * 60 * 60 / consensus.nPowTargetSpacing;                      // nPowTargetTimespan / nPowTargetSpacing
        consensus.nTreasuryPaymentsCycleBlocks = 1 * 24 * 60 * 60 / consensus.nPowTargetSpacing;                   // Once per day

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        // The best chain should have at least this much work.
        //consensus.nMinimumChainWork = uint256S("0x00000000000000000000000000000000000000000e1ab5ec9348e9f4b8eb8154");

        consensus.mTreasuryPayees.emplace(CScript() << OP_0 << ParseHex("978a5064cd1fdf8c2510fe3fcbd65eaa5e98b32d"), 100); // 10% (full reward) for ep1qj799qexdrl0ccfgslcluh4j74f0f3vedatcv0k
        consensus.nTreasuryRewardPercentage = 10;                                                                          // 10% of block reward goes to treasury

        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000004d4a27e34ba8c684ba2b");
        //consensus.defaultAssumeValid = uint256S("0xb7f5913d988682f90be2a534dfc0c7310f71d658a9eb2aa6b6952a424021010a"); // 900000

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xd1;
        pchMessageStart[1] = 0xba;
        pchMessageStart[2] = 0xe1;
        pchMessageStart[3] = 0xf5;
        nDefaultPort = 16817;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 5;
        m_assumed_chain_state_size = 1;

        std::vector<CAmount> genesisRewards;             // premine
        genesisRewards.emplace_back(27000000000 * COIN); // 27 billion
        genesisRewards.emplace_back(1500000000 * COIN);  // 1.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesis = CreateGenesisBlock(1609246800, 10543997, UintToArith256(consensus.powLimit[CBlockHeader::AlgoType::ALGO_POW_SHA256]).GetCompactBase256(), 1, genesisRewards);
        consensus.hashGenesisBlock = genesis.GetHash();
        //printf("Merkle hash mainnet: %s\n", genesis.hashMerkleRoot.ToString().c_str());
        //printf("Genesis hash mainnet: %s\n", consensus.hashGenesisBlock.ToString().c_str());
        assert(genesis.hashMerkleRoot == uint256S("0x951ef417a7e31855adad366ad777b3a4608a7f50679baa54e81a28904097a26f"));
        assert(consensus.hashGenesisBlock == uint256S("0x000000954c02f260a6db02c712557adcb5a7a8a0a9acfd3d3c2b3a427376c56f"));

        // Note that of those which support the service bits prefix, most only support a subset of
        // possible options.
        // This is fine at runtime as we'll fall back to using them as a oneshot if they don't support the
        // service bits we want, but we should get them updated to support all service bits wanted by any
        // release ASAP to avoid it where possible.
        vSeeds.emplace_back("seed01.electraprotocol.eu");
        vSeeds.emplace_back("seed02.electraprotocol.eu");
        vSeeds.emplace_back("seed03.electraprotocol.eu");
        vSeeds.emplace_back("seed04.electraprotocol.eu");
        vSeeds.emplace_back("seed05.electraprotocol.eu");
        vSeeds.emplace_back("seed06.electraprotocol.eu");
        vSeeds.emplace_back("seed07.electraprotocol.eu");
        vSeeds.emplace_back("seed08.electraprotocol.eu");
        vSeeds.emplace_back("xep.zentec.network");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 55);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 137);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 162);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x88, 0xB2, 0x1E};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x88, 0xAD, 0xE4};

        bech32_hrp = "ep";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        m_is_test_chain = false;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {0, uint256S("000000954c02f260a6db02c712557adcb5a7a8a0a9acfd3d3c2b3a427376c56f")},
                {50000, uint256S("505286a87781aabbb6cfc7a9b735ffacd8ce73bc06ed17dae546cafe4ca3e7a3")},
                {100000, uint256S("88e536f2f4dad78b2177694d3b269f2145a5087d677f393a9980a300f746b6bf")},
                {150000, uint256S("a11f28829bedd92e634b249e77d4aa6d1dab10075bf19339d02ccc7ae55bb993")},
                {200000, uint256S("fb31a51ee1893fbaaec42af1ab1f7bee208c62ad3a483e6988b0b65e20d5f9aa")},
                {250000, uint256S("e2291547671d02ef6ab287e5820359404224cc827fe9f67c9e36417597832ff2")},
                {300000, uint256S("46c0269c51758613e434ed68460a14237e783280d4b23328ae64cf6177aca609")},
                {350000, uint256S("7ece4c4e3332cde2a53ef8ebaa1de6744482d946de38aa76586913fb0a97ab05")},
                {400000, uint256S("cf9360a5acf99d45a8d2f86c0f8141734c61088294fb1934b6ca7dce8617968c")},
                {450000, uint256S("5fbff547e15f6ad22cad7dad4a79dd5ed893552ea809a10400cc618e52a2be91")},
                {500000, uint256S("577286d8ba7183398ecefcb7df2ef722c226a888c7a440aa650411802e67b224")},
                {550000, uint256S("a79eff4cc1a1ab2b98d814beea9fc84c9318abf2ab8890e18df4b6996ce48441")},
                {600000, uint256S("d5b72da40a58aad7df1cbea415f63e2e6efce593fd8e7feb0dd6c6cff25f0589")},
                {650000, uint256S("51c070bf0c0c6855cfec74c87354e6633752be05b904948bba28c4438f64a65e")},
                {700000, uint256S("742c77743ce02d6d8cc9b830c14de3d6f0c555f90cd648c6708ef90939789789")},
                {750000, uint256S("7edd43aea64c1eac214e3043921527dfe8ecaa21eb5a8ad8ee0a2ba34eea16fd")},
                {800000, uint256S("0a6a0fc8773fe97d1a0599168468def7ae866f72feeab75d3226873670782903")},
                {850000, uint256S("7a4ad48466bdc20539349421f985dc4026bd7843a8758bd321b029b2f37fc36e")},
                {900000, uint256S("b7f5913d988682f90be2a534dfc0c7310f71d658a9eb2aa6b6952a424021010a")},
            }};

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 32400 b7f5913d988682f90be2a534dfc0c7310f71d658a9eb2aa6b6952a424021010a
            /* nTime    */ 1681004432,
            /* nTxCount */ 1837360,
            /* dTxRate  */ 0.02521959469838949
        };
    }
};

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = CBaseChainParams::TESTNET;
        consensus.nSubsidyHalvingInterval = 210000;
        consensus.nBudgetPaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.nPoSStartBlock = 0;
        consensus.nLastPoWBlock = std::numeric_limits<int>::max();
        consensus.nTreasuryPaymentsStartBlock = 200;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 0;
        consensus.BIP34Hash = uint256S("0x000000954c02f260a6db02c712557adcb5a7a8a0a9acfd3d3c2b3a427376c56f");
        consensus.BIP65Height = 0;
        consensus.BIP66Height = 0;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 0;
        consensus.MinBIP9WarningHeight = 0;                                                                                                         // segwit activation height + miner confirmation window
        consensus.powLimit[CBlockHeader::AlgoType::ALGO_POS] = uint256S("000000ffff000000000000000000000000000000000000000000000000000000");        // 0x1e00ffff
        consensus.powLimit[CBlockHeader::AlgoType::ALGO_POW_SHA256] = uint256S("000000ffff000000000000000000000000000000000000000000000000000000"); // 0x1e00ffff
        consensus.nPowTargetTimespan = 12 * 60 * 60;                                                                                                // 12 hours
        consensus.nPowTargetSpacing = 80;                                                                                                           // 80-second block spacing - must be divisible by (nStakeTimestampMask+1)
        consensus.nStakeTimestampMask = 0xf;                                                                                                        // 16 second time slots
        consensus.nStakeMinDepth = 100;
        consensus.nStakeMinAge = 2 * 60 * 60;       // testnet min age is 2 hours
        consensus.nStakeMaxAge = 30 * 24 * 60 * 60; // 30 days
        consensus.nModifierInterval = 1 * 60;       // Modifier interval: time to elapse before new modifier is computed
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = (14 * 24 * 60 * 60 * 75) / (100 * consensus.nPowTargetSpacing); // 75% for testchains
        consensus.nMinerConfirmationWindow = 14 * 24 * 60 * 60 / consensus.nPowTargetSpacing;                      // nPowTargetTimespan / nPowTargetSpacing
        consensus.nTreasuryPaymentsCycleBlocks = 24 * 6 * 60 / consensus.nPowTargetSpacing;                        // Ten times per day
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = Consensus::BIP9Deployment::NEVER_ACTIVE;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.mTreasuryPayees.emplace(CScript() << OP_0 << ParseHex("978a5064cd1fdf8c2510fe3fcbd65eaa5e98b32d"), 100); // 10% (full reward) for ep1qj799qexdrl0ccfgslcluh4j74f0f3vedatcv0k
        consensus.nTreasuryRewardPercentage = 10;                                                                          // 10% of block reward goes to treasury

        consensus.nMinimumChainWork = uint256S("0x0000000000000000000000000000000000000000000000000000030d440d4400");
        consensus.defaultAssumeValid = uint256S("0x001"); // 200000

        pchMessageStart[0] = 0xdb;
        pchMessageStart[1] = 0xb1;
        pchMessageStart[2] = 0xc9;
        pchMessageStart[3] = 0xa7;
        nDefaultPort = 18317;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 5;
        m_assumed_chain_state_size = 1;

        std::vector<CAmount> genesisRewards;             // premine
        genesisRewards.emplace_back(27000000000 * COIN); // 27 billion
        genesisRewards.emplace_back(1500000000 * COIN);  // 1.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesis = CreateGenesisBlock(1609246800, 10543997, UintToArith256(consensus.powLimit[CBlockHeader::AlgoType::ALGO_POW_SHA256]).GetCompactBase256(), 1, genesisRewards);
        consensus.hashGenesisBlock = genesis.GetHash();
        //printf("Merkle hash testnet: %s\n", genesis.hashMerkleRoot.ToString().c_str());
        //printf("Genesis hash testnet: %s\n", consensus.hashGenesisBlock.ToString().c_str());
        assert(genesis.hashMerkleRoot == uint256S("0x951ef417a7e31855adad366ad777b3a4608a7f50679baa54e81a28904097a26f"));
        assert(consensus.hashGenesisBlock == uint256S("0x000000954c02f260a6db02c712557adcb5a7a8a0a9acfd3d3c2b3a427376c56f"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        vSeeds.emplace_back("seed01.electraprotocol.eu");
        vSeeds.emplace_back("seed02.electraprotocol.eu");
        vSeeds.emplace_back("seed03.electraprotocol.eu");
        vSeeds.emplace_back("seed04.electraprotocol.eu");
        vSeeds.emplace_back("seed05.electraprotocol.eu");
        vSeeds.emplace_back("seed06.electraprotocol.eu");
        vSeeds.emplace_back("seed07.electraprotocol.eu");
        vSeeds.emplace_back("seed08.electraprotocol.eu");
        vSeeds.emplace_back("xep.zentec.network");

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 141);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "te";

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_test, pnSeed6_test + ARRAYLEN(pnSeed6_test));

        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        m_is_test_chain = true;
        m_is_mockable_chain = false;

        checkpointData = {
            {
                {0, uint256S("000000954c02f260a6db02c712557adcb5a7a8a0a9acfd3d3c2b3a427376c56f")},
                {50000, uint256S("80527921e815691fa6c036163b847019da3eef41469b64dc90de120b6cbf3a2f")},
                {100000, uint256S("db0ff8f8967068e6d8478f4994440d344c50014952ad751ada2fb565006a7aaa")},
                {150000, uint256S("ebc04d48a973267b21f3899e5c42b61c0c7ed519627078c6c3b6ef514cfffc52")},
                {200000, uint256S("4a121e7765837b21bcffad979e499ecbf7184fcbf772c34c481433059d0840f8")},
            }};

        chainTxData = ChainTxData{
            // Data from RPC: getchaintxstats 30720 4a121e7765837b21bcffad979e499ecbf7184fcbf772c34c481433059d0840f8
            /* nTime    */ 1639755808,
            /* nTxCount */ 395244,
            /* dTxRate  */ 0.0314994216903049,
        };
    }
};

/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    explicit CRegTestParams(const ArgsManager& args) {
        strNetworkID = CBaseChainParams::REGTEST;
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nBudgetPaymentsStartBlock = std::numeric_limits<int>::max();
        consensus.nPoSStartBlock = 0;
        consensus.nLastPoWBlock = std::numeric_limits<int>::max();
        consensus.nTreasuryPaymentsStartBlock = 30;
        consensus.BIP16Exception = uint256{};
        consensus.BIP34Height = 500; // BIP34 activated on regtest (Used in functional tests)
        consensus.BIP34Hash = uint256{};
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in functional tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in functional tests)
        consensus.CSVHeight = 432;    // CSV activated on regtest (Used in rpc activation tests)
        consensus.SegwitHeight = 0;   // SEGWIT is always activated on regtest unless overridden
        consensus.MinBIP9WarningHeight = 0;
        consensus.powLimit[CBlockHeader::AlgoType::ALGO_POS] = uint256S("7fffff0000000000000000000000000000000000000000000000000000000000");        // 0x207fffff
        consensus.powLimit[CBlockHeader::AlgoType::ALGO_POW_SHA256] = uint256S("7fffff0000000000000000000000000000000000000000000000000000000000"); // 0x207fffff
        consensus.nPowTargetTimespan = 1 * 60 * 60;                                                                                                 // 1 hour
        consensus.nPowTargetSpacing = 80;                                                                                                           // 80-second block spacing - must be divisible by (nStakeTimestampMask+1)
        consensus.nStakeTimestampMask = 0x3;                                                                                                        // 4 second time slots
        consensus.nStakeMinDepth = 0;
        consensus.nStakeMinAge = 1 * 60;            // regtest min age is 1 minute
        consensus.nStakeMaxAge = 30 * 24 * 60 * 60; // 30 days
        consensus.nModifierInterval = 1 * 60;       // Modifier interval: time to elapse before new modifier is computed
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = (24 * 60 * 60 * 75) / (100 * consensus.nPowTargetSpacing); // 75% for testchains
        consensus.nMinerConfirmationWindow = 24 * 60 * 60 / consensus.nPowTargetSpacing;                      // Faster than normal for regtest (one day instead of two weeks)
        consensus.nTreasuryPaymentsCycleBlocks = 20;

        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = Consensus::BIP9Deployment::NO_TIMEOUT;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].min_activation_height = 0; // No activation delay

        consensus.mTreasuryPayees.emplace(CScript() << OP_0 << ParseHex("978a5064cd1fdf8c2510fe3fcbd65eaa5e98b32d"), 100); // 10% (full reward) for ep1qj799qexdrl0ccfgslcluh4j74f0f3vedatcv0k
        consensus.nTreasuryRewardPercentage = 10;                                                                          // 10% of block reward goes to treasury

        consensus.nMinimumChainWork = uint256{};
        consensus.defaultAssumeValid = uint256{};

        pchMessageStart[0] = 0xfa;
        pchMessageStart[1] = 0xbf;
        pchMessageStart[2] = 0xc5;
        pchMessageStart[3] = 0xda;
        nDefaultPort = 18444;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 0;
        m_assumed_chain_state_size = 0;

        UpdateActivationParametersFromArgs(args);

        std::vector<CAmount> genesisRewards;             // premine
        genesisRewards.emplace_back(27000000000 * COIN); // 27 billion
        genesisRewards.emplace_back(1500000000 * COIN);  // 1.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesisRewards.emplace_back(500000000 * COIN);   // 0.5 billion
        genesis = CreateGenesisBlock(1609246800, 14201, UintToArith256(consensus.powLimit[CBlockHeader::AlgoType::ALGO_POW_SHA256]).GetCompactBase256(), 1, genesisRewards);
        consensus.hashGenesisBlock = genesis.GetHash();
        //printf("Merkle hash regtest: %s\n", genesis.hashMerkleRoot.ToString().c_str());
        //printf("Genesis hash regtest: %s\n", consensus.hashGenesisBlock.ToString().c_str());
        assert(genesis.hashMerkleRoot == uint256S("0x74d37252db3a2e1960cb4d62da34954ab26d39e431a8b77afe3dd31d8ddc96b3"));
        assert(consensus.hashGenesisBlock == uint256S("0x00005c7509dcd261eea59d1cbe054f8ad6adb0b783ea4169d22ddba5b3fc6b50"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fDefaultConsistencyChecks = true;
        fRequireStandard = true;
        m_is_test_chain = true;
        m_is_mockable_chain = true;

        checkpointData = {
            {
                {0, uint256S("00005c7509dcd261eea59d1cbe054f8ad6adb0b783ea4169d22ddba5b3fc6b50")},
            }};

        chainTxData = ChainTxData{
            0,
            0,
            0};

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 141);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 19);
        base58Prefixes[SECRET_KEY] = std::vector<unsigned char>(1, 239);
        base58Prefixes[EXT_PUBLIC_KEY] = {0x04, 0x35, 0x87, 0xCF};
        base58Prefixes[EXT_SECRET_KEY] = {0x04, 0x35, 0x83, 0x94};

        bech32_hrp = "eprt";
    }

    /**
     * Allows modifying the Version Bits regtest parameters.
     */
    void UpdateVersionBitsParameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
    void UpdateActivationParametersFromArgs(const ArgsManager& args);
};

void CRegTestParams::UpdateActivationParametersFromArgs(const ArgsManager& args)
{
    if (args.IsArgSet("-segwitheight")) {
        int64_t height = args.GetArg("-segwitheight", consensus.SegwitHeight);
        if (height < -1 || height >= std::numeric_limits<int>::max()) {
            throw std::runtime_error(strprintf("Activation height %ld for segwit is out of valid range. Use -1 to disable segwit.", height));
        } else if (height == -1) {
            LogPrintf("Segwit disabled for testing\n");
            height = std::numeric_limits<int>::max();
        }
        consensus.SegwitHeight = static_cast<int>(height);
    }

    if (!args.IsArgSet("-vbparams")) return;

    for (const std::string& strDeployment : args.GetArgs("-vbparams")) {
        std::vector<std::string> vDeploymentParams;
        boost::split(vDeploymentParams, strDeployment, boost::is_any_of(":"));
        if (vDeploymentParams.size() < 3 || 4 < vDeploymentParams.size()) {
            throw std::runtime_error("Version bits parameters malformed, expecting deployment:start:end[:min_activation_height]");
        }
        int64_t nStartTime, nTimeout;
        int min_activation_height = 0;
        if (!ParseInt64(vDeploymentParams[1], &nStartTime)) {
            throw std::runtime_error(strprintf("Invalid nStartTime (%s)", vDeploymentParams[1]));
        }
        if (!ParseInt64(vDeploymentParams[2], &nTimeout)) {
            throw std::runtime_error(strprintf("Invalid nTimeout (%s)", vDeploymentParams[2]));
        }
        if (vDeploymentParams.size() >= 4 && !ParseInt32(vDeploymentParams[3], &min_activation_height)) {
            throw std::runtime_error(strprintf("Invalid min_activation_height (%s)", vDeploymentParams[3]));
        }
        bool found = false;
        for (int j = 0; j < (int)Consensus::MAX_VERSION_BITS_DEPLOYMENTS; ++j) {
            if (vDeploymentParams[0] == VersionBitsDeploymentInfo[j].name) {
                UpdateVersionBitsParameters(Consensus::DeploymentPos(j), nStartTime, nTimeout);
                found = true;
                LogPrintf("Setting version bits activation parameters for %s to start=%ld, timeout=%ld, min_activation_height=%d\n", vDeploymentParams[0], nStartTime, nTimeout, min_activation_height);
                break;
            }
        }
        if (!found) {
            throw std::runtime_error(strprintf("Invalid deployment (%s)", vDeploymentParams[0]));
        }
    }
}

static std::unique_ptr<const CChainParams> globalChainParams;

const CChainParams& Params()
{
    assert(globalChainParams);
    return *globalChainParams;
}

std::unique_ptr<const CChainParams> CreateChainParams(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
        return std::unique_ptr<CChainParams>(new CMainParams());
    else if (chain == CBaseChainParams::TESTNET)
        return std::unique_ptr<CChainParams>(new CTestNetParams());
    else if (chain == CBaseChainParams::REGTEST)
        return std::unique_ptr<CChainParams>(new CRegTestParams(gArgs));
    throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    SelectBaseParams(network);
    globalChainParams = CreateChainParams(network);
}
