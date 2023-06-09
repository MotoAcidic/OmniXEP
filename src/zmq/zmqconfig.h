// Copyright (c) 2014-2019 The Xep Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XEP_ZMQ_ZMQCONFIG_H
#define XEP_ZMQ_ZMQCONFIG_H

#if defined(HAVE_CONFIG_H)
#include <config/xep-config.h>
#endif

#include <stdarg.h>

#if ENABLE_ZMQ
#include <zmq.h>
#endif

#include <primitives/transaction.h>

void zmqError(const char *str);

#endif // XEP_ZMQ_ZMQCONFIG_H
