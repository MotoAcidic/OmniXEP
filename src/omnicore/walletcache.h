#ifndef XEP_OMNICORE_WALLETCACHE_H
#define XEP_OMNICORE_WALLETCACHE_H

class uint256;

#include <vector>

namespace mastercore
{
/** Updates the cache and returns whether any wallet addresses were changed */
int WalletCacheUpdate();
}

#endif // XEP_OMNICORE_WALLETCACHE_H
