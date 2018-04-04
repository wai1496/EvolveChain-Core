// Copyright (c) 2014-2015 The EvolveChainX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <QMessageBox>
#include <QScrollBar>
#include <QTextDocument>

#include "sendcoinsdialog.h"
#include "ui_sendcoinsdialog.h"
#include "init.h"
#include "addresstablemodel.h"
#include "evolvechainunits.h"
#include "coincontroldialog.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include "sendcoinsentry.h"
#include "walletmodel.h"
#include "main.h"
#include "base58.h"
#include "coincontrol.h"
#include "ui_interface.h"
#include "wallet.h"
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

#include <boost/asio.hpp>

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

void sendcoins_writeIdentificationLog(std::string MessageStr,std::string Str);
string sendcoins_get_http(const string &server,const string &path);


SendCoinsDialog::SendCoinsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendCoinsDialog),
    model(0)
{
    ui->setupUi(this);

#ifdef Q_OS_MAC // Icons on push buttons are very uncommon on Mac
    ui->addButton->setIcon(QIcon());
    ui->clearButton->setIcon(QIcon());
    ui->sendButton->setIcon(QIcon());
#endif

    GUIUtil::setupAddressWidget(ui->lineEditCoinControlChange, this);

    addEntry();

    connect(ui->addButton, SIGNAL(clicked()), this, SLOT(addEntry()));
    connect(ui->clearButton, SIGNAL(clicked()), this, SLOT(clear()));

    // Coin Control
    connect(ui->pushButtonCoinControl, SIGNAL(clicked()), this, SLOT(coinControlButtonClicked()));
    connect(ui->checkBoxCoinControlChange, SIGNAL(stateChanged(int)), this, SLOT(coinControlChangeChecked(int)));
    connect(ui->lineEditCoinControlChange, SIGNAL(textEdited(const QString &)), this, SLOT(coinControlChangeEdited(const QString &)));

    // Coin Control: clipboard actions
    QAction *clipboardQuantityAction = new QAction(tr("Copy quantity"), this);
    QAction *clipboardAmountAction = new QAction(tr("Copy amount"), this);
    QAction *clipboardFeeAction = new QAction(tr("Copy fee"), this);
    QAction *clipboardAfterFeeAction = new QAction(tr("Copy after fee"), this);
    QAction *clipboardBytesAction = new QAction(tr("Copy bytes"), this);
    QAction *clipboardPriorityAction = new QAction(tr("Copy priority"), this);
    QAction *clipboardLowOutputAction = new QAction(tr("Copy low output"), this);
    QAction *clipboardChangeAction = new QAction(tr("Copy change"), this);
    connect(clipboardQuantityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardQuantity()));
    connect(clipboardAmountAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAmount()));
    connect(clipboardFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardFee()));
    connect(clipboardAfterFeeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardAfterFee()));
    connect(clipboardBytesAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardBytes()));
    connect(clipboardPriorityAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardPriority()));
    connect(clipboardLowOutputAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardLowOutput()));
    connect(clipboardChangeAction, SIGNAL(triggered()), this, SLOT(coinControlClipboardChange()));
    ui->labelCoinControlQuantity->addAction(clipboardQuantityAction);
    ui->labelCoinControlAmount->addAction(clipboardAmountAction);
    ui->labelCoinControlFee->addAction(clipboardFeeAction);
    ui->labelCoinControlAfterFee->addAction(clipboardAfterFeeAction);
    ui->labelCoinControlBytes->addAction(clipboardBytesAction);
    ui->labelCoinControlPriority->addAction(clipboardPriorityAction);
    ui->labelCoinControlLowOutput->addAction(clipboardLowOutputAction);
    ui->labelCoinControlChange->addAction(clipboardChangeAction);

    fNewRecipientAllowed = true;
}

void SendCoinsDialog::setModel(WalletModel *model)
{
    this->model = model;

    if(model && model->getOptionsModel())
    {
        for(int i = 0; i < ui->entries->count(); ++i)
        {
            SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
            if(entry)
            {
                entry->setModel(model);
            }
        }

        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance());
        connect(model, SIGNAL(balanceChanged(qint64, qint64, qint64)), this, SLOT(setBalance(qint64, qint64, qint64)));
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        // Coin Control
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(coinControlUpdateLabels()));
        connect(model->getOptionsModel(), SIGNAL(coinControlFeaturesChanged(bool)), this, SLOT(coinControlFeatureChanged(bool)));
        connect(model->getOptionsModel(), SIGNAL(transactionFeeChanged(qint64)), this, SLOT(coinControlUpdateLabels()));
        ui->frameCoinControl->setVisible(model->getOptionsModel()->getCoinControlFeatures());
        coinControlUpdateLabels();
    }
}

SendCoinsDialog::~SendCoinsDialog()
{
    delete ui;
}

void SendCoinsDialog::on_sendButton_clicked()
{
    if(!model || !model->getOptionsModel())
        return;
	
	
	if(addressIdentified==false){
		QMessageBox::warning ( this, "Alert","Please enter evolvechain address, search email or phone before sending.", QMessageBox::Ok);
		return;
	}
	
	///////////////////////////////////////////////***************************************************
    QList<SendCoinsRecipient> recipients;
    bool valid = true;
	
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            if(entry->validate())
            {
                recipients.append(entry->getValue());
            }
            else
            {
                valid = false;
            }
        }
		
	}
	
	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;

	QString questionString = tr("Are you sure you want to send?"); /// Main Question string dialog
	QString recipientElementTax; /// Tax Element QString
	QString recipientElementTaxTo; /// Tax Element QString


	/// First, get from country!
	std::string parameters = "/search/code/" + identiCode ;	
	
	try
	{
		EvolveChainJSON << sendcoins_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		QMessageBox::warning ( this, "Alert","Unable to check the tax.", QMessageBox::Ok);
		return ;
	}
	
	if(fListening==true)
	{
		// JSON ****************************************
		using boost::property_tree::ptree;
		using boost::property_tree::read_json;
		using boost::property_tree::write_json;
		ptree NewPointer;

		read_json (EvolveChainJSON, NewPointer);
		std::string strSuccess = NewPointer.get<std::string> ("success");
		
		
		if(strSuccess=="1")
		{
			fromCountry = NewPointer.get<std::string> ("country");
			std::string Method = "sendcoinsdialog ";
			sendcoins_writeIdentificationLog("Method:",Method);
			sendcoins_writeIdentificationLog("**Success","From Country: "+ fromCountry);
		}else{
			QMessageBox::warning ( this, "Alert","Unable to confirm your identification a this time, try again!", QMessageBox::Ok);
			sendcoins_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
		}
	}
    qint64 totalTax = 0;
	QStringList formatted;
	QList<std::pair<QString,qint64> > taxInformation;		
	taxInformation.clear();
	
    foreach(const SendCoinsRecipient &rcp, recipients) // the main loop to check all the recipients are checked for tax
    {
		
		float rate = 0; ///this is the tax rate (in percentage)
		std::string taxAddress = ""; /// this is the address where tax is deposited
		std::string taxEmail = ""; /// this is the email address where tax is deposited
		float tax = 0; /// the is the tax amount
		
		qreal rateTo = 0; ///this is the tax rate (in percentage)
		std::string taxAddressTo = ""; /// this is the address where tax is deposited
		std::string taxEmailTo = ""; /// this is the email address where tax is deposited
		float taxTo = 0; /// the is the tax amount
		
		///Calculate in a loop for each recipient
		// sendcoins_writeIdentificationLog("**Success","toAddress: " + rcp.address.toStdString());			
		/// Get tax for each countries
		parameters = "/verify/address/" + rcp.address.toStdString();	/// This will get the country to the recipient
		try
		{
			EvolveChainJSON << sendcoins_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			QMessageBox::warning ( this, "Alert","Unable to check the tax.", QMessageBox::Ok);
			return ;
		}
		
		if(fListening==true)
		{ 
			// JSON ****************************************
			using boost::property_tree::ptree;
			using boost::property_tree::read_json;
			using boost::property_tree::write_json;
			ptree NewPointer;

			read_json (EvolveChainJSON, NewPointer);
			std::string strSuccess = NewPointer.get<std::string> ("success");
		
			if(strSuccess=="1")
			{
				toCountry = NewPointer.get<std::string> ("country");
				std::string Method = "sendcoinsdialog ";
				sendcoins_writeIdentificationLog("Method:",Method);
				sendcoins_writeIdentificationLog("**Success","To Country: "+ toCountry);			
			}else{
				sendcoins_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
				QMessageBox::warning ( this, "Alert","Unable to confirm your identification a this time, try again!", QMessageBox::Ok);
			}
		}
		
		parameters = "/search/tax/" + fromCountry + "/" + toCountry +  "/" + QString::number(rcp.amount).toStdString();	/// This will get the tax information of the recipient country and calculate accordingly.
		try
		{
			EvolveChainJSON << sendcoins_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			QMessageBox::warning ( this, "Alert","Unable to check the tax.", QMessageBox::Ok);
			return ;
		}
		if(fListening==true)
		{
			// JSON ****************************************
			using boost::property_tree::ptree;
			using boost::property_tree::read_json;
			using boost::property_tree::write_json;
			ptree NewPointer;

			read_json (EvolveChainJSON, NewPointer);
			std::string strSuccess = NewPointer.get<std::string> ("success");
			
			if(strSuccess=="0"){
				std::string strError = NewPointer.get<std::string> ("error");
				QMessageBox::warning ( this, "Alert",QString::fromUtf8(strError.c_str()), QMessageBox::Ok);
				return ;
			}

			if(strSuccess=="1")
			{
				rate = NewPointer.get<float> ("rate");
				taxAddress = NewPointer.get<std::string> ("address");
				taxEmail = NewPointer.get<std::string> ("email");
				rateTo = NewPointer.get<float> ("rateTo");
				taxAddressTo = NewPointer.get<std::string> ("addressTo");
				taxEmailTo = NewPointer.get<std::string> ("emailTo");
				std::string Method = "taxCalculation ";
				
				sendcoins_writeIdentificationLog("Method:",Method);
				sendcoins_writeIdentificationLog("**Success","Amount: " + QString::number(rcp.amount).toStdString());
				sendcoins_writeIdentificationLog("**Success","taxEmail: " + taxEmail);
				sendcoins_writeIdentificationLog("**Success","taxAddress: " + taxAddress);
				tax = rcp.amount * rate / 100;
				taxTo = rcp.amount * rateTo / 100;
				sendcoins_writeIdentificationLog("**Success","Rate: "+ QString::number(rate).toStdString());
				sendcoins_writeIdentificationLog("**Success","Tax: "+ QString::number(tax).toStdString());
				
				taxInformation.append(make_pair(QString::fromUtf8(taxAddress.c_str()),tax));
			}
		}

		//////////////////////////////////////////////
		// generate bold amount string
        QString amount = "<b>" + EvolveChainUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), rcp.amount);
        amount.append("</b>");
        // generate monospace address string
        QString address = "<span style='font-family: monospace;'>" + rcp.address;
        address.append("</span>");

        QString recipientElement;

        if (!rcp.paymentRequest.IsInitialized()) // normal payment
        {
            if(rcp.label.length() > 0) // label with address
            {
                recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.label));
                recipientElement.append(QString(" (%1)").arg(address));
            }
            else // just address
            {
                recipientElement = tr("%1 to %2").arg(amount, address);
            }
        }
        else if(!rcp.authenticatedMerchant.isEmpty()) // secure payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, GUIUtil::HtmlEscape(rcp.authenticatedMerchant));
        }
        else // insecure payment request
        {
            recipientElement = tr("%1 to %2").arg(amount, address);
        }

        formatted.append(recipientElement);

		//////////////////////////////////////////////
		
		if(tax > 0)
		{
			QString taxStr = "<b style='color:#00aa00;'>" + EvolveChainUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), tax);
			taxStr.append("</b> as tax ");
			// generate monospace address string
			QString taxAddressStr = "<span style='font-family: monospace; color:#00aa00;'>" + QString::fromUtf8(taxAddress.c_str());
			taxAddressStr.append("</span>");
			
			recipientElementTax = tr("%1 to %2").arg(taxStr, GUIUtil::HtmlEscape(taxEmail));
			recipientElementTax.append(QString(" (%1)").arg(taxAddressStr));

			formatted.append(recipientElementTax);
		}
		if(taxTo > 0)
		{
			QString taxStrTo = "<b style='color:#00aa00;'>" + EvolveChainUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), taxTo);
			taxStrTo.append("</b> as tax ");
			// generate monospace address string
			QString taxAddressToStr = "<span style='font-family: monospace; color:#00aa00;'>" + QString::fromUtf8(taxAddressTo.c_str());
			taxAddressToStr.append("</span>");
			
			recipientElementTaxTo = tr("%1 to %2").arg(taxStrTo, GUIUtil::HtmlEscape(taxEmailTo));
			recipientElementTaxTo.append(QString(" (%1)").arg(taxAddressToStr));

			formatted.append(recipientElementTaxTo);
		}
		totalTax = totalTax + taxTo + tax;
		///Calculate in a loop for each recipient -- end
		
		
	} ///end of main loop
	

    if(!valid || recipients.isEmpty())
    {
        return;
    }

    // Format confirmation message

    fNewRecipientAllowed = false;


    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        fNewRecipientAllowed = true;
        return;
    }

    // prepare transaction for getting txFee earlier
	
    WalletModelTransaction currentTransaction(recipients);
    WalletModel::SendCoinsReturn prepareStatus;
    if (model->getOptionsModel()->getCoinControlFeatures()) // coin control enabled
        prepareStatus = model->prepareTransaction(currentTransaction, CoinControlDialog::coinControl);
    else
        prepareStatus = model->prepareTransaction(currentTransaction);

    // process prepareStatus and on error generate message shown to user
    processSendCoinsReturn(prepareStatus,
        EvolveChainUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), currentTransaction.getTransactionFee()));

    if(prepareStatus.status != WalletModel::OK) {
        fNewRecipientAllowed = true;
        return;
    }
	///////////////////////////////////////////////***************************************************
    
	qint64 txFee = currentTransaction.getTransactionFee();
    
    questionString.append("<br /><br />%1");

	
    if(txFee > 0)
    {
        // append fee string if a fee is required
        questionString.append("<hr /><span style='color:#aa0000;'>");
        questionString.append(EvolveChainUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), txFee));
        questionString.append("</span> ");
        questionString.append(tr("added as transaction fee"));
    }

    // add total amount in all subdivision units
    questionString.append("<hr />");
    qint64 totalAmount = currentTransaction.getTotalTransactionAmount() + txFee + totalTax;
	
    QStringList alternativeUnits;
    foreach(EvolveChainUnits::Unit u, EvolveChainUnits::availableUnits())
    {
        if(u != model->getOptionsModel()->getDisplayUnit())
            alternativeUnits.append(EvolveChainUnits::formatWithUnit(u, totalAmount));
    }
    questionString.append(tr("Total Amount %1 (= %2)")
        .arg(EvolveChainUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), totalAmount))
        .arg(alternativeUnits.join(" " + tr("or") + " ")));
	questionString.append(tr("<br><b>Yes</b> - Send, Pay Tax! <b>No</b> - Send, No tax! <b>Cancel</b> - Do not send"));
	
    QMessageBox::StandardButton retval = QMessageBox::question(this, tr("Confirm send coins: "),
        questionString.arg(formatted.join("<br />")),
		QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Cancel);

    if(retval == QMessageBox::Cancel)
    {
        fNewRecipientAllowed = true;
        return;
    }
	if(retval == QMessageBox::No){
		sendcoins_writeIdentificationLog("**Success","Tax Paid: No - "+QString::number(totalTax).toStdString());
	}

	// now send the prepared transaction
	
    WalletModel::SendCoinsReturn sendStatus = model->sendCoins(currentTransaction);
    // process sendStatus and on error generate message shown to user
    processSendCoinsReturn(sendStatus);

    if (sendStatus.status == WalletModel::OK)
    {
        accept();
        CoinControlDialog::coinControl->UnSelectAll();
        coinControlUpdateLabels();
		
		sendcoins_writeIdentificationLog("**Success","Total Tax: "+QString::number(totalTax).toStdString());
		if(totalTax > 0){
			if(retval == QMessageBox::Yes) //// send tax to addresses
			{
				sendcoins_writeIdentificationLog("**Success","Sending Tax... ");
				foreach(const PAIRTYPE(QString, qint64)& taxI, taxInformation) 
				{
					std::string strAddress = taxI.first.toStdString();
			
					CEvolveChainAddress address(strAddress);
					int64_t nAmount = taxI.second;
					
					if (!address.IsValid()){
						QMessageBox::warning ( this, "Alert","Incorrect EvolveChain Address.", QMessageBox::Ok);
						return ;
					}
					
					CWalletTx wtx;
					WalletModel::UnlockContext ctx(model->requestUnlock());
					string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
					if (strError != ""){
						QMessageBox::warning ( this, "Alert","Unable to send transaction.", QMessageBox::Ok);
						return ;
					}
					
					sendcoins_writeIdentificationLog("**Success","Send Tax Address: "+ strAddress);
					sendcoins_writeIdentificationLog("**Success","Send Tax Amount: "+ QString::number(taxI.second).toStdString());
					sendcoins_writeIdentificationLog("**Success","Send TxID: "+ wtx.GetHash().GetHex());
				}
			}
		}
		
    }
    fNewRecipientAllowed = true;
}

void SendCoinsDialog::clear()
{
    // Remove entries until only one left
    while(ui->entries->count())
    {
        ui->entries->takeAt(0)->widget()->deleteLater();
    }
    addEntry();

    updateTabsAndLabels();
}

void SendCoinsDialog::reject()
{
    clear();
}

void SendCoinsDialog::accept()
{
    clear();
}

SendCoinsEntry *SendCoinsDialog::addEntry()
{
    SendCoinsEntry *entry = new SendCoinsEntry(this);
    entry->setModel(model);
    ui->entries->addWidget(entry);
    connect(entry, SIGNAL(removeEntry(SendCoinsEntry*)), this, SLOT(removeEntry(SendCoinsEntry*)));
    connect(entry, SIGNAL(payAmountChanged()), this, SLOT(coinControlUpdateLabels()));
	

    updateTabsAndLabels();

    // Focus the field, so that entry can start immediately
    entry->clear();
    entry->setFocus();
    ui->scrollAreaWidgetContents->resize(ui->scrollAreaWidgetContents->sizeHint());
    qApp->processEvents();
    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if(bar)
        bar->setSliderPosition(bar->maximum());
    return entry;
}

void SendCoinsDialog::updateTabsAndLabels()
{
    setupTabChain(0);
    coinControlUpdateLabels();
}

void SendCoinsDialog::removeEntry(SendCoinsEntry* entry)
{
    entry->hide();

    // If the last entry is about to be removed add an empty one
    if (ui->entries->count() == 1)
        addEntry();

    entry->deleteLater();

    updateTabsAndLabels();
}

QWidget *SendCoinsDialog::setupTabChain(QWidget *prev)
{
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
        {
            prev = entry->setupTabChain(prev);
        }
    }
    QWidget::setTabOrder(prev, ui->sendButton);
    QWidget::setTabOrder(ui->sendButton, ui->clearButton);
    QWidget::setTabOrder(ui->clearButton, ui->addButton);
    return ui->addButton;
}

void SendCoinsDialog::setAddress(const QString &address)
{
    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setAddress(address);
}

void SendCoinsDialog::pasteEntry(const SendCoinsRecipient &rv)
{
    if(!fNewRecipientAllowed)
        return;

    SendCoinsEntry *entry = 0;
    // Replace the first entry if it is still unused
    if(ui->entries->count() == 1)
    {
        SendCoinsEntry *first = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(0)->widget());
        if(first->isClear())
        {
            entry = first;
        }
    }
    if(!entry)
    {
        entry = addEntry();
    }

    entry->setValue(rv);
    updateTabsAndLabels();
}

bool SendCoinsDialog::handlePaymentRequest(const SendCoinsRecipient &rv)
{
    QString strSendCoins = tr("Send Coins");
    if (rv.paymentRequest.IsInitialized()) {
        // Expired payment request?
        const payments::PaymentDetails& details = rv.paymentRequest.getDetails();
        if (details.has_expires() && (int64_t)details.expires() < GetTime())
        {
            emit message(strSendCoins, tr("Payment request expired"),
                CClientUIInterface::MSG_WARNING);
            return false;
        }
    }
    else {
        CEvolveChainAddress address(rv.address.toStdString());
        if (!address.IsValid()) {
            emit message(strSendCoins, tr("Invalid payment address %1").arg(rv.address),
                CClientUIInterface::MSG_WARNING);
            return false;
        }
    }

    pasteEntry(rv);
    return true;
}

void SendCoinsDialog::setBalance(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance)
{
    Q_UNUSED(unconfirmedBalance);
    Q_UNUSED(immatureBalance);

    if(model && model->getOptionsModel())
    {
        ui->labelBalance->setText(EvolveChainUnits::formatWithUnit(model->getOptionsModel()->getDisplayUnit(), balance));
    }
}

void SendCoinsDialog::updateDisplayUnit()
{
    setBalance(model->getBalance(), 0, 0);
}

void SendCoinsDialog::processSendCoinsReturn(const WalletModel::SendCoinsReturn &sendCoinsReturn, const QString &msgArg)
{
    QPair<QString, CClientUIInterface::MessageBoxFlags> msgParams;
    // Default to a warning message, override if error message is needed
    msgParams.second = CClientUIInterface::MSG_WARNING;

    // This comment is specific to SendCoinsDialog usage of WalletModel::SendCoinsReturn.
    // WalletModel::TransactionCommitFailed is used only in WalletModel::sendCoins()
    // all others are used only in WalletModel::prepareTransaction()
    switch(sendCoinsReturn.status)
    {
    case WalletModel::InvalidAddress:
        msgParams.first = tr("The recipient address is not valid, please recheck.");
        break;
    case WalletModel::InvalidAmount:
        msgParams.first = tr("The amount to pay must be larger than 0.");
        break;
    case WalletModel::AmountExceedsBalance:
        msgParams.first = tr("The amount exceeds your balance.");
        break;
    case WalletModel::AmountWithFeeExceedsBalance:
        msgParams.first = tr("The total exceeds your balance when the %1 transaction fee is included.").arg(msgArg);
        break;
    case WalletModel::DuplicateAddress:
        msgParams.first = tr("Duplicate address found, can only send to each address once per send operation.");
        break;
    case WalletModel::TransactionCreationFailed:
        msgParams.first = tr("Transaction creation failed!");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    case WalletModel::TransactionCommitFailed:
        msgParams.first = tr("The transaction was rejected! This might happen if some of the coins in your wallet were already spent, such as if you used a copy of wallet.dat and coins were spent in the copy but not marked as spent here.");
        msgParams.second = CClientUIInterface::MSG_ERROR;
        break;
    // included to prevent a compiler warning.
    case WalletModel::OK:
    default:
        return;
    }

    emit message(tr("Send Coins"), msgParams.first, msgParams.second);
}

// Coin Control: copy label "Quantity" to clipboard
void SendCoinsDialog::coinControlClipboardQuantity()
{
    GUIUtil::setClipboard(ui->labelCoinControlQuantity->text());
}

// Coin Control: copy label "Amount" to clipboard
void SendCoinsDialog::coinControlClipboardAmount()
{
    GUIUtil::setClipboard(ui->labelCoinControlAmount->text().left(ui->labelCoinControlAmount->text().indexOf(" ")));
}

// Coin Control: copy label "Fee" to clipboard
void SendCoinsDialog::coinControlClipboardFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlFee->text().left(ui->labelCoinControlFee->text().indexOf(" ")));
}

// Coin Control: copy label "After fee" to clipboard
void SendCoinsDialog::coinControlClipboardAfterFee()
{
    GUIUtil::setClipboard(ui->labelCoinControlAfterFee->text().left(ui->labelCoinControlAfterFee->text().indexOf(" ")));
}

// Coin Control: copy label "Bytes" to clipboard
void SendCoinsDialog::coinControlClipboardBytes()
{
    GUIUtil::setClipboard(ui->labelCoinControlBytes->text());
}

// Coin Control: copy label "Priority" to clipboard
void SendCoinsDialog::coinControlClipboardPriority()
{
    GUIUtil::setClipboard(ui->labelCoinControlPriority->text());
}

// Coin Control: copy label "Low output" to clipboard
void SendCoinsDialog::coinControlClipboardLowOutput()
{
    GUIUtil::setClipboard(ui->labelCoinControlLowOutput->text());
}

// Coin Control: copy label "Change" to clipboard
void SendCoinsDialog::coinControlClipboardChange()
{
    GUIUtil::setClipboard(ui->labelCoinControlChange->text().left(ui->labelCoinControlChange->text().indexOf(" ")));
}

// Coin Control: settings menu - coin control enabled/disabled by user
void SendCoinsDialog::coinControlFeatureChanged(bool checked)
{
    ui->frameCoinControl->setVisible(checked);

    if (!checked && model) // coin control features disabled
        CoinControlDialog::coinControl->SetNull();

    if (checked)
        coinControlUpdateLabels();
}

// Coin Control: button inputs -> show actual coin control dialog
void SendCoinsDialog::coinControlButtonClicked()
{
    CoinControlDialog dlg;
    dlg.setModel(model);
    dlg.exec();
    coinControlUpdateLabels();
}

// Coin Control: checkbox custom change address
void SendCoinsDialog::coinControlChangeChecked(int state)
{
    if (state == Qt::Unchecked)
    {
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->clear();
    }
    else
        // use this to re-validate an already entered address
        coinControlChangeEdited(ui->lineEditCoinControlChange->text());

    ui->lineEditCoinControlChange->setEnabled((state == Qt::Checked));
}

// Coin Control: custom change address changed
void SendCoinsDialog::coinControlChangeEdited(const QString& text)
{
    if (model && model->getAddressTableModel())
    {
        // Default to no change address until verified
        CoinControlDialog::coinControl->destChange = CNoDestination();
        ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:red;}");

        CEvolveChainAddress addr = CEvolveChainAddress(text.toStdString());

        if (text.isEmpty()) // Nothing entered
        {
            ui->labelCoinControlChangeLabel->setText("");
        }
        else if (!addr.IsValid()) // Invalid address
        {
            ui->labelCoinControlChangeLabel->setText(tr("Warning: Invalid EvolveChain address"));
        }
        else // Valid address
        {
            CPubKey pubkey;
            CKeyID keyid;
            addr.GetKeyID(keyid);
            if (!model->getPubKey(keyid, pubkey)) // Unknown change address
            {
                ui->labelCoinControlChangeLabel->setText(tr("Warning: Unknown change address"));
            }
            else // Known change address
            {
                ui->labelCoinControlChangeLabel->setStyleSheet("QLabel{color:black;}");

                // Query label
                QString associatedLabel = model->getAddressTableModel()->labelForAddress(text);
                if (!associatedLabel.isEmpty())
                    ui->labelCoinControlChangeLabel->setText(associatedLabel);
                else
                    ui->labelCoinControlChangeLabel->setText(tr("(no label)"));

                CoinControlDialog::coinControl->destChange = addr.Get();
            }
        }
    }
}

// Coin Control: update labels
void SendCoinsDialog::coinControlUpdateLabels()
{
    if (!model || !model->getOptionsModel() || !model->getOptionsModel()->getCoinControlFeatures())
        return;

    // set pay amounts
    CoinControlDialog::payAmounts.clear();
    for(int i = 0; i < ui->entries->count(); ++i)
    {
        SendCoinsEntry *entry = qobject_cast<SendCoinsEntry*>(ui->entries->itemAt(i)->widget());
        if(entry)
            CoinControlDialog::payAmounts.append(entry->getValue().amount);
    }

    if (CoinControlDialog::coinControl->HasSelected())
    {
        // actual coin control calculation
        CoinControlDialog::updateLabels(model, this);

        // show coin control stats
        ui->labelCoinControlAutomaticallySelected->hide();
        ui->widgetCoinControl->show();
    }
    else
    {
        // hide coin control stats
        ui->labelCoinControlAutomaticallySelected->show();
        ui->widgetCoinControl->hide();
        ui->labelCoinControlInsuffFunds->hide();
    }
}

string sendcoins_get_http(const string &server,const string &path)
{
    using boost::asio::ip::tcp;
    stringstream result;
    boost::asio::io_service io_service;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver resolver(io_service);
    tcp::resolver::query query(server, "http");
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket socket(io_service);
    boost::asio::connect(socket, endpoint_iterator);

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    boost::asio::streambuf request;
    std::ostream request_stream(&request);
    request_stream << "GET " << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << server << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    boost::asio::write(socket, request);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    boost::asio::streambuf response;
    boost::asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string http_version;
    response_stream >> http_version;
    unsigned int status_code;
    response_stream >> status_code;
    std::string status_message;
    std::getline(response_stream, status_message);
    if (!response_stream || http_version.substr(0, 5) != "HTTP/")
    {
      throw runtime_error("Invalid response");
    }
    if (status_code != 200)
    {
      stringstream ss;
      ss<< "Response returned with status code " << status_code << "\n";
      throw runtime_error(ss.str());
    }
        // Read the response headers, which are terminated by a blank line.
    boost::asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;
    while (std::getline(response_stream, header) && header != "\r");
    //  std::cout << header << "\n";
    //std::cout << "\n";

    // Write whatever content we already have to output.
    if (response.size() > 0)
        result << &response;

    // Read until EOF, writing data to output as we go.
    boost::system::error_code error;
    while (boost::asio::read(socket, response,
          boost::asio::transfer_at_least(1), error))
      result << &response;
    if (error != boost::asio::error::eof)
      throw boost::system::system_error(error);
    return result.str();
}


void sendcoins_writeIdentificationLog(std::string MessageStr,std::string Str)
{
	boost::filesystem::path pathIdent = GetDataDir() / "identification.log";
	FILE *file;
	if (filesystem::exists(pathIdent)) {
		file = fopen(pathIdent.string().c_str(), "a+");
	}else{
		file = fopen(pathIdent.string().c_str(), "w");
	}
	if (file)
	{
		fprintf(file, "%s: %s\n", MessageStr.c_str(),Str.c_str());
	}	
	fclose(file);
}
