Omni Core (beta) integration/staging tree
=========================================

What is the Omni Layer
----------------------
The Omni Layer is a communications protocol that uses the Xep block chain to enable features such as smart contracts, user currencies and decentralized peer-to-peer exchanges. A common analogy that is used to describe the relation of the Omni Layer to Xep is that of HTTP to TCP/IP: HTTP, like the Omni Layer, is the application layer to the more fundamental transport and internet layer of TCP/IP, like Xep.

http://www.omnilayer.org

What is Omni Core
-----------------

Omni Core is a fast, portable Omni Layer implementation that is based off the Xep Core codebase (currently 0.20.1). This implementation requires no external dependencies extraneous to Xep Core, and is native to the Xep network just like other Xep nodes. It currently supports a wallet mode and is seamlessly available on three platforms: Windows, Linux and Mac OS. Omni Layer extensions are exposed via the JSON-RPC interface. Development has been consolidated on the Omni Core product, and it is the reference client for the Omni Layer.

Disclaimer, warning
-------------------
This software is EXPERIMENTAL software. USE ON MAINNET AT YOUR OWN RISK.

By default this software will use your existing Xep wallet, including spending xeps contained therein (for example for transaction fees or trading).
The protocol and transaction processing rules for the Omni Layer are still under active development and are subject to change in future.
Omni Core should be considered an alpha-level product, and you use it at your own risk. Neither the Omni Foundation nor the Omni Core developers assumes any responsibility for funds misplaced, mishandled, lost, or misallocated.

Further, please note that this installation of Omni Core should be viewed as EXPERIMENTAL. Your wallet data, xeps and Omni Layer tokens may be lost, deleted, or corrupted, with or without warning due to bugs or glitches. Please take caution.

This software is provided open-source at no cost. You are responsible for knowing the law in your country and determining if your use of this software contravenes any local laws.

PLEASE DO NOT use wallet(s) with significant amounts of xeps or Omni Layer tokens while testing!

Dependencies
------------
Boost >= 1.53

Installation
------------

You will need appropriate libraries to run Omni Core on Unix,
please see [doc/build-unix.md](doc/build-unix.md) for the full listing.

You will need to install git & pkg-config:

```
sudo apt-get install git
sudo apt-get install pkg-config
```

Clone the Omni Core repository:

```
git clone https://github.com/Jenova7/omnixep.git
cd omnixep/
```

Then, run:

```
./autogen.sh
./configure
make
```
Once complete:

```
cd src/
```
And start Omni Core using `./omnixepd` (or `./qt/omnixep-qt` if built with UI). The initial parse step for a first time run
will take up to 60 minutes or more, during this time your client will scan the blockchain for Omni Layer transactions. You can view the
output of the parsing at any time by viewing the log located in your datadir, by default: `~/.omnixep/omnixep.log`.

Omni Core requires the transaction index to be enabled. Add an entry to your xep.conf file for `txindex=1` to enable it or Omni Core will refuse to start.

If a message is returned asking you to reindex, pass the `-reindex` flag as startup option. The reindexing process can take several hours.

To issue RPC commands to Omni Core you may add the `-server=1` CLI flag or add an entry to the xep.conf file (located in `~/.omnixep/` by default).

In xep.conf:
```
server=1
```

After this step completes, check that the installation went smoothly by issuing the following command `./omnixep-cli omni_getinfo` which should return the `omnicoreversion` as well as some
additional information related to the client.

The documentation for the RPC interface and command-line is located in [src/omnicore/doc/rpc-api.md] (src/omnicore/doc/rpc-api.md).

Current feature set:
--------------------

* Broadcasting of simple send (tx 0) [doc] (src/omnicore/doc/rpc-api.md#omni_send), and send to owners (tx 3) [doc] (src/omnicore/doc/rpc-api.md#omni_sendsto)

* Obtaining a Omni Layer balance [doc] (src/omnicore/doc/rpc-api.md#omni_getbalance)

* Obtaining all balances (including smart property) for an address [doc] (src/omnicore/doc/rpc-api.md#omni_getallbalancesforaddress)

* Obtaining all balances associated with a specific smart property [doc] (src/omnicore/doc/rpc-api.md#omni_getallbalancesforid)

* Retrieving information about any Omni Layer transaction [doc] (src/omnicore/doc/rpc-api.md#omni_gettransaction)

* Listing historical transactions of addresses in the wallet [doc] (src/omnicore/doc/rpc-api.md#omni_listtransactions)

* Retrieving detailed information about a smart property [doc] (src/omnicore/doc/rpc-api.md#omni_getproperty)

* Retrieving active and expired crowdsale information [doc] (src/omnicore/doc/rpc-api.md#omni_getcrowdsale)

* Sending a specific XEP amount to a receiver with referenceamount in `omni_send`

* Creating and broadcasting transactions based on raw Omni Layer transactions with `omni_sendrawtx`

* Functional UI for balances, sending and historical transactions

* Creating any supported transaction type via RPC interface

* Meta-DEx integration

* Support for class B (multisig) and class C (op-return) encoded transactions

* Support of unconfirmed transactions

* Creation of raw transactions with non-wallet inputs

