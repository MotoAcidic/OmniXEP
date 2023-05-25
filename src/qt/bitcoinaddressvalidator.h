// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef XEP_QT_XEPADDRESSVALIDATOR_H
#define XEP_QT_XEPADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class XepAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit XepAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Xep address widget validator, checks for a valid bitcoin address.
 */
class XepAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit XepAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // XEP_QT_XEPADDRESSVALIDATOR_H
