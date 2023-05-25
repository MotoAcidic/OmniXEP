#ifndef XEP_OMNICORE_UTILSXEP_H
#define XEP_OMNICORE_UTILSXEP_H

class CBlockIndex;
class uint256;

#include <stdint.h>

namespace mastercore
{
/** Returns the current chain length. */
int GetHeight();
/** Returns the timestamp of the latest block. */
uint32_t GetLatestBlockTime();
/** Returns the CBlockIndex for a given block hash, or NULL. */
CBlockIndex* GetBlockIndex(const uint256& hash);

bool MainNet();
bool TestNet();
bool RegTest();
bool UnitTest();
bool isNonMainNet();
}

#endif // XEP_OMNICORE_UTILSXEP_H
