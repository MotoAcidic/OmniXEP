// Copyright (c) 2011-2013 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"

  #include "wallet.h"

  // potentially overzealous includes here
  #include "base58.h"
  #include "rpcserver.h"
  #include "init.h"
  #include "util.h"
  #include <fstream>
  #include <algorithm>
  #include <vector>
  #include <utility>
  #include <string>
  #include <boost/assign/list_of.hpp>
  #include <boost/algorithm/string.hpp>
  #include <boost/algorithm/string/find.hpp>
  #include <boost/algorithm/string/join.hpp>
  #include <boost/lexical_cast.hpp>
  #include <boost/format.hpp>
  #include <boost/filesystem.hpp>
  #include "json/json_spirit_utils.h"
  #include "json/json_spirit_value.h"
  #include "leveldb/db.h"
  #include "leveldb/write_batch.h"
  // end potentially overzealous includes
  using namespace json_spirit; // since now using Array in mastercore.h this needs to come first

  #include "mastercore.h"
  using namespace mastercore;

  // potentially overzealous using here
  using namespace std;
  using namespace boost;
  using namespace boost::assign;
  using namespace leveldb;
  // end potentially overzealous using

  #include "mastercore_dex.h"
  #include "mastercore_tx.h"
  #include "mastercore_sp.h"

//#include "overviewlistdelegate.h"
#include <QAbstractItemDelegate>
#include <QPainter>

#define DECORATION_SIZE 64
#define NUM_ITEMS 6 // 3 - number of recent transactions to display

  extern uint64_t global_balance_money_maineco[100000];
  extern uint64_t global_balance_reserved_maineco[100000];
  extern uint64_t global_balance_money_testeco[100000];
  extern uint64_t global_balance_reserved_testeco[100000];

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate(): QAbstractItemDelegate(), unit(BitcoinUnits::BTC)
    {

    }

    inline void paint(QPainter *painter, const QStyleOptionViewItem &option,
                      const QModelIndex &index ) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2*ypad)/2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top()+ypad, mainRect.width() - xspace, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top()+ypad+halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();
        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = option.palette.color(QPalette::Text);
        if(value.canConvert<QBrush>())
        {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft|Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool())
        {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top()+ypad+halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(amount < 0)
        {
            foreground = COLOR_NEGATIVE;
        }
        else if(!confirmed)
        {
            foreground = COLOR_UNCONFIRMED;
        }
        else
        {
            foreground = option.palette.color(QPalette::Text);
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if(!confirmed)
        {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight|Qt::AlignVCenter, amountText);

        painter->setPen(option.palette.color(QPalette::Text));
        painter->drawText(amountRect, Qt::AlignLeft|Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;

};
#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OverviewPage),
    clientModel(0),
    walletModel(0),
    currentBalance(-1),
    currentUnconfirmedBalance(-1),
    currentImmatureBalance(-1),
    currentWatchOnlyBalance(-1),
    currentWatchUnconfBalance(-1),
    currentWatchImmatureBalance(-1),
    txdelegate(new TxViewDelegate()),
    filter(0)
{
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");

    // make sure BTC and MSC are always first in the list by adding them first
    UpdatePropertyBalance(0,0,0);
    UpdatePropertyBalance(1,0,0);

    updateOmni();

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);
}

void OverviewPage::handleTransactionClicked(const QModelIndex &index)
{
    if(filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::UpdatePropertyBalance(unsigned int propertyId, uint64_t available, uint64_t reserved)
{
    // look for this property, does it already exist in overview and if so are the balances correct?
    int existingItem = -1;
    for(int i=0; i < ui->overviewLW->count(); i++) {
        uint64_t itemPropertyId = ui->overviewLW->item(i)->data(Qt::UserRole + 1).value<uint64_t>();
        if (itemPropertyId == propertyId) {
            uint64_t itemAvailableBalance = ui->overviewLW->item(i)->data(Qt::UserRole + 2).value<uint64_t>();
            uint64_t itemReservedBalance = ui->overviewLW->item(i)->data(Qt::UserRole + 3).value<uint64_t>();
            if ((available == itemAvailableBalance) && (reserved == itemReservedBalance)) {
                return; // norhing more to do, balance exists and is up to date
            } else {
                existingItem = i;
                break;
            }
        }
    }

    // this property doesn't exist in overview, create an entry for it
    QWidget *listItem = new QWidget();
    QVBoxLayout *vlayout = new QVBoxLayout();
    QHBoxLayout *hlayout = new QHBoxLayout();
    bool divisible = false;
    string tokenStr;
    // property label
    string spName = getPropertyName(propertyId).c_str();
    if(spName.size()>22) spName=spName.substr(0,22)+"...";
    spName += " (#" + static_cast<ostringstream*>( &(ostringstream() << propertyId) )->str() + ")";
    QLabel *propLabel = new QLabel(QString::fromStdString(spName));
    propLabel->setStyleSheet("QLabel { font-weight:bold; }");
    vlayout->addWidget(propLabel);
    // customizations based on property
    if(propertyId == 0) { divisible = true; } else { divisible = isPropertyDivisible(propertyId); } // override for bitcoin
    if(propertyId == 0) {tokenStr = " BTC";} else {if(propertyId == 1) {tokenStr = " MSC";} else {if(propertyId ==2) {tokenStr = " TMSC";} else {tokenStr = " SPT";}}}
    // Left Panel
    QVBoxLayout *vlayoutleft = new QVBoxLayout();
    QLabel *balReservedLabel = new QLabel;
    if(propertyId != 0) { balReservedLabel->setText("Reserved:"); } else { balReservedLabel->setText("Pending:"); propLabel->setText("Bitcoin"); } // override for bitcoin
    QLabel *balAvailableLabel = new QLabel("Available:");
    QLabel *balTotalLabel = new QLabel("Total:");
    vlayoutleft->addWidget(balReservedLabel);
    vlayoutleft->addWidget(balAvailableLabel);
    vlayoutleft->addWidget(balTotalLabel);
    // Right panel
    QVBoxLayout *vlayoutright = new QVBoxLayout();
    QLabel *balReservedLabelAmount = new QLabel();
    QLabel *balAvailableLabelAmount = new QLabel();
    QLabel *balTotalLabelAmount = new QLabel();
    if(divisible) {
        balReservedLabelAmount->setText(QString::fromStdString(FormatDivisibleMP(reserved) + tokenStr));
        balAvailableLabelAmount->setText(QString::fromStdString(FormatDivisibleMP(available) + tokenStr));
        balTotalLabelAmount->setText(QString::fromStdString(FormatDivisibleMP(available+reserved) + tokenStr));
    } else {
        balReservedLabelAmount->setText(QString::fromStdString(FormatIndivisibleMP(reserved) + tokenStr));
        balAvailableLabelAmount->setText(QString::fromStdString(FormatIndivisibleMP(available) + tokenStr));
        balTotalLabelAmount->setText(QString::fromStdString(FormatIndivisibleMP(available+reserved) + tokenStr));
    }
    balReservedLabelAmount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    balAvailableLabelAmount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    balTotalLabelAmount->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    balReservedLabelAmount->setStyleSheet("QLabel { padding-right:2px; }");
    balAvailableLabelAmount->setStyleSheet("QLabel { padding-right:2px; }");
    balTotalLabelAmount->setStyleSheet("QLabel { padding-right:2px; font-weight:bold; }");
    vlayoutright->addWidget(balReservedLabelAmount);
    vlayoutright->addWidget(balAvailableLabelAmount);
    vlayoutright->addWidget(balTotalLabelAmount);
    // put together
    vlayoutleft->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Fixed,QSizePolicy::Expanding));
    vlayoutright->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Fixed,QSizePolicy::Expanding));
    vlayoutleft->setContentsMargins(0,0,0,0);
    vlayoutright->setContentsMargins(0,0,0,0);
    vlayoutleft->setMargin(0);
    vlayoutright->setMargin(0);
    vlayoutleft->setSpacing(3);
    vlayoutright->setSpacing(3);
    hlayout->addLayout(vlayoutleft);
    hlayout->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Expanding,QSizePolicy::Fixed));
    hlayout->addLayout(vlayoutright);
    hlayout->setContentsMargins(0,0,0,0);
    vlayout->addLayout(hlayout);
    vlayout->addSpacerItem(new QSpacerItem(1,10,QSizePolicy::Fixed,QSizePolicy::Fixed));
    vlayout->setMargin(0);
    vlayout->setSpacing(3);
    listItem->setLayout(vlayout);
    listItem->setContentsMargins(0,0,0,0);
    listItem->layout()->setContentsMargins(0,0,0,0);
    // set data
    if(existingItem == -1) { // new
        QListWidgetItem *item = new QListWidgetItem();
        item->setData(Qt::UserRole + 1, QVariant::fromValue<qulonglong>(propertyId));
        item->setData(Qt::UserRole + 2, QVariant::fromValue<qulonglong>(available));
        item->setData(Qt::UserRole + 3, QVariant::fromValue<qulonglong>(reserved));
        item->setSizeHint(QSize(0,listItem->sizeHint().height())); // resize
        // add the entry
        ui->overviewLW->addItem(item);
        ui->overviewLW->setItemWidget(item, listItem);
    } else {
        ui->overviewLW->item(existingItem)->setData(Qt::UserRole + 2, QVariant::fromValue<qulonglong>(available));
        ui->overviewLW->item(existingItem)->setData(Qt::UserRole + 3, QVariant::fromValue<qulonglong>(reserved));
        ui->overviewLW->setItemWidget(ui->overviewLW->item(existingItem), listItem);
    }
}

void OverviewPage::updateOmni()
{
    // force a refresh of wallet totals
    set_wallet_totals();
    // always show MSC
    UpdatePropertyBalance(1,global_balance_money_maineco[1],global_balance_reserved_maineco[1]);
    // loop properties and update overview
    unsigned int propertyId;
    unsigned int maxPropIdMainEco = GetNextPropertyId(true);  // these allow us to end the for loop at the highest existing
    unsigned int maxPropIdTestEco = GetNextPropertyId(false); // property ID rather than a fixed value like 100000 (optimization)
    // main eco
    for (propertyId = 2; propertyId < maxPropIdMainEco; propertyId++) {
        if ((global_balance_money_maineco[propertyId] > 0) || (global_balance_reserved_maineco[propertyId] > 0)) {
            UpdatePropertyBalance(propertyId,global_balance_money_maineco[propertyId],global_balance_reserved_maineco[propertyId]);
        }
    }
    // test eco
    for (propertyId = 2147483647; propertyId < maxPropIdTestEco; propertyId++) {
        if ((global_balance_money_testeco[propertyId-2147483647] > 0) || (global_balance_reserved_testeco[propertyId-2147483647] > 0)) {
            UpdatePropertyBalance(propertyId,global_balance_money_testeco[propertyId-2147483647],global_balance_reserved_testeco[propertyId-2147483647]);
        }
    }
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance, const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    // Mastercore alerts come as block transactions and do not trip bitcoin alertsChanged() signal so let's check the
    // alert status with the update balance signal that comes in after each block to see if it had any alerts in it
    updateAlerts();

    int unit = walletModel->getOptionsModel()->getDisplayUnit();
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;
/*
    ui->labelBalance->setText(BitcoinUnits::formatWithUnit(unit, balance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::formatWithUnit(unit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::formatWithUnit(unit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::formatWithUnit(unit, balance + unconfirmedBalance + immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchAvailable->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::formatWithUnit(unit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::formatWithUnit(unit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::formatWithUnit(unit, watchOnlyBalance + watchUnconfBalance + watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    // only show immature (newly mined) balance if it's non-zero, so as not to complicate things
    // for the non-mining users
    bool showImmature = immatureBalance != 0;
    bool showWatchOnlyImmature = watchImmatureBalance != 0;
    // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature); // show watch-only immature balance
*/
    // instead simply pass the values to UpdatePropertyBalance - no support for watch-only yet
    UpdatePropertyBalance(0,balance,unconfirmedBalance);
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
/*
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->lineWatchBalance->setVisible(showWatchOnly);    // show watch-only balance separator line
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly)
        ui->labelWatchImmature->hide();
*/
}

void OverviewPage::setClientModel(ClientModel *model)
{
    this->clientModel = model;
    if(model)
    {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts();

        // Refresh Omni info if there have been Omni layer transactions
        connect(model, SIGNAL(refreshOmniState()), this, SLOT(updateOmni()));
    }
}

void OverviewPage::setWalletModel(WalletModel *model)
{
    this->walletModel = model;
    if(model && model->getOptionsModel())
    {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Status, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(setBalance(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)));

        // Refresh Omni information in case this was an internal Omni transaction
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(updateOmni()));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("BTC")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if(walletModel && walletModel->getOptionsModel())
    {
        if(currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance,
                       currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = walletModel->getOptionsModel()->getDisplayUnit();

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts()
{
    // init variables
    bool showAlert = false;
    QString totalMessage;
    // override to check alert directly rather than passing in param as we won't always be calling from bitcoin in
    // the clientmodel emit for alertsChanged
    QString warnings = QString::fromStdString(GetWarnings("statusbar")); // get current bitcoin alert/warning directly
    QString alertMessage = QString::fromStdString(getMasterCoreAlertTextOnly()); // just return the text message from alert
    // any BitcoinCore or MasterCore alerts to display?
    if((!alertMessage.isEmpty()) || (!warnings.isEmpty())) showAlert = true;
    this->ui->labelAlerts->setVisible(showAlert);
    // check if we have a Bitcoin alert to display
    if(!warnings.isEmpty()) totalMessage = warnings + "\n";
    // check if we have a MasterProtocol alert to display
    if(!alertMessage.isEmpty()) totalMessage += alertMessage;
    // display the alert if needed
    if(showAlert) { this->ui->labelAlerts->setText(totalMessage); }
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}
