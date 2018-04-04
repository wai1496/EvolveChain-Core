// Copyright (c) 2014-2015 The EvolveChainX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <QDir>
#include <QIntValidator>
#include <QLocale>
#include <QMessageBox>
#include <QTimer>

#if defined(HAVE_CONFIG_H)
#include "evolvechain-config.h"
#endif

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "util.h"
#include "evolvechainunits.h"
#include "guiutil.h"
#include "monitoreddatamapper.h"
#include "optionsmodel.h"
#include "wallet.h"
#include "walletdb.h"






#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>


#include "main.h" // for CTransaction::nMinTxFee and MAX_SCRIPTCHECK_THREADS
#include "netbase.h"
#include "txdb.h" // for -dbcache defaults

using namespace std;
using namespace boost;

QString verificationCode;
QString IdentifiedCode;
QString IPAddress;
QString KYCID;

string options_get_http(const string &server,const string &path);
void options_writeIdentificationLog(std::string MessageStr,std::string Str);
extern std::map<std::string, std::string> identifiedTransactions;

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog),
    model(0),
    mapper(0),
    fProxyIpValid(true)
{
    ui->setupUi(this);
    GUIUtil::restoreWindowGeometry("nOptionsDialogWindow", this->size(), this);

    /* Main elements init */
    ui->databaseCache->setMinimum(nMinDbCache);
    ui->databaseCache->setMaximum(nMaxDbCache);
    ui->threadsScriptVerif->setMinimum(-(int)boost::thread::hardware_concurrency());
    ui->threadsScriptVerif->setMaximum(MAX_SCRIPTCHECK_THREADS);

    /* Network elements init */
#ifndef USE_UPNP
    ui->mapPortUpnp->setEnabled(false);
#endif

    ui->proxyIp->setEnabled(false);
    ui->proxyPort->setEnabled(false);
    ui->proxyPort->setValidator(new QIntValidator(1, 65535, this));

    /** SOCKS version is only selectable for default proxy and is always 5 for IPv6 and Tor */
    ui->socksVersion->setEnabled(false);
    ui->socksVersion->addItem("5", 5);
    ui->socksVersion->addItem("4", 4);
    ui->socksVersion->setCurrentIndex(0);

    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyIp, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->proxyPort, SLOT(setEnabled(bool)));
    connect(ui->connectSocks, SIGNAL(toggled(bool)), ui->socksVersion, SLOT(setEnabled(bool)));

    ui->proxyIp->installEventFilter(this);

    /* Window elements init */
#ifdef Q_OS_MAC
    /* remove Window tab on Mac */
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabWindow));
#endif

    /* Display elements init */
    QDir translations(":translations");
    ui->lang->addItem(QString("(") + tr("default") + QString(")"), QVariant(""));
    foreach(const QString &langStr, translations.entryList())
    {
        QLocale locale(langStr);

        /** check if the locale name consists of 2 parts (language_country) */
        if(langStr.contains("_"))
        {
#if QT_VERSION >= 0x040800
            /** display language strings as "native language - native country (locale name)", e.g. "Deutsch - Deutschland (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" - ") + locale.nativeCountryName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
#else
            /** display language strings as "language - country (locale name)", e.g. "German - Germany (de)" */
            ui->lang->addItem(QLocale::languageToString(locale.language()) + QString(" - ") + QLocale::countryToString(locale.country()) + QString(" (") + langStr + QString(")"), QVariant(langStr));
#endif
        }
        else
        {
#if QT_VERSION >= 0x040800
            /** display language strings as "native language (locale name)", e.g. "Deutsch (de)" */
            ui->lang->addItem(locale.nativeLanguageName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
#else
            /** display language strings as "language (locale name)", e.g. "German (de)" */
            ui->lang->addItem(QLocale::languageToString(locale.language()) + QString(" (") + langStr + QString(")"), QVariant(langStr));
#endif
        }
    }
#if QT_VERSION >= 0x040700
    ui->thirdPartyTxUrls->setPlaceholderText("https://example.com/tx/%s");
#endif

    ui->unit->setModel(new EvolveChainUnits(this));
    ui->transactionFee->setSingleStep(CTransaction::nMinTxFee);
				QString QIdentiKYC = QString::fromUtf8(identiKYC.c_str());
				ui->kyc_id->setText(QIdentiKYC);
    /* Widget-to-option mapper */
    mapper = new MonitoredDataMapper(this);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setOrientation(Qt::Vertical);

    /* setup/change UI elements when proxy IP is invalid/valid */
    connect(this, SIGNAL(proxyIpChecks(QValidatedLineEdit *, int)), this, SLOT(doProxyIpChecks(QValidatedLineEdit *, int)));
}

OptionsDialog::~OptionsDialog()
{
    GUIUtil::saveWindowGeometry("nOptionsDialogWindow", this);
    delete ui;
}

void OptionsDialog::setModel(OptionsModel *model)
{
    this->model = model;

    if(model)
    {
        /* check if client restart is needed and show persistent message */
        if (model->isRestartRequired())
            showRestartWarning(true);

        QString strLabel = model->getOverriddenByCommandLine();
        if (strLabel.isEmpty())
            strLabel = tr("none");
        ui->overriddenByCommandLineLabel->setText(strLabel);

        connect(model, SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));

        mapper->setModel(model);
        setMapper();
        mapper->toFirst();
    }

    /* update the display unit, to not use the default ("EVC") */
    updateDisplayUnit();

    /* warn when one of the following settings changes by user action (placed here so init via mapper doesn't trigger them) */

    /* Main */
    connect(ui->databaseCache, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    connect(ui->threadsScriptVerif, SIGNAL(valueChanged(int)), this, SLOT(showRestartWarning()));
    /* Wallet */
    connect(ui->spendZeroConfChange, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    /* Network */
    connect(ui->connectSocks, SIGNAL(clicked(bool)), this, SLOT(showRestartWarning()));
    /* Display */
    connect(ui->lang, SIGNAL(valueChanged()), this, SLOT(showRestartWarning()));
    connect(ui->thirdPartyTxUrls, SIGNAL(textChanged(const QString &)), this, SLOT(showRestartWarning()));
	
}

void OptionsDialog::setMapper()
{
    /* Main */
    mapper->addMapping(ui->evolvechainAtStartup, OptionsModel::StartAtStartup);
    mapper->addMapping(ui->threadsScriptVerif, OptionsModel::ThreadsScriptVerif);
    mapper->addMapping(ui->databaseCache, OptionsModel::DatabaseCache);

    /* Wallet */
    mapper->addMapping(ui->transactionFee, OptionsModel::Fee);
    mapper->addMapping(ui->spendZeroConfChange, OptionsModel::SpendZeroConfChange);
    mapper->addMapping(ui->coinControlFeatures, OptionsModel::CoinControlFeatures);

				
	/* Identification */

    mapper->addMapping(ui->identificationEmail, OptionsModel::IdentificationEmail);
    mapper->addMapping(ui->identificationEmailCode, OptionsModel::IdentificationEmailCode);
	mapper->addMapping(ui->identificationPhone, OptionsModel::IdentificationPhone);
	mapper->addMapping(ui->identificationPhoneCode, OptionsModel::IdentificationPhoneCode);

	
    /* Network */
    mapper->addMapping(ui->mapPortUpnp, OptionsModel::MapPortUPnP);

    mapper->addMapping(ui->connectSocks, OptionsModel::ProxyUse);
    mapper->addMapping(ui->proxyIp, OptionsModel::ProxyIP);
    mapper->addMapping(ui->proxyPort, OptionsModel::ProxyPort);
    mapper->addMapping(ui->socksVersion, OptionsModel::ProxySocksVersion);

    /* Window */
#ifndef Q_OS_MAC
    mapper->addMapping(ui->minimizeToTray, OptionsModel::MinimizeToTray);
    mapper->addMapping(ui->minimizeOnClose, OptionsModel::MinimizeOnClose);
#endif

    /* Display */
    mapper->addMapping(ui->lang, OptionsModel::Language);
    mapper->addMapping(ui->unit, OptionsModel::DisplayUnit);
    mapper->addMapping(ui->displayAddresses, OptionsModel::DisplayAddresses);
    mapper->addMapping(ui->thirdPartyTxUrls, OptionsModel::ThirdPartyTxUrls);
}

void OptionsDialog::enableOkButton()
{
    /* prevent enabling of the OK button when data modified, if there is an invalid proxy address present */
    if(fProxyIpValid)
        setOkButtonState(true);
}

void OptionsDialog::disableOkButton()
{
    setOkButtonState(false);
}

void OptionsDialog::setOkButtonState(bool fState)
{
    ui->okButton->setEnabled(fState);
}

void OptionsDialog::on_resetButton_clicked()
{
    if(model)
    {
        // confirmation dialog
        QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, tr("Confirm options reset"),
            tr("Client restart required to activate changes.") + "<br><br>" + tr("Client will be shutdown, do you want to proceed?"),
            QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);

        if(btnRetVal == QMessageBox::Cancel)
            return;

        /* reset all options and close GUI */
        model->Reset();
        QApplication::quit();
    }
}
/* Identification */
void OptionsDialog::on_checkKYC_clicked()
{
	QMessageBox::StandardButton btnRetVal;
	showRestartWarning();
	if(identiKYC!="")
	{
		QString QIdentiKYC = QString::fromUtf8(identiKYC.c_str());
		btnRetVal = QMessageBox::question(this, "Let's start!" ,"You already have KYC ID: "+ QIdentiKYC+"\nDo you want to check KYC ID again?", QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
	}else{
		btnRetVal = QMessageBox::question(this, "Let's start!" ,"Get KYC ID information?", QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
	}
	ui->startVerification->setEnabled(false);				
	if(btnRetVal == QMessageBox::Cancel)
        return;
	if(btnRetVal == QMessageBox::Yes)
	{
		QString strKYCID = ui->kyc_id->text();
		
		options_writeIdentificationLog("\n\nCheck KYC ID",DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());
		options_writeIdentificationLog("**Success KYC ID:",strKYCID.toStdString());
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		
		try
		{
			std::string parameters = "/kyc/info/" + strKYCID.toStdString();
			EvolveChainJSON << options_get_http("EvolveChain.net",parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			strerr = strprintf(_("%s"), e.what());
			ui->infoLabel->setText("Please connect to internet.");
		}
		
		if(fListening==true)
		{
			using boost::property_tree::ptree;
			using boost::property_tree::read_json;
			using boost::property_tree::write_json;
			ptree NewPointer;
	  
			
			read_json (EvolveChainJSON, NewPointer);
			std::string strSuccess = NewPointer.get<std::string> ("success");
			
			
			if(strSuccess=="1"){
				std::string strKYC = NewPointer.get<std::string> ("kyc_id");
				
				
    
				
				identiKYC = strKYC;
				ui->startVerification->setEnabled(true);				
				ui->kyc_id->setEnabled(false);				
				
				options_writeIdentificationLog("**Success KYC ID:",strKYCID.toStdString());
				options_writeIdentificationLog("##KYCID##",identiKYC);
			
				
			}
		}
		
	}
	
}


void OptionsDialog::on_startVerification_clicked()
{
	QMessageBox::StandardButton btnRetVal;
	showRestartWarning();
	if(identiCode!="")
	{
		QString QIdentiCode = QString::fromUtf8(identiCode.c_str());
		btnRetVal = QMessageBox::question(this, "Let's start!" ,"You are identified with "+ QIdentiCode+"\nDo you want to start verification process?", QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
	}else{
		btnRetVal = QMessageBox::question(this, "Let's start!" ,"Do you want to start verification process?", QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
	}
	ui->getNewAddressButton->setEnabled(false);				
	ui->identificationPhoneCode->setText("");
	ui->identificationEmailCode->setText("");
	
	if(btnRetVal == QMessageBox::Cancel)
        return;
	
	if(btnRetVal == QMessageBox::Yes)
	{
	
		options_writeIdentificationLog("\n\nStart Verification",DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());
		
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		ui->identificationEmail->setEnabled(false);
		ui->emailGetCode->setEnabled(false);
		ui->startVerification->setEnabled(true);
		options_writeIdentificationLog("In Verification",identiKYC);
		try
		{
			std::string parameters = "/code/index/" + ExternalIPAddress + "/" + identiKYC;
			EvolveChainJSON << options_get_http("evolvechain.org",parameters );
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			strerr = strprintf(_("%s"), e.what());
			ui->infoLabel->setText("Please connect to internet.");
		}
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
			using boost::property_tree::ptree;
			using boost::property_tree::read_json;
			using boost::property_tree::write_json;
			ptree NewPointer;
	  
			read_json (EvolveChainJSON, NewPointer);
			std::string strVerificationCode = NewPointer.get<std::string> ("code");
			std::string strSuccess = NewPointer.get<std::string> ("success");
			std::string strIP = NewPointer.get<std::string> ("IP");
			std::string strCity = NewPointer.get<std::string> ("city");
			std::string strOrg = NewPointer.get<std::string> ("org");
			std::string strLatLon = NewPointer.get<std::string> ("latlon");
			std::string strCountry = NewPointer.get<std::string> ("country");
			std::string strPhone = NewPointer.get<std::string> ("phone");
			
			const std::string& Psearch = "00";
			const std::string& Preplace = "";
			pos = 0;
			while ((pos = strPhone.find(Psearch, pos)) != std::string::npos) {
				strPhone.replace(pos, Psearch.length(), Preplace);
				pos += Preplace.length();
			}
			std::string strInfoLabel = "Connected with IP: " + strIP + ", through " + strOrg + ", " + strCity + "(" + strLatLon + ") " + strCountry + ". Phone prefix: " + strPhone;
			QString PhoneStr = QString::fromUtf8(strPhone.c_str());
		
			if(strSuccess=="1")
			{
				ui->identificationEmail->setEnabled(true);
				ui->emailGetCode->setEnabled(true);
				ui->startVerification->setEnabled(false);
				ui->errLabel->setText("Please enter correct email address. Click on 'Get Code'. This will send an email to your address, with a 6 digit code, which you enter in next step.");
				verificationCode = QString::fromUtf8(strVerificationCode.c_str());
//				ui->infoLabel->setText(EvolveChainStr.c_str());
				ui->infoLabel->setText(strInfoLabel.c_str());
				ui->identificationPhone->setText(PhoneStr);
				options_writeIdentificationLog("**Success",strInfoLabel.c_str());
			}else{
				ui->errLabel->setText("Please connect to internet. Oops! Try again later!");			
				options_writeIdentificationLog("----Error","Please connect to internet. Oops! Try again later!");

			}
			
			//**********************************************
			
		}
	}
	
}
void OptionsDialog::on_emailGetCode_clicked()
{

	QString strIdentificationEmail = ui->identificationEmail->text();
	ui->errLabel->setText("Please enter correct email address.");
	if(strIdentificationEmail == "")
	{
		QMessageBox::warning ( this, "Alert","Please enter correct email address.", QMessageBox::Ok);
		return;
	}
					   
	QRegExp filter_re("^.+@[^\.].*\.[a-z]{2,}$");	 
	
	if(!filter_re.exactMatch(strIdentificationEmail))
	{
		QMessageBox::warning ( this, "Alert","Please enter correct email address.", QMessageBox::Ok);
		return;
	
	}
	//
    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, "Get code!" , "Get email verification code on email: " + strIdentificationEmail+ "?",
	QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);

	std::string strerr;
	bool fListening = false;
	std::stringstream EvolveChainJSON;		
	
	if(btnRetVal == QMessageBox::Cancel)
        return;

	if(btnRetVal == QMessageBox::Yes)
	{
		ui->identificationEmailCode->setEnabled(true);
		ui->emailVerify->setEnabled(true);
		//  getemailcode <email> <receivedcode>
/*
2. Use the above code for all verifications
URL: http://evolvechainx.com/code/email/nd@evolvechainx.com/TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F
getemailcode <email> <receivedcode>
returns: JSON
{"success":1,"now":1415084252,"email":"Code sent to email!"}

User checks his email:
Use this "Identification code" to verify on your evolvechainx.com client
Email: nd@evolvechainx.com
Identification Code: 382802
IP: 127.0.0.1
Date and time: 2014-11-04 06:57:30
*/

		
		std::string parameters = "/code/email/" + strIdentificationEmail.toStdString() + "/" + verificationCode.toStdString();
		//std::string strParameters = parameters.toStdString();
		try
		{
			EvolveChainJSON << options_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			strerr = strprintf(_("%s"), e.what());
			ui->infoLabel->setText("Please connect to internet.");
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
				string EvolveChainStr = EvolveChainJSON.str();  
				ui->errLabel->setText("Please check your email "+strIdentificationEmail+" for email code. Please check both INBOX and SPAM folder for an email from evolvechainx.com. Enter the code and click 'Verify'.");
				ui->infoLabel->setText(EvolveChainStr.c_str());
				options_writeIdentificationLog("**Success",strIdentificationEmail.toStdString());
			}else{
				options_writeIdentificationLog("----Error","Unable to send email!");
			}
		}
	}
}

void OptionsDialog::on_emailVerify_clicked()
{
	QString strIdentificationEmail = ui->identificationEmail->text();
	QString strIdentificationEmailCode = ui->identificationEmailCode->text();

	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;
			
	ui->errLabel->setText("Please enter correct email verification code.");
	if(strIdentificationEmailCode == "" || strIdentificationEmailCode.length() > 6 || strIdentificationEmailCode.length() < 6 )
	{
		QMessageBox::warning ( this, "Alert","Please enter correct email verification code.", QMessageBox::Ok);
		return;
	}

	
    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, "Verify email code!" , "Verify email: " + strIdentificationEmail+ " with the code "+strIdentificationEmailCode+"?", QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
	
	if(btnRetVal == QMessageBox::Cancel)
        return;

	if(btnRetVal == QMessageBox::Yes)
	{
		ui->errLabel->setText("Verifying email and code!");
/*
4. User enters the validation code: 382802 and clicks on “verify” button
URL: http://evolvechainx.com/verify/email/nd@evolvechainx.com/TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F/382802
getemailverify <name@youremail.com> <receivedcode> <code>
returns: JSON
{"success":1,"now":1415084457,"email":"Email is verified!","emailverify":"qw2345re9i903je9!"}
*/

		
		std::string parameters = "/verify/email/" + strIdentificationEmail.toStdString() + "/" + verificationCode.toStdString() + "/" + strIdentificationEmailCode.toStdString();
		//std::string strParameters = parameters.toStdString();
		try
		{
			EvolveChainJSON << options_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			strerr = strprintf(_("%s"), e.what());
			ui->infoLabel->setText("Please connect to internet.");
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
			string EvolveChainStr = EvolveChainJSON.str();  
			ui->infoLabel->setText(EvolveChainStr.c_str());
			if(strSuccess=="1")
			{
				ui->identificationEmail->setEnabled(false);
				ui->emailGetCode->setEnabled(false);
				ui->identificationEmailCode->setEnabled(false);
				ui->emailVerify->setEnabled(false);
				ui->identificationPhone->setEnabled(true);
				ui->phoneGetCode->setEnabled(true);
				options_writeIdentificationLog("**Success","Email is verified.");
				ui->errLabel->setText("Please enter correct phone number with ISD code (only numbers) [998887776666], 99 is ISD code. Click 'Get Code' to receive SMS.");
			}else{
				ui->identificationEmail->setEnabled(true);
				ui->emailGetCode->setEnabled(true);
				ui->identificationEmailCode->setEnabled(true);
				ui->emailVerify->setEnabled(true);
				ui->identificationPhone->setEnabled(false);
				ui->phoneGetCode->setEnabled(false);
				options_writeIdentificationLog("----Error","Email not verified.");
			}
		}
	}
}

void OptionsDialog::on_phoneGetCode_clicked()
{
	QString strIdentificationPhone = ui->identificationPhone->text();
	ui->errLabel->setText("Please enter correct phone number with ISD code (only numbers) [998887776666], 99 is ISD code");

	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;

	if(strIdentificationPhone == "")
	{
		QMessageBox::warning ( this, "Alert","Please enter correct phone number.", QMessageBox::Ok);
		return;
	}

	QRegExp filter_re("^(999|998|997|996|995|994|993|992|991|990|979|978|977|976|975|974|973|972|971|970|969|968|967|966|965|964|963|962|961|960|899|898|897|896|895|894|893|892|891|890|889|888|887|886|885|884|883|882|881|880|879|878|877|876|875|874|873|872|871|870|859|858|857|856|855|854|853|852|851|850|839|838|837|836|835|834|833|832|831|830|809|808|807|806|805|804|803|802|801|800|699|698|697|696|695|694|693|692|691|690|689|688|687|686|685|684|683|682|681|680|679|678|677|676|675|674|673|672|671|670|599|598|597|596|595|594|593|592|591|590|509|508|507|506|505|504|503|502|501|500|429|428|427|426|425|424|423|422|421|420|389|388|387|386|385|384|383|382|381|380|379|378|377|376|375|374|373|372|371|370|359|358|357|356|355|354|353|352|351|350|299|298|297|296|295|294|293|292|291|290|289|288|287|286|285|284|283|282|281|280|269|268|267|266|265|264|263|262|261|260|259|258|257|256|255|254|253|252|251|250|249|248|247|246|245|244|243|242|241|240|239|238|237|236|235|234|233|232|231|230|229|228|227|226|225|224|223|222|221|220|219|218|217|216|215|214|213|212|211|210|98|95|94|93|92|91|90|86|84|82|81|66|65|64|63|62|61|60|58|57|56|55|54|53|52|51|49|48|47|46|45|44|43|41|40|39|36|34|33|32|31|30|27|20|7|1)[0-9]{0,14}$");	 
	
	if(!filter_re.exactMatch(strIdentificationPhone))
	{
		QMessageBox::warning ( this, "Alert","Please enter correct phone address.", QMessageBox::Ok);
		return;
	
	}

    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, "Get phone code!" , "Get phone verification code on: " + strIdentificationPhone+ "?",
	QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);

	if(btnRetVal == QMessageBox::Cancel)
        return;

	if(btnRetVal == QMessageBox::Yes)
	{

		
/*
5. User enters his phone with international code
URL: http://evolvechainx.com/code/phone/+917597219319/TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F
getphonecode <phonenumber> <receivedcode>
returns: JSON
{"success":1,"now":1415084726,"phone":"Code sent to phone!"}

Code is sent to the phone as: 
Please enter EvolveChain verification code 448343 on your EvolveChain client.
*/

		std::string parameters = "/code/phone/+" + strIdentificationPhone.toStdString() + "/" + verificationCode.toStdString();
		//std::string strParameters = parameters.toStdString();
		
		try
		{
			EvolveChainJSON << options_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			strerr = strprintf(_("%s"), e.what());
			ui->infoLabel->setText("Please connect to internet.");
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
			string EvolveChainStr = EvolveChainJSON.str();  
			ui->infoLabel->setText(EvolveChainStr.c_str());

			if(strSuccess=="1")
			{
				ui->errLabel->setText("Please check your phone "+strIdentificationPhone+" for SMS phone code. Enter the code and click 'Verify'.");
				ui->identificationPhone->setEnabled(true);
				ui->phoneGetCode->setEnabled(true);
				ui->identificationPhoneCode->setEnabled(true);
				ui->phoneVerify->setEnabled(true);
				options_writeIdentificationLog("**Success",strIdentificationPhone.toStdString());
			}else{
				ui->identificationPhone->setEnabled(false);
				ui->phoneGetCode->setEnabled(false);
				ui->identificationPhoneCode->setEnabled(false);
				ui->phoneVerify->setEnabled(false);
				options_writeIdentificationLog("----Error","Unable to send code to phone.");
			}
		}
	}

}

void OptionsDialog::on_phoneVerify_clicked()
{
	QString strIdentificationPhone = ui->identificationPhone->text();
	QString strIdentificationPhoneCode = ui->identificationPhoneCode->text();
	ui->errLabel->setText("Please enter correct phone code.");

	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;
	
	if(strIdentificationPhoneCode == "" || strIdentificationPhoneCode.length() > 6 || strIdentificationPhoneCode.length() < 6)
	{
		QMessageBox::warning ( this, "Alert","Please enter correct phone code.", QMessageBox::Ok);
		return;
	}
	

    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, "Verify phone code!" , "Verify phone: " + strIdentificationPhone+ " with the code "+strIdentificationPhoneCode+"?", QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);
	
	if(btnRetVal == QMessageBox::Cancel)
        return;

	if(btnRetVal == QMessageBox::Yes)
	{
		ui->errLabel->setText("Enter additional info you would like to add. Only alphabets and numbers.");
/*
6. Users enters the code: 448343 and clicks on “verify” button
URL: http://evolvechainx.com/verify/phone/+917597219319/TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F/448343
getphoneverify <phonenumber> <recivedcode> <code>
returns JSON
{"success":1,"now":1415084947,"phone":"Phone is verified!","phoneverify":"1we4rt6fghju8"}
*/

		
		std::string parameters = "/verify/phone/+" + strIdentificationPhone.toStdString() + "/" + verificationCode.toStdString() + "/" + strIdentificationPhoneCode.toStdString();
		//std::string strParameters = parameters.toStdString();

		
		try
		{
			EvolveChainJSON << options_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			strerr = strprintf(_("%s"), e.what());
			ui->infoLabel->setText("Please connect to internet.");
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
			string EvolveChainStr = EvolveChainJSON.str();  
			ui->infoLabel->setText(EvolveChainStr.c_str());
			if(strSuccess=="1")
			{
				ui->identificationPhone->setEnabled(false);
				ui->phoneGetCode->setEnabled(false);
				ui->identificationPhoneCode->setEnabled(false);
				ui->phoneVerify->setEnabled(false);
				ui->confirmVerification->setEnabled(true);
				ui->identificationExtra->setEnabled(true);
				options_writeIdentificationLog("**Success","Phone verified.");
			}else{
				ui->identificationPhone->setEnabled(true);
				ui->phoneGetCode->setEnabled(true);
				ui->identificationPhoneCode->setEnabled(true);
				ui->phoneVerify->setEnabled(true);
				ui->confirmVerification->setEnabled(false);
				options_writeIdentificationLog("----Error","Phone not verified.");
			}
		}
	}
}

void OptionsDialog::on_confirmVerification_clicked()
{
	showRestartWarning(true);
	QString strIdentificationEmailCode = ui->identificationEmailCode->text();
	QString strIdentificationPhoneCode = ui->identificationPhoneCode->text();
	QString strIdentificationExtra 	   = ui->identificationExtra->text();
	QRegExp filter_re("^([a-zA-Z0-9])([a-zA-Z0-9_ ]){0,}");	 
	
	if(!filter_re.exactMatch(strIdentificationExtra))
	{
		QMessageBox::warning ( this, "Alert","Only characters and numbers, length less than 64 characters", QMessageBox::Ok);
		return;
	
	}	
	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;

    QMessageBox::StandardButton btnRetVal = QMessageBox::question(this, "Finalize" , "Confirm Verification? ",
	QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Cancel);

	if(btnRetVal == QMessageBox::Cancel)
        return;

	if(btnRetVal == QMessageBox::Yes)
	{
		ui->confirmVerification->setEnabled(false);
		ui->errLabel->setText("Confirming!");
		

/*
7. Once the EvolveChain Client gets both the email / phone verified.
The EvolveChain client will get an address from the client using the method
getnewaddress nd@evolvechainx.com+917597219319 
1GKN8Ab6qNQnocGPDd7mgzMUBSCNqQHMgX
URL: http://evolvechainx.com/verify/verified/TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F/123456/654788/1GKN8Ab6qNQnocGPDd7mgzMUBSCNqQHMgX/SSNorPANorGovtID
setidentification <receivedcode>  <emailcode> <phonecode> <evolvechainaddress> <Additional Info>
returns: JSON
{"success":1,"now":1415085451,"error":"Client verified","txid":"f0bcb6499347f1804335ad9608abb97ff674c634c8be2091a3039efabe64883f"}
*/

		QString evolvechainAddresses = "";
		////////////////////////////////////////////////////////////////////////
		// Generate New Address
		// Parse the account first so we don't generate a key if there's an error
		string strAccount = "EVC Identified";

		if (!pwalletMain->IsLocked())
			pwalletMain->TopUpKeyPool();
		// Generate a new key that is added to wallet
		CPubKey newKey;
		if (!pwalletMain->GetKeyFromPool(newKey))
			throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
		CKeyID keyID = newKey.GetID();

		pwalletMain->SetAddressBook(keyID, strAccount, "receive");
		std::string evolvechainAddressesStr = "";
		evolvechainAddressesStr = CEvolveChainAddress(keyID).ToString();
		evolvechainAddresses = QString::fromUtf8(evolvechainAddressesStr.c_str());
		////////////////////////////////////////////////////////////////////////
		
			size_t pos = 0;
			const std::string& searchSpace = " ";
			const std::string& replaceSpace ="%20";
			std::string IdentificationExtraString = strIdentificationExtra.toStdString();
			std::string buffer;
			buffer.reserve(IdentificationExtraString.size());
				for(size_t pos = 0; pos != IdentificationExtraString.size(); ++pos) {
					switch(IdentificationExtraString[pos]) {
						case '&':  buffer.append("%26");	break;
						case ' ':  buffer.append("%20");	break;
						case '/':  buffer.append("%2F");	break;
						case ':':  buffer.append("%3A");	break;
						case '=':  buffer.append("%3D");	break;
						case '?':  buffer.append("%3F");	break;
						case ';':  buffer.append("%3B");	break;
						case '\"': buffer.append("%22");	break;
						case '\'': buffer.append("%27");	break;
						case '<':  buffer.append("%3C");	break;
						case '>':  buffer.append("%3E");	break;
						case '\\':  buffer.append("%5C");	break;
						default:   buffer.append(&IdentificationExtraString[pos], 1); break;
					}
				}
			IdentificationExtraString.swap(buffer);
			strIdentificationExtra = QString::fromUtf8(IdentificationExtraString.c_str());
		
		std::string parameters = "/verify/verified/" + verificationCode.toStdString() + "/" + strIdentificationEmailCode.toStdString() + "/" + strIdentificationPhoneCode.toStdString() + "/" + evolvechainAddresses.toStdString() + "/" + strIdentificationExtra.toStdString();
		// std::string strParameters = parameters.toStdString();


		try
		{
			EvolveChainJSON << options_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			strerr = strprintf(_("%s"), e.what());
			ui->infoLabel->setText("Please connect to internet.");
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
				std::string strSecretCode = NewPointer.get<std::string> ("secret");			
				std::string strEmailCode = NewPointer.get<std::string> ("email");
				std::string strPhoneCode = NewPointer.get<std::string> ("phone");

				IdentifiedCode = QString::fromUtf8(strSecretCode.c_str());
				string EvolveChainStr = EvolveChainJSON.str();  
				ui->infoLabel->setText("EvolveChain address: "+QString::fromUtf8(evolvechainAddressesStr.c_str()) + " is confirmed, you can receive coins on this address. Email: " + strEmailCode.c_str() + " Phone: "+ strPhoneCode.c_str());
				ui->startVerification->setEnabled(true);				
				ui->getNewAddressButton->setEnabled(true);				
				options_writeIdentificationLog("**Success","EvolveChain address: "+evolvechainAddressesStr + " is confirmed, you can receive coins on this address. Email: " + strEmailCode + " Phone: "+ strPhoneCode);
				options_writeIdentificationLog("@@EvolveChain address:",evolvechainAddressesStr);
				options_writeIdentificationLog("**ExtraInfo",strIdentificationExtra.toStdString());
				options_writeIdentificationLog("##EMAIL##",strEmailCode);
				options_writeIdentificationLog("##PHONE##",strPhoneCode);
				options_writeIdentificationLog("##CODE##",strSecretCode);
				
				

			}else{
				std::string strError = NewPointer.get<std::string> ("error");
				ui->infoLabel->setText("Unable to confirm your identification a this time, try again!");
				ui->startVerification->setEnabled(true);				
				options_writeIdentificationLog("----Error",strError);
			}
		}
	}
}

void OptionsDialog::on_getNewAddressButton_clicked()
{
	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;

	ui->infoLabel->setText("Let us confirm with the same code: "+ IdentifiedCode  );
	QString evolvechainAddresses = "";
	////////////////////////////////////////////////////////////////////////
	// Generate New Address
	// Parse the account first so we don't generate a key if there's an error
	string strAccount = "EVC Identified";

	if (!pwalletMain->IsLocked())
		pwalletMain->TopUpKeyPool();
	// Generate a new key that is added to wallet
	CPubKey newKey;
	if (!pwalletMain->GetKeyFromPool(newKey))
		throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
	CKeyID keyID = newKey.GetID();

	pwalletMain->SetAddressBook(keyID, strAccount, "receive");
	std::string evolvechainAddressesStr = "";
	evolvechainAddressesStr = CEvolveChainAddress(keyID).ToString();
	evolvechainAddresses = QString::fromUtf8(evolvechainAddressesStr.c_str());
	////////////////////////////////////////////////////////////////////////

	std::string parameters = "/verify/addaddress/" + IdentifiedCode.toStdString() + "/" + evolvechainAddresses.toStdString();

	try
	{
		EvolveChainJSON << options_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		strerr = strprintf(_("%s"), e.what());
		ui->infoLabel->setText("Please connect to internet.");
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
		std::string strSecretCode = NewPointer.get<std::string> ("secret");
		IdentifiedCode = QString::fromUtf8(strSecretCode.c_str());
		if(strSuccess=="1")
		{
			string EvolveChainStr = EvolveChainJSON.str();  
			ui->infoLabel->setText("EvolveChain address: "+QString::fromUtf8(evolvechainAddressesStr.c_str()) + " is confirmed, you can receive coins on this address.");
			ui->startVerification->setEnabled(true);				
			ui->getNewAddressButton->setEnabled(false);				
			options_writeIdentificationLog("**Success","EvolveChain address: "+evolvechainAddressesStr + " is confirmed, you can receive coins on this address.");
			options_writeIdentificationLog("@@EvolveChain address:",evolvechainAddressesStr);
			options_writeIdentificationLog("##CODE##",strSecretCode);
			
		}else{
			ui->infoLabel->setText("Unable to confirm your identification a this time, try again!");
			ui->startVerification->setEnabled(true);				
			options_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
		}
	}
}

void OptionsDialog::on_okButton_clicked()
{
    mapper->submit();
    accept();
}

void OptionsDialog::on_cancelButton_clicked()
{
    reject();
}

void OptionsDialog::showRestartWarning(bool fPersistent)
{
    ui->statusLabel->setStyleSheet("QLabel { color: red; }");

    if(fPersistent)
    {
        ui->statusLabel->setText(tr("Client restart required to activate changes."));
    }
    else
    {
        ui->statusLabel->setText(tr("This change would require a client restart."));
        // clear non-persistent status label after 10 seconds
        // Todo: should perhaps be a class attribute, if we extend the use of statusLabel
        QTimer::singleShot(10000, this, SLOT(clearStatusLabel()));
    }
}

void OptionsDialog::clearStatusLabel()
{
    ui->statusLabel->clear();
}

void OptionsDialog::updateDisplayUnit()
{
    if(model)
    {
        /* Update transactionFee with the current unit */
        ui->transactionFee->setDisplayUnit(model->getDisplayUnit());
    }
}

void OptionsDialog::doProxyIpChecks(QValidatedLineEdit *pUiProxyIp, int nProxyPort)
{
    Q_UNUSED(nProxyPort);

    const std::string strAddrProxy = pUiProxyIp->text().toStdString();
    CService addrProxy;

    /* Check for a valid IPv4 / IPv6 address */
    if (!(fProxyIpValid = LookupNumeric(strAddrProxy.c_str(), addrProxy)))
    {
        disableOkButton();
        pUiProxyIp->setValid(false);
        ui->statusLabel->setStyleSheet("QLabel { color: red; }");
        ui->statusLabel->setText(tr("The supplied proxy address is invalid."));
    }
    else
    {
        enableOkButton();
        ui->statusLabel->clear();
    }
}

bool OptionsDialog::eventFilter(QObject *object, QEvent *event)
{
    if(event->type() == QEvent::FocusOut)
    {
        if(object == ui->proxyIp)
        {
            emit proxyIpChecks(ui->proxyIp, ui->proxyPort->text().toInt());
        }
    }
    return QDialog::eventFilter(object, event);
}

string options_get_http(const string &server,const string &path)
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

void options_writeIdentificationLog(std::string MessageStr,std::string Str)
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
