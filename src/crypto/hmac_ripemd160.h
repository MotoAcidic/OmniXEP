// Copyright (c) 2014-2018 The Xep Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XEP_CRYPTO_HMAC_RIPEMD160_H
#define XEP_CRYPTO_HMAC_RIPEMD160_H

#include <crypto/ripemd160.h>

#include <stdint.h>
#include <stdlib.h>

/** A hasher class for HMAC-RIPEMD-160. */
class CHMAC_RIPEMD160
{
private:
    CRIPEMD160 outer;
    CRIPEMD160 inner;

public:
    static const size_t OUTPUT_SIZE = 20;

    CHMAC_RIPEMD160(const unsigned char* key, size_t keylen);
    CHMAC_RIPEMD160& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(unsigned char hash[OUTPUT_SIZE]);
};

#endif // XEP_CRYPTO_HMAC_RIPEMD160_H
