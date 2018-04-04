// Copyright (c) 2014-2015 The EvolveChainX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QApplication>
#include <QClipboard>
#include "sendcoinsentry.h"
#include "ui_sendcoinsentry.h"
#include "evolvechainunits.h"
#include "base58.h"

#include "util.h"
#include "main.h"
#include "addressbookpage.h"
#include "addresstablemodel.h"
#include "guiutil.h"
#include "optionsmodel.h"
#include <iostream>

#include <boost/asio.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

string get_coin_http(const string &server,const string &path);


SendCoinsEntry::SendCoinsEntry(QWidget *parent) :
	
    QStackedWidget(parent),
    ui(new Ui::SendCoinsEntry),
    model(0)
{
    ui->setupUi(this);

    setCurrentWidget(ui->SendCoins);

#ifdef Q_OS_MAC
    ui->payToLayout->setSpacing(4);
#endif
#if QT_VERSION >= 0x040700
    ui->addAsLabel->setPlaceholderText(tr("Enter a label for this address to add it to your address book"));
	ui->lineEmail->setPlaceholderText(tr("Enter a valid email address"));
	ui->linePhone->setPlaceholderText(tr("Enter a valid phone number with international code"));
#endif

    // normal evolvechain address field
    GUIUtil::setupAddressWidget(ui->payTo, this);
    // just a label for displaying evolvechain address(es)
    ui->payTo_is->setFont(GUIUtil::evolvechainAddressFont());
}

SendCoinsEntry::~SendCoinsEntry()
{
    delete ui;
}

void SendCoinsEntry::on_pasteButton_clicked()
{
    // Paste text from clipboard into recipient field
    ui->payTo->setText(QApplication::clipboard()->text());
}

void SendCoinsEntry::on_addressBookButton_clicked()
{
    if(!model)
        return;
    AddressBookPage dlg(AddressBookPage::ForSelection, AddressBookPage::SendingTab, this);
    dlg.setModel(model->getAddressTableModel());
    if(dlg.exec())
    {
        ui->payTo->setText(dlg.getReturnValue());
        ui->payAmount->setFocus();
    }
}

void SendCoinsEntry::on_payTo_textChanged(const QString &address)
{
 updateLabel(address);
	updateIdentification(address);	
}
void SendCoinsEntry::on_searchEmail_Clicked(){
	
	bool fListening = false;
	std::stringstream EvolveChainJSON;		
	
	std::string parameters = "/search/code/" + identiCode ;	
	std::string fromCountry = "";
	try
	{
		EvolveChainJSON << get_coin_http("evolvechain.org", parameters);
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
		}
	}
	
	QString strIdentificationEmail = ui->lineEmail->text();
	ui->whoisLabel->setText("Please enter correct email address.");
	if(strIdentificationEmail == "")
	{
//		QMessageBox::warning ( this, "Alert","Please enter correct email address.", QMessageBox::Ok);
		return;
	}
	QRegExp filter_re("^.+@[^\.].*\.[a-z]{2,}$");	 
	
	if(!filter_re.exactMatch(strIdentificationEmail))
	{
//		QMessageBox::warning ( this, "Alert","Please enter correct email address.", QMessageBox::Ok);
		return;
	}
	std::string strerr;
	parameters = "/search/email/"  + strIdentificationEmail.toStdString();	
	try
	{
		EvolveChainJSON << get_coin_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		ui->whoisLabel->setText("Please connect to internet.");
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

		if(strSuccess=="1"){
			std::string strAddress = NewPointer.get<std::string> ("address");
			std::string strEmail = NewPointer.get<std::string> ("email");;
			std::string strPhone = NewPointer.get<std::string> ("phone");			
			std::string strIP = NewPointer.get<std::string> ("ip");
			std::string strCity = NewPointer.get<std::string> ("city");
			std::string strCountry = fromCountry;
			 toCountry = NewPointer.get<std::string> ("country"); /***************************************/
			std::string strDateTime = NewPointer.get<std::string> ("DateTime");
			std::string strExtra = NewPointer.get<std::string> ("extra");
			
			std::string whoisStr = "Email: " + strEmail + ", Phone: " +strPhone +", City/Country: " + strCity + "/" + strCountry + " on " + strDateTime+" Additional Info: "+strExtra;
			
			ui->payTo->setText(strAddress.c_str());
			ui->payTo->setEnabled(false);
	
		ui->lineEmail->setText(strEmail.c_str());
		ui->lineEmail->setEnabled(true);

			ui->linePhone->setText(strPhone.c_str());
			ui->linePhone->setEnabled(false);

			ui->whoisLabel->setText(whoisStr.c_str());
			ui->labelIN->setText(strCountry.c_str());
			ui->labelOUT->setText(toCountry.c_str());
			ui->payAmount->setFocus();
			return ;
		}else{
			QString blank = "";
			
			ui->lineEmail->setEnabled(true);

			ui->payTo->setText(blank);
			ui->payTo->setEnabled(true);

			ui->linePhone->setText(blank);
			ui->linePhone->setEnabled(true);
			ui->whoisLabel->setText("Email address not registered with EvolveChainX");
			return ;
		}
	}
	ui->whoisLabel->setText("Email address not registered with EvolveChainX");
	return;
}
void SendCoinsEntry::on_searchPhone_Clicked(){
	bool fListening = false;
	std::stringstream EvolveChainJSON;		
	
	std::string parameters = "/search/code/" + identiCode ;	
	std::string fromCountry = "";
	try
	{
		EvolveChainJSON << get_coin_http("evolvechain.org", parameters);
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
		}
	}
	
	
	QString strIdentificationPhone = ui->linePhone->text();
	ui->whoisLabel->setText("Please enter correct phone number with international code.");	
	if(strIdentificationPhone == "")
	{
//		QMessageBox::warning ( this, "Alert","Please enter correct phone number.", QMessageBox::Ok);
		return;
	}
	QRegExp filter_re("^(999|998|997|996|995|994|993|992|991|990|979|978|977|976|975|974|973|972|971|970|969|968|967|966|965|964|963|962|961|960|899|898|897|896|895|894|893|892|891|890|889|888|887|886|885|884|883|882|881|880|879|878|877|876|875|874|873|872|871|870|859|858|857|856|855|854|853|852|851|850|839|838|837|836|835|834|833|832|831|830|809|808|807|806|805|804|803|802|801|800|699|698|697|696|695|694|693|692|691|690|689|688|687|686|685|684|683|682|681|680|679|678|677|676|675|674|673|672|671|670|599|598|597|596|595|594|593|592|591|590|509|508|507|506|505|504|503|502|501|500|429|428|427|426|425|424|423|422|421|420|389|388|387|386|385|384|383|382|381|380|379|378|377|376|375|374|373|372|371|370|359|358|357|356|355|354|353|352|351|350|299|298|297|296|295|294|293|292|291|290|289|288|287|286|285|284|283|282|281|280|269|268|267|266|265|264|263|262|261|260|259|258|257|256|255|254|253|252|251|250|249|248|247|246|245|244|243|242|241|240|239|238|237|236|235|234|233|232|231|230|229|228|227|226|225|224|223|222|221|220|219|218|217|216|215|214|213|212|211|210|98|95|94|93|92|91|90|86|84|82|81|66|65|64|63|62|61|60|58|57|56|55|54|53|52|51|49|48|47|46|45|44|43|41|40|39|36|34|33|32|31|30|27|20|7|1)[0-9]{0,14}$");	 
	
	if(!filter_re.exactMatch(strIdentificationPhone))
	{
		QMessageBox::warning ( this, "Alert","Please enter correct phone address.", QMessageBox::Ok);
		return;
	}
	std::string strerr;
	parameters = "/search/phone/+" + strIdentificationPhone.toStdString();
	try
	{
		EvolveChainJSON << get_coin_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		ui->whoisLabel->setText("Please connect to internet.");
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
		if(strSuccess=="1"){
			std::string strAddress = NewPointer.get<std::string> ("address");
			std::string strEmail = NewPointer.get<std::string> ("email");;
			std::string strPhone = NewPointer.get<std::string> ("phone");			
			std::string strIP = NewPointer.get<std::string> ("ip");
			std::string strCity = NewPointer.get<std::string> ("city");
			std::string strCountry = fromCountry;
			 toCountry = NewPointer.get<std::string> ("country"); /***************************************/
			std::string strDateTime = NewPointer.get<std::string> ("DateTime");
			std::string strExtra = NewPointer.get<std::string> ("extra");
			
			std::string whoisStr = "Email: " + strEmail + ", Phone: " +strPhone +", City/Country: " + strCity + "/" + strCountry + " on " + strDateTime+" Additional Info: "+strExtra;
			
			ui->lineEmail->setText(strEmail.c_str());
			ui->lineEmail->setEnabled(false);

			ui->payTo->setText(strAddress.c_str());
			ui->payTo->setEnabled(false);

			ui->linePhone->setText(strIdentificationPhone);
			ui->linePhone->setEnabled(true);

			ui->whoisLabel->setText(whoisStr.c_str());
			ui->labelIN->setText(strCountry.c_str());
			ui->labelOUT->setText(toCountry.c_str());
			ui->payAmount->setFocus();
			return ;
		}else{
			QString blank = "";
			ui->lineEmail->setText(blank);
			ui->lineEmail->setEnabled(true);

			ui->payTo->setText(blank);
			ui->payTo->setEnabled(true);

			
			ui->linePhone->setEnabled(true);
			ui->whoisLabel->setText("Phone is not registered with EvolveChainX");
			return ;
		}
	
		ui->whoisLabel->setText("Phone is not registered with EvolveChainX");
		return;
	}
}

void SendCoinsEntry::on_startOver_Clicked(){

	QString blank = "";
	ui->lineEmail->setText(blank);
	ui->lineEmail->setEnabled(true);

	ui->payTo->setText(blank);
	ui->payTo->setEnabled(true);

	ui->linePhone->setText(blank);
	ui->linePhone->setEnabled(true);
	return;
}
void SendCoinsEntry::setModel(WalletModel *model)
{
    this->model = model;

    if (model && model->getOptionsModel())
        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

    connect(ui->payAmount, SIGNAL(textChanged()), this, SIGNAL(payAmountChanged()));
				connect(ui->payAmount, SIGNAL(textChanged()), this, SLOT(taxCalculations()));
    connect(ui->deleteButton, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_is, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->deleteButton_s, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->searchEmail, SIGNAL(clicked()), this, SLOT(on_searchEmail_Clicked()));
	   connect(ui->searchPhone, SIGNAL(clicked()), this, SLOT(on_searchPhone_Clicked()));	
	   connect(ui->startOver, SIGNAL(clicked()), this, SLOT(on_startOver_Clicked()));		
    clear();
}

void SendCoinsEntry::clear()
{
    // clear UI elements for normal payment
    ui->payTo->clear();
    ui->addAsLabel->clear();
    ui->payAmount->clear();
    ui->messageTextLabel->clear();
    ui->messageTextLabel->hide();
    ui->messageLabel->hide();
    // clear UI elements for insecure payment request
    ui->payTo_is->clear();
    ui->memoTextLabel_is->clear();
    ui->payAmount_is->clear();
    // clear UI elements for secure payment request
    ui->payTo_s->clear();
    ui->memoTextLabel_s->clear();
    ui->payAmount_s->clear();

    // update the display unit, to not use the default ("EVC")
    updateDisplayUnit();
}

void SendCoinsEntry::deleteClicked()
{
    emit removeEntry(this);
}

bool SendCoinsEntry::validate()
{
    if (!model)
        return false;

    // Check input validity
    bool retval = true;

    // Skip checks for payment request
    if (recipient.paymentRequest.IsInitialized())
        return retval;

    if (!model->validateAddress(ui->payTo->text()))
    {
        ui->payTo->setValid(false);
        retval = false;
    }

    if (!ui->payAmount->validate())
    {
        retval = false;
    }

    // Reject dust outputs:
    if (retval && GUIUtil::isDust(ui->payTo->text(), ui->payAmount->value())) {
        ui->payAmount->setValid(false);
        retval = false;
    }

    return retval;
}

SendCoinsRecipient SendCoinsEntry::getValue()
{
    // Payment request
    if (recipient.paymentRequest.IsInitialized())
        return recipient;

    // Normal payment
    recipient.address = ui->payTo->text();
    recipient.label = ui->addAsLabel->text();
    recipient.amount = ui->payAmount->value();
    recipient.message = ui->messageTextLabel->text();
    return recipient;
}


QWidget *SendCoinsEntry::setupTabChain(QWidget *prev)
{
    QWidget::setTabOrder(prev, ui->payTo);
    QWidget::setTabOrder(ui->payTo, ui->addAsLabel);
    QWidget *w = ui->payAmount->setupTabChain(ui->addAsLabel);
    QWidget::setTabOrder(w, ui->addressBookButton);
    QWidget::setTabOrder(ui->addressBookButton, ui->pasteButton);
    QWidget::setTabOrder(ui->pasteButton, ui->deleteButton);
    return ui->deleteButton;
}

void SendCoinsEntry::setValue(const SendCoinsRecipient &value)
{
    recipient = value;

    if (recipient.paymentRequest.IsInitialized()) // payment request
    {
        if (recipient.authenticatedMerchant.isEmpty()) // insecure
        {
            ui->payTo_is->setText(recipient.address);
            ui->memoTextLabel_is->setText(recipient.message);
            ui->payAmount_is->setValue(recipient.amount);
            ui->payAmount_is->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_InsecurePaymentRequest);
        }
        else // secure
        {
            ui->payTo_s->setText(recipient.authenticatedMerchant);
            ui->memoTextLabel_s->setText(recipient.message);
            ui->payAmount_s->setValue(recipient.amount);
            ui->payAmount_s->setReadOnly(true);
            setCurrentWidget(ui->SendCoins_SecurePaymentRequest);
        }
    }
    else // normal payment
    {
        // message
        ui->messageTextLabel->setText(recipient.message);
        ui->messageTextLabel->setVisible(!recipient.message.isEmpty());
        ui->messageLabel->setVisible(!recipient.message.isEmpty());

        ui->addAsLabel->clear();
        ui->payTo->setText(recipient.address); // this may set a label from addressbook
        if (!recipient.label.isEmpty()) // if a label had been set from the addressbook, dont overwrite with an empty label
            ui->addAsLabel->setText(recipient.label);
        ui->payAmount->setValue(recipient.amount);
    }
}

void SendCoinsEntry::setAddress(const QString &address)
{
    ui->payTo->setText(address);
    ui->payAmount->setFocus();
}

bool SendCoinsEntry::isClear()
{
    return ui->payTo->text().isEmpty() && ui->payTo_is->text().isEmpty() && ui->payTo_s->text().isEmpty();
}

void SendCoinsEntry::setFocus()
{
    ui->payTo->setFocus();
}

void SendCoinsEntry::updateDisplayUnit()
{
    if(model && model->getOptionsModel())
    {
        // Update payAmount with the current unit
        ui->payAmount->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_is->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
        ui->payAmount_s->setDisplayUnit(model->getOptionsModel()->getDisplayUnit());
    }
}

bool SendCoinsEntry::updateLabel(const QString &address)
{
    if(!model)
        return false;

    // Fill in label from address book, if address has an associated label
    QString associatedLabel = model->getAddressTableModel()->labelForAddress(address);
    if(!associatedLabel.isEmpty())
    {
        ui->addAsLabel->setText(associatedLabel);
        return true;
    }

    return false;
}

bool SendCoinsEntry::updateIdentification(const QString &address)
{
    if(!model){
		ui->whoisLabel->setText("Out!");
        return false;
	}
	if(!model->validateAddress(address))
	{
		ui->whoisLabel->setText("EvolveChain address is invalid.");	
		return false;
	}
	std::stringstream EvolveChainJSON;
	bool fListening = false;
	const std::string addressStr = address.toStdString();	
	try
	{
		EvolveChainJSON << get_coin_http("evolvechain.org","/verify/address/"+addressStr);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		ui->whoisLabel->setText("Please connect to internet.");
	}
	std::string whoisStr = "";
	if(fListening==true)
		{

			string EvolveChainStr = EvolveChainJSON.str();  
			EvolveChainStr.replace(EvolveChainStr.find(","), sizeof(", ")-1, ", ");
			
			size_t pos = 0;
			const std::string& search = ",";
			const std::string& replace =", ";

			while ((pos = EvolveChainStr.find(search, pos)) != std::string::npos) {
				EvolveChainStr.replace(pos, search.length(), replace);
				pos += replace.length();
			}
			
			// JSON ****************************************

			using namespace boost::property_tree;
			using boost::property_tree::ptree;
			using boost::property_tree::read_json;
			using boost::property_tree::write_json;
			ptree NewPointer;
	  
			read_json (EvolveChainJSON, NewPointer);
			std::string strSuccess = NewPointer.get<std::string> ("success");
			
			if(strSuccess=="1"){
				std::string strAddress = NewPointer.get<std::string> ("address");
				std::string strEmail = NewPointer.get<std::string> ("email");;
				std::string strPhone = NewPointer.get<std::string> ("phone");			
				std::string strIP = NewPointer.get<std::string> ("ip");
				std::string strCity = NewPointer.get<std::string> ("city");
				std::string strCountry = NewPointer.get<std::string> ("country");
				 toCountry = NewPointer.get<std::string> ("country"); /***************************************/
				std::string strDateTime = NewPointer.get<std::string> ("DateTime");
				std::string strExtra = NewPointer.get<std::string> ("extra");
	
				addressIdentified = true; 
				whoisStr = "Email: " + strEmail + ", Phone: " +strPhone +", City/Country: " + strCity + "/" + strCountry + " on " + strDateTime+" Additional Info: "+strExtra;
				ui->labelIN->setText(strCountry.c_str());
				ui->labelOUT->setText(toCountry.c_str());
				ui->whoisLabel->setText(whoisStr.c_str());
				
				ui->lineEmail->setText(strEmail.c_str());
				ui->lineEmail->setEnabled(false);
				
				ui->linePhone->setText(strPhone.c_str());
				ui->linePhone->setEnabled(false);
				ui->whoisLabel->setText(whoisStr.c_str());
				//addressIdentified = true;
				

				
				return true;
			}
		}
		whoisStr = "EvolveChain address is not identified. Please check the address again.";
		ui->whoisLabel->setText(whoisStr.c_str());
		//addressIdentified = false;
		return true;
}



void SendCoinsEntry::taxCalculations(){

	std::stringstream EvolveChainJSON;
	bool fListening = false;

	QString fromCountry = ui->labelIN->text();
	QString toCountry = ui->labelOUT->text();
	qreal amount = ui->payAmount->value();
	
	std::string parameters;
	qreal rate = 0; ///this is the tax rate (in percentage)
	qint64 tax = 0; /// the is the tax amount
	
	qreal rateTo = 0; ///this is the tax rate (in percentage)
	qint64 taxTo = 0; /// the is the tax amount


	parameters = "/search/tax/" + fromCountry.toStdString() + "/" + toCountry.toStdString() +  "/" + QString::number(amount).toStdString();	/// This will get the tax information of the recipient country and calculate accordingly.

	try
	{
		EvolveChainJSON << get_coin_http("evolvechain.org", parameters);
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
				rateTo = NewPointer.get<float> ("rateTo");

				tax = amount * rate / 100;
				taxTo = amount * rateTo / 100;
				
				ui->labelRateIN->setText(QString::number(rate) +"%");
				ui->labelRateOUT->setText(QString::number(rateTo) +"%");
				
				ui->QTax->setText(EvolveChainUnits::formatWithOutUnit(model->getOptionsModel()->getDisplayUnit(), tax));
				ui->QTaxTo->setText(EvolveChainUnits::formatWithOutUnit(model->getOptionsModel()->getDisplayUnit(), taxTo));
				
			}
		}

}


string get_coin_http(const string &server,const string &path)
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
