// Copyright (c) 2014-2015 The EvolveChainX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVOLVECHAINADDRESSVALIDATOR_H
#define EVOLVECHAINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class EvolveChainAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit EvolveChainAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** EvolveChain address widget validator, checks for a valid evolvechain address.
 */
class EvolveChainAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit EvolveChainAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // EVOLVECHAINADDRESSVALIDATOR_H
