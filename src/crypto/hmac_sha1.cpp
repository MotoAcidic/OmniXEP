// Copyright (c) 2014-2018 The Xep Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <crypto/hmac_sha1.h>

#include <string.h>

CHMAC_SHA1::CHMAC_SHA1(const unsigned char* key, size_t keylen)
{
    unsigned char rkey[64];
    if (keylen <= 64) {
        memcpy(rkey, key, keylen);
        memset(rkey + keylen, 0, 64 - keylen);
    } else {
        CSHA1().Write(key, keylen).Finalize(rkey);
        memset(rkey + 20, 0, 20);
    }

    for (int n = 0; n < 64; n++)
        rkey[n] ^= 0x5c;
    outer.Write(rkey, 64);

    for (int n = 0; n < 64; n++)
        rkey[n] ^= 0x5c ^ 0x36;
    inner.Write(rkey, 64);
}

void CHMAC_SHA1::Finalize(unsigned char hash[OUTPUT_SIZE])
{
    unsigned char temp[20];
    inner.Finalize(temp);
    outer.Write(temp, 20).Finalize(hash);
}
