// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The EvolveChainX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "util.h"
#include "wallet.h"
#include "walletdb.h"


#include <stdint.h>
#include <string>

#include <boost/lexical_cast.hpp>
#include <vector>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
#include <boost/tokenizer.hpp>
#include <boost/variant.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

#include <boost/asio.hpp>
#include <exception>
#include <iostream>
#include <istream>
#include <ostream>
#include <sstream>


using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

string wallet_get_http(const string &server,const string &path);
void wallet_writeIdentificationLog(std::string MessageStr,std::string Str);
std::string SendMinValueToAddress( std::string addressStr, std::string comment, int64_t nAmount);

static CNodeSignals g_signals;
int64_t nWalletUnlockTime;
int64_t nFeeRequired;

static CCriticalSection cs_nWalletUnlockTime;


std::string HelpRequiringPassphrase()
{
    return pwalletMain && pwalletMain->IsCrypted()
        ? "\nRequires wallet passphrase to be set with walletpassphrase call."
        : "";
}

void EnsureWalletIsUnlocked()
{
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");
}

void WalletTxToJSON(const CWalletTx& wtx, Object& entry)
{
    int confirms = wtx.GetDepthInMainChain();
    entry.push_back(Pair("confirmations", confirms));
    if (wtx.IsCoinBase())
        entry.push_back(Pair("generated", true));
    if (confirms > 0)
    {
        entry.push_back(Pair("blockhash", wtx.hashBlock.GetHex()));
        entry.push_back(Pair("blockindex", wtx.nIndex));
        entry.push_back(Pair("blocktime", (int64_t)(mapBlockIndex[wtx.hashBlock]->nTime)));
    }
    uint256 hash = wtx.GetHash();
    entry.push_back(Pair("txid", hash.GetHex()));
    Array conflicts;
    BOOST_FOREACH(const uint256& conflict, wtx.GetConflicts())
        conflicts.push_back(conflict.GetHex());
    entry.push_back(Pair("walletconflicts", conflicts));
    entry.push_back(Pair("time", wtx.GetTxTime()));
    entry.push_back(Pair("timereceived", (int64_t)wtx.nTimeReceived));
    BOOST_FOREACH(const PAIRTYPE(string,string)& item, wtx.mapValue)
        entry.push_back(Pair(item.first, item.second));
}

string AccountFromValue(const Value& value)
{
    string strAccount = value.get_str();
    if (strAccount == "*")
        throw JSONRPCError(RPC_WALLET_INVALID_ACCOUNT_NAME, "Invalid account name");
    return strAccount;
}

Value getnewaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewaddress ( \"account\" )\n"
            "\nReturns a new EvolveChain address for receiving payments.\n"
            "If 'account' is specified (recommended), it is added to the address book \n"
            "so payments received with the address will be credited to 'account'.\n"
            "\nArguments:\n"
            "1. \"account\"        (string, optional) The account name for the address to be linked to. if not provided, the default account \"\" is used. It can also be set to the empty string \"\" to represent the default account. The account does not need to exist, it will be created if there is no account by the given name.\n"
            "\nResult:\n"
            "\"evolvechainaddress\"    (string) The new evolvechain address\n"
            "\nExamples:\n"
            + HelpExampleCli("getnewaddress", "")
            + HelpExampleCli("getnewaddress", "\"\"")
            + HelpExampleCli("getnewaddress", "\"myaccount\"")
            + HelpExampleRpc("getnewaddress", "\"myaccount\"")
        );
	if(identiCode!=""){
		// Parse the account first so we don't generate a key if there's an error
		string strAccount;
		if (params.size() > 0)
			strAccount = AccountFromValue(params[0]);

		if (!pwalletMain->IsLocked())
			pwalletMain->TopUpKeyPool();

		// Generate a new key that is added to wallet
		CPubKey newKey;
		if (!pwalletMain->GetKeyFromPool(newKey))
			throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");
		CKeyID keyID = newKey.GetID();

		pwalletMain->SetAddressBook(keyID, strAccount, "receive");

		
		// Record in identification.log
		////////////////////////////////
		std::string strEvolveChainAddress = CEvolveChainAddress(keyID).ToString();
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		
		std::string parameters = "/verify/addaddress/" + identiCode + "/" + strEvolveChainAddress;
		try
		{
			EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			return "Please connect to internet.";
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
			
			if(strSuccess=="1")
			{
				std::string Method = "getnewaddress "  + strAccount;
				wallet_writeIdentificationLog("Method:",Method);
				wallet_writeIdentificationLog("**Success","EvolveChain address: "+ strEvolveChainAddress + " is confirmed, you can receive coins on this address.");
				wallet_writeIdentificationLog("@@EvolveChain address:",strEvolveChainAddress);
				wallet_writeIdentificationLog("##CODE##",strSecretCode);
			}else{
				wallet_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
			}
		}

		////////////////////////////////
		
		return strEvolveChainAddress;

	}else{
		return "getnewaddress Error: You are not identified. Get yourself identified to get a new address.\nYou can use Settings->Options->Identification Tab->Start Verification\n\nYou can also use RPC command startverification\n";
	}	
}


CEvolveChainAddress GetAccountAddress(string strAccount, bool bForceNew=false)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);

    CAccount account;
    walletdb.ReadAccount(strAccount, account);

    bool bKeyUsed = false;

    // Check if the current key has been used
    if (account.vchPubKey.IsValid())
    {
        CScript scriptPubKey;
        scriptPubKey.SetDestination(account.vchPubKey.GetID());
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin();
             it != pwalletMain->mapWallet.end() && account.vchPubKey.IsValid();
             ++it)
        {
            const CWalletTx& wtx = (*it).second;
            BOOST_FOREACH(const CTxOut& txout, wtx.vout)
                if (txout.scriptPubKey == scriptPubKey)
                    bKeyUsed = true;
        }
    }

    // Generate a new key
    if (!account.vchPubKey.IsValid() || bForceNew || bKeyUsed)
    {
        if (!pwalletMain->GetKeyFromPool(account.vchPubKey))
            throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Error: Keypool ran out, please call keypoolrefill first");

        pwalletMain->SetAddressBook(account.vchPubKey.GetID(), strAccount, "receive");
        walletdb.WriteAccount(strAccount, account);
    }

    return CEvolveChainAddress(account.vchPubKey.GetID());
}

Value getaccountaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaccountaddress \"account\"\n"
            "\nReturns the current EvolveChain address for receiving payments to this account.\n"
            "\nArguments:\n"
            "1. \"account\"       (string, required) The account name for the address. It can also be set to the empty string \"\" to represent the default account. The account does not need to exist, it will be created and a new address created  if there is no account by the given name.\n"
            "\nResult:\n"
            "\"evolvechainaddress\"   (string) The account evolvechain address\n"
            "\nExamples:\n"
            + HelpExampleCli("getaccountaddress", "")
            + HelpExampleCli("getaccountaddress", "\"\"")
            + HelpExampleCli("getaccountaddress", "\"myaccount\"")
            + HelpExampleRpc("getaccountaddress", "\"myaccount\"")
        );
		
	if(identiCode!=""){
    // Parse the account first so we don't generate a key if there's an error
    string strAccount = AccountFromValue(params[0]);

    
    	// Record in identification.log
		////////////////////////////////
		std::string strEvolveChainAddress = GetAccountAddress(strAccount).ToString();
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		
		std::string parameters = "/verify/addaddress/" + identiCode + "/" + strEvolveChainAddress;
		try
		{
			EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			return "Please connect to internet.";
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
			
			if(strSuccess=="1")
			{
				std::string Method = "getaccountaddress " + strAccount;
				wallet_writeIdentificationLog("Method:",Method);
				wallet_writeIdentificationLog("**Success","EvolveChain address: "+ strEvolveChainAddress + " is confirmed, you can receive coins on this address.");
				wallet_writeIdentificationLog("@@EvolveChain address:",strEvolveChainAddress);
				wallet_writeIdentificationLog("##CODE##",strSecretCode);
			}else{
				wallet_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
			}
		}

		////////////////////////////////
	
		return strEvolveChainAddress;
	}else{
		return "getaccountaddress Error: You are not identified. Get yourself identified to get a new account address.\nYou can use Settings->Options->Identification Tab->Start Verification\n\nYou can also use RPC command startverification\n";
	}	
}


Value getrawchangeaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getrawchangeaddress\n"
            "\nReturns a new EvolveChain address, for receiving change.\n"
            "This is for use with raw transactions, NOT normal use.\n"
            "\nResult:\n"
            "\"address\"    (string) The address\n"
            "\nExamples:\n"
            + HelpExampleCli("getrawchangeaddress", "")
            + HelpExampleRpc("getrawchangeaddress", "")
       );
	if(identiCode!=""){
		if (!pwalletMain->IsLocked())
			pwalletMain->TopUpKeyPool();

		CReserveKey reservekey(pwalletMain);
		CPubKey vchPubKey;
		if (!reservekey.GetReservedKey(vchPubKey))
			throw JSONRPCError(RPC_WALLET_ERROR, "Error: Unable to obtain key for change");

		reservekey.KeepKey();

		CKeyID keyID = vchPubKey.GetID();

		// Record in identification.log
		////////////////////////////////
		std::string strEvolveChainAddress = CEvolveChainAddress(keyID).ToString();
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		
		std::string parameters = "/verify/addaddress/" + identiCode + "/" + strEvolveChainAddress;
		try
		{
			EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			return "Please connect to internet.";
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
			
			if(strSuccess=="1")
			{
				std::string Method = "getrawchangeaddress ";
				wallet_writeIdentificationLog("Method:",Method);
				wallet_writeIdentificationLog("**Success","EvolveChain address: "+ strEvolveChainAddress + " is confirmed, you can receive coins on this address.");
				wallet_writeIdentificationLog("@@EvolveChain address:",strEvolveChainAddress);
				wallet_writeIdentificationLog("##CODE##",strSecretCode);
			}else{
				wallet_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
			}
		}

		////////////////////////////////
		
		return strEvolveChainAddress;
	}else{
		return "getrawchangeaddress Error: You are not identified. Get yourself identified to get a new raw change address.\nYou can use Settings->Options->Identification Tab->Start Verification\n\nYou can also use RPC command startverification\n";
	}	
}


Value setaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "setaccount \"evolvechainaddress\" \"account\"\n"
            "\nSets the account associated with the given address.\n"
            "\nArguments:\n"
            "1. \"evolvechainaddress\"  (string, required) The evolvechain address to be associated with an account.\n"
            "2. \"account\"         (string, required) The account to assign the address to.\n"
            "\nExamples:\n"
            + HelpExampleCli("setaccount", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"tabby\"")
            + HelpExampleRpc("setaccount", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\", \"tabby\"")
        );

    CEvolveChainAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");


    string strAccount;
    if (params.size() > 1)
        strAccount = AccountFromValue(params[1]);

    // Detect when changing the account of an address that is the 'unused current key' of another account:
    if (pwalletMain->mapAddressBook.count(address.Get()))
    {
        string strOldAccount = pwalletMain->mapAddressBook[address.Get()].name;
        if (address == GetAccountAddress(strOldAccount))
            GetAccountAddress(strOldAccount, true);
    }

    pwalletMain->SetAddressBook(address.Get(), strAccount, "receive");

    return Value::null;
}


Value getaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaccount \"evolvechainaddress\"\n"
            "\nReturns the account associated with the given address.\n"
            "\nArguments:\n"
            "1. \"evolvechainaddress\"  (string, required) The evolvechain address for account lookup.\n"
            "\nResult:\n"
            "\"accountname\"        (string) the account address\n"
            "\nExamples:\n"
            + HelpExampleCli("getaccount", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\"")
            + HelpExampleRpc("getaccount", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\"")
        );

    CEvolveChainAddress address(params[0].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");

    string strAccount;
    map<CTxDestination, CAddressBookData>::iterator mi = pwalletMain->mapAddressBook.find(address.Get());
    if (mi != pwalletMain->mapAddressBook.end() && !(*mi).second.name.empty())
        strAccount = (*mi).second.name;
    return strAccount;
}


Value getaddressesbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getaddressesbyaccount \"account\"\n"
            "\nReturns the list of addresses for the given account.\n"
            "\nArguments:\n"
            "1. \"account\"  (string, required) The account name.\n"
            "\nResult:\n"
            "[                     (json array of string)\n"
            "  \"evolvechainaddress\"  (string) a evolvechain address associated with the given account\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("getaddressesbyaccount", "\"tabby\"")
            + HelpExampleRpc("getaddressesbyaccount", "\"tabby\"")
        );

    string strAccount = AccountFromValue(params[0]);

    // Find all addresses that have the given account
    Array ret;
    BOOST_FOREACH(const PAIRTYPE(CEvolveChainAddress, CAddressBookData)& item, pwalletMain->mapAddressBook)
    {
        const CEvolveChainAddress& address = item.first;
        const string& strName = item.second.name;
        if (strName == strAccount)
            ret.push_back(address.ToString());
    }
    return ret;
}

Value sendtoaddress(const Array& params, bool fHelp)
{
	if(identiCode==""){
		return "Not Identified!";
	}
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "sendtoaddress \"evolvechainaddress\" amount ( \"comment\" \"comment-to\" )\n"
            "\nSent an amount to a given address. The amount is a real and is rounded to the nearest 0.00000001\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"evolvechainaddress\"  (string, required) The evolvechain address to send to.\n"
            "2. \"amount\"      (numeric, required) The amount in btc to send. eg 0.1\n"
												"3. paytax                 bool (true - pay taxes, false - not to pay tax) \n"
            "4. \"comment\"     (string, optional) A comment used to store what the transaction is for. \n"
            "                             This is not part of the transaction, just kept in your wallet.\n"
            "5. \"comment-to\"  (string, optional) A comment to store the name of the person or organization \n"
            "                             to which you're sending the transaction. This is not part of the \n"
            "                             transaction, just kept in your wallet.\n"
            "\nResult:\n"
            "\"transactionid\"  (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
            + HelpExampleCli("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 \"donation\" \"seans outpost\"")
            + HelpExampleRpc("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1, \"donation\", \"seans outpost\"")
        );

    CEvolveChainAddress address(params[0].get_str());
				
	
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");

    // Amount
    int64_t nAmount = AmountFromValue(params[1]);
				bool payTax = params[2].get_bool();
	///////////////////////////////////////////////////////////////
	/// Tax Information calculations 
	
	std::string strAmount = boost::lexical_cast<std::string>(nAmount);
	int64_t tax = 0;
	int64_t taxTo = 0;
	
	wallet_writeIdentificationLog("**Success","Amount: "+ strAmount);
	wallet_writeIdentificationLog("**Success","Address: "+ address.ToString());
	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;
	std::string parameters = "/search/code/" + identiCode ;	
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");
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
			wallet_writeIdentificationLog("**Success","From Country: "+ fromCountry);
		}else{
			throw runtime_error("Unable to check tax calculations try later!");
		}
	}
	float rate = 0; ///this is the tax rate (in percentage)
	std::string taxAddress = ""; /// this is the address where tax is deposited
	std::string taxEmail = ""; /// this is the email address where tax is deposited
	
	float rateTo = 0; ///this is the tax rate (in percentage)
	std::string taxAddressTo = ""; /// this is the address where tax is deposited
	std::string taxEmailTo = ""; /// this is the email address where tax is deposited

	///Calculate in a loop for each recipient
	
	/// Get tax for each countries
	parameters = "/verify/address/" + address.ToString();	/// This will get the country to the recipient
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");	
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
			wallet_writeIdentificationLog("**Success","To Country: "+ toCountry);			
		}else{
			throw runtime_error("Unable to check tax calculations try later!");				
		}
	}
	
	parameters = "/search/tax/" + fromCountry + "/" + toCountry +  "/" +  strAmount;	/// This will get the tax information of the recipient country and calculate accordingly.
	wallet_writeIdentificationLog("**Success Email:",parameters);
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");				
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
			throw runtime_error(strError);	
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
			tax = nAmount * rate / 100;
			taxTo = nAmount * rateTo / 100;
			wallet_writeIdentificationLog("**Success Email:",taxEmail);
			wallet_writeIdentificationLog("**Success Rate:",boost::lexical_cast<std::string>(rate));
			wallet_writeIdentificationLog("**Success Address:",taxAddress);
			wallet_writeIdentificationLog("**Success EmailTo:",taxEmailTo);
			wallet_writeIdentificationLog("**Success RateTo:",boost::lexical_cast<std::string>(rateTo));
			wallet_writeIdentificationLog("**Success AddressTo:",taxAddressTo);
			
		}
		
	}

	///////////////////////////////////////////////////////////////	
	
    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["to"]      = params[4].get_str();

    EnsureWalletIsUnlocked();

    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
    if (strError != ""){
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
	}else{
		std::string Transactions = wtx.GetHash().GetHex() + " - Main Tx\n";
		if(tax > 0)
		{
			if (payTax == true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTax (taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain taxAddress:",taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTax.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(tax));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTax.Get(), tax, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions +  wtx.GetHash().GetHex() + " - Tax to " + taxEmail;
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax not paid:",boost::lexical_cast<std::string>(tax));
			}
		}

		if(taxTo > 0)
		{
			if(payTax==true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTaxTo (taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain taxAddressTo:",taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTaxTo.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(taxTo));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTaxTo.Get(), taxTo, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions + "\n" + wtx.GetHash().GetHex() + " - Tax to " + taxEmailTo;
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax not paid:",boost::lexical_cast<std::string>(taxTo));
			}
		}
		return Transactions;
	}
}

std::string uint64_to_string( int64_t value) {
    std::string result;
	result.reserve( 20 ); // max. 20 digits possible
    int64_t q = value;
    do {
        result += "0123456789"[ q % 10 ];
        q /= 10;
    } while ( q );
    std::reverse( result.begin(), result.end() );
	return result;
}

Value listaddressgroupings(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listaddressgroupings\n"
            "\nLists groups of addresses which have had their common ownership\n"
            "made public by common use as inputs or as the resulting change\n"
            "in past transactions\n"
            "\nResult:\n"
            "[\n"
            "  [\n"
            "    [\n"
            "      \"evolvechainaddress\",     (string) The evolvechain address\n"
            "      amount,                 (numeric) The amount in btc\n"
            "      \"account\"             (string, optional) The account\n"
            "    ]\n"
            "    ,...\n"
            "  ]\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            + HelpExampleCli("listaddressgroupings", "")
            + HelpExampleRpc("listaddressgroupings", "")
        );

    Array jsonGroupings;
    map<CTxDestination, int64_t> balances = pwalletMain->GetAddressBalances();
    BOOST_FOREACH(set<CTxDestination> grouping, pwalletMain->GetAddressGroupings())
    {
        Array jsonGrouping;
        BOOST_FOREACH(CTxDestination address, grouping)
        {
            Array addressInfo;
            addressInfo.push_back(CEvolveChainAddress(address).ToString());
            addressInfo.push_back(ValueFromAmount(balances[address]));
            {
                LOCK(pwalletMain->cs_wallet);
                if (pwalletMain->mapAddressBook.find(CEvolveChainAddress(address).Get()) != pwalletMain->mapAddressBook.end())
                    addressInfo.push_back(pwalletMain->mapAddressBook.find(CEvolveChainAddress(address).Get())->second.name);
            }
            jsonGrouping.push_back(addressInfo);
        }
        jsonGroupings.push_back(jsonGrouping);
    }
    return jsonGroupings;
}

Value signmessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "signmessage \"evolvechainaddress\" \"message\"\n"
            "\nSign a message with the private key of an address"
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
            "1. \"evolvechainaddress\"  (string, required) The evolvechain address to use for the private key.\n"
            "2. \"message\"         (string, required) The message to create a signature of.\n"
            "\nResult:\n"
            "\"signature\"          (string) The signature of the message encoded in base 64\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\", \"my message\"")
        );

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    string strMessage = params[1].get_str();

    CEvolveChainAddress addr(strAddress);
    if (!addr.IsValid())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKeyID keyID;
    if (!addr.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

Value getreceivedbyaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getreceivedbyaddress \"evolvechainaddress\" ( minconf )\n"
            "\nReturns the total amount received by the given evolvechainaddress in transactions with at least minconf confirmations.\n"
            "\nArguments:\n"
            "1. \"evolvechainaddress\"  (string, required) The evolvechain address for transactions.\n"
            "2. minconf             (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
            "\nResult:\n"
            "amount   (numeric) The total amount in btc received at this address.\n"
            "\nExamples:\n"
            "\nThe amount from transactions with at least 1 confirmation\n"
            + HelpExampleCli("getreceivedbyaddress", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\"") +
            "\nThe amount including unconfirmed transactions, zero confirmations\n"
            + HelpExampleCli("getreceivedbyaddress", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" 0") +
            "\nThe amount with at least 6 confirmation, very safe\n"
            + HelpExampleCli("getreceivedbyaddress", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" 6") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("getreceivedbyaddress", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\", 6")
       );

    // EvolveChain address
    CEvolveChainAddress address = CEvolveChainAddress(params[0].get_str());
    CScript scriptPubKey;
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");
    scriptPubKey.SetDestination(address.Get());
    if (!IsMine(*pwalletMain,scriptPubKey))
        return (double)0.0;

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Tally
    int64_t nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || !IsFinalTx(wtx))
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
            if (txout.scriptPubKey == scriptPubKey)
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
    }

    return  ValueFromAmount(nAmount);
}


Value getreceivedbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "getreceivedbyaccount \"account\" ( minconf )\n"
            "\nReturns the total amount received by addresses with <account> in transactions with at least [minconf] confirmations.\n"
            "\nArguments:\n"
            "1. \"account\"      (string, required) The selected account, may be the default account using \"\".\n"
            "2. minconf          (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
            "\nResult:\n"
            "amount              (numeric) The total amount in btc received for this account.\n"
            "\nExamples:\n"
            "\nAmount received by the default account with at least 1 confirmation\n"
            + HelpExampleCli("getreceivedbyaccount", "\"\"") +
            "\nAmount received at the tabby account including unconfirmed amounts with zero confirmations\n"
            + HelpExampleCli("getreceivedbyaccount", "\"tabby\" 0") +
            "\nThe amount with at least 6 confirmation, very safe\n"
            + HelpExampleCli("getreceivedbyaccount", "\"tabby\" 6") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("getreceivedbyaccount", "\"tabby\", 6")
        );

    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    // Get the set of pub keys assigned to account
    string strAccount = AccountFromValue(params[0]);
    set<CTxDestination> setAddress = pwalletMain->GetAccountAddresses(strAccount);

    // Tally
    int64_t nAmount = 0;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (wtx.IsCoinBase() || !IsFinalTx(wtx))
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            CTxDestination address;
            if (ExtractDestination(txout.scriptPubKey, address) && IsMine(*pwalletMain, address) && setAddress.count(address))
                if (wtx.GetDepthInMainChain() >= nMinDepth)
                    nAmount += txout.nValue;
        }
    }

    return (double)nAmount / (double)COIN;
}


int64_t GetAccountBalance(CWalletDB& walletdb, const string& strAccount, int nMinDepth)
{
    int64_t nBalance = 0;

    // Tally wallet transactions
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        if (!IsFinalTx(wtx) || wtx.GetBlocksToMaturity() > 0 || wtx.GetDepthInMainChain() < 0)
            continue;

        int64_t nReceived, nSent, nFee;
        wtx.GetAccountAmounts(strAccount, nReceived, nSent, nFee);

        if (nReceived != 0 && wtx.GetDepthInMainChain() >= nMinDepth)
            nBalance += nReceived;
        nBalance -= nSent + nFee;
    }

    // Tally internal accounting entries
    nBalance += walletdb.GetAccountCreditDebit(strAccount);

    return nBalance;
}

int64_t GetAccountBalance(const string& strAccount, int nMinDepth)
{
    CWalletDB walletdb(pwalletMain->strWalletFile);
    return GetAccountBalance(walletdb, strAccount, nMinDepth);
}


Value getbalance(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "getbalance ( \"account\" minconf )\n"
            "\nIf account is not specified, returns the server's total available balance.\n"
            "If account is specified, returns the balance in the account.\n"
            "Note that the account \"\" is not the same as leaving the parameter out.\n"
            "The server total may be different to the balance in the default \"\" account.\n"
            "\nArguments:\n"
            "1. \"account\"      (string, optional) The selected account, or \"*\" for entire wallet. It may be the default account using \"\".\n"
            "2. minconf          (numeric, optional, default=1) Only include transactions confirmed at least this many times.\n"
            "\nResult:\n"
            "amount              (numeric) The total amount in btc received for this account.\n"
            "\nExamples:\n"
            "\nThe total amount in the server across all accounts\n"
            + HelpExampleCli("getbalance", "") +
            "\nThe total amount in the server across all accounts, with at least 5 confirmations\n"
            + HelpExampleCli("getbalance", "\"*\" 6") +
            "\nThe total amount in the default account with at least 1 confirmation\n"
            + HelpExampleCli("getbalance", "\"\"") +
            "\nThe total amount in the account named tabby with at least 6 confirmations\n"
            + HelpExampleCli("getbalance", "\"tabby\" 6") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("getbalance", "\"tabby\", 6")
        );

    if (params.size() == 0)
        return  ValueFromAmount(pwalletMain->GetBalance());

    int nMinDepth = 1;
    if (params.size() > 1)
        nMinDepth = params[1].get_int();

    if (params[0].get_str() == "*") {
        // Calculate total balance a different way from GetBalance()
        // (GetBalance() sums up all unspent TxOuts)
        // getbalance and getbalance '*' 0 should return the same number
        int64_t nBalance = 0;
        for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
        {
            const CWalletTx& wtx = (*it).second;
            if (!wtx.IsTrusted() || wtx.GetBlocksToMaturity() > 0)
                continue;

            int64_t allFee;
            string strSentAccount;
            list<pair<CTxDestination, int64_t> > listReceived;
            list<pair<CTxDestination, int64_t> > listSent;
            wtx.GetAmounts(listReceived, listSent, allFee, strSentAccount);
            if (wtx.GetDepthInMainChain() >= nMinDepth)
            {
                BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64_t)& r, listReceived)
                    nBalance += r.second;
            }
            BOOST_FOREACH(const PAIRTYPE(CTxDestination,int64_t)& r, listSent)
                nBalance -= r.second;
            nBalance -= allFee;
        }
        return  ValueFromAmount(nBalance);
    }

    string strAccount = AccountFromValue(params[0]);

    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);

    return ValueFromAmount(nBalance);
}

Value getunconfirmedbalance(const Array &params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
                "getunconfirmedbalance\n"
                "Returns the server's total unconfirmed balance\n");
    return ValueFromAmount(pwalletMain->GetUnconfirmedBalance());
}


Value movecmd(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "move \"fromaccount\" \"toaccount\" amount ( minconf \"comment\" )\n"
            "\nMove a specified amount from one account in your wallet to another.\n"
            "\nArguments:\n"
            "1. \"fromaccount\"   (string, required) The name of the account to move funds from. May be the default account using \"\".\n"
            "2. \"toaccount\"     (string, required) The name of the account to move funds to. May be the default account using \"\".\n"
            "3. minconf           (numeric, optional, default=1) Only use funds with at least this many confirmations.\n"
            "4. \"comment\"       (string, optional) An optional comment, stored in the wallet only.\n"
            "\nResult:\n"
            "true|false           (boolean) true if successful.\n"
            "\nExamples:\n"
            "\nMove 0.01 btc from the default account to the account named tabby\n"
            + HelpExampleCli("move", "\"\" \"tabby\" 0.01") +
            "\nMove 0.01 btc timotei to akiko with a comment and funds have 6 confirmations\n"
            + HelpExampleCli("move", "\"timotei\" \"akiko\" 0.01 6 \"happy birthday!\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("move", "\"timotei\", \"akiko\", 0.01, 6, \"happy birthday!\"")
        );

    string strFrom = AccountFromValue(params[0]);
    string strTo = AccountFromValue(params[1]);
    int64_t nAmount = AmountFromValue(params[2]);
    if (params.size() > 3)
        // unused parameter, used to be nMinDepth, keep type-checking it though
        (void)params[3].get_int();
    string strComment;
    if (params.size() > 4)
        strComment = params[4].get_str();

    CWalletDB walletdb(pwalletMain->strWalletFile);
    if (!walletdb.TxnBegin())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    int64_t nNow = GetAdjustedTime();

    // Debit
    CAccountingEntry debit;
    debit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    debit.strAccount = strFrom;
    debit.nCreditDebit = -nAmount;
    debit.nTime = nNow;
    debit.strOtherAccount = strTo;
    debit.strComment = strComment;
    walletdb.WriteAccountingEntry(debit);

    // Credit
    CAccountingEntry credit;
    credit.nOrderPos = pwalletMain->IncOrderPosNext(&walletdb);
    credit.strAccount = strTo;
    credit.nCreditDebit = nAmount;
    credit.nTime = nNow;
    credit.strOtherAccount = strFrom;
    credit.strComment = strComment;
    walletdb.WriteAccountingEntry(credit);

    if (!walletdb.TxnCommit())
        throw JSONRPCError(RPC_DATABASE_ERROR, "database error");

    return true;
}


Value sendfrom(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 7)
        throw runtime_error(
            "sendfrom \"fromaccount\" \"toevolvechainaddress\" amount paytax ( minconf \"comment\" \"comment-to\" )\n"
            "\nSent an amount from an account to a evolvechain address.\n"
            "The amount is a real and is rounded to the nearest 0.00000001."
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
            "1. \"fromaccount\"       (string, required) The name of the account to send funds from. May be the default account using \"\".\n"
            "2. \"toevolvechainaddress\"  (string, required) The evolvechain address to send funds to.\n"
            "3. amount                (numeric, required) The amount in btc. (transaction fee is added on top).\n"
			"4. paytax                 bool (true - pay taxes, false - not to pay tax) \n"
            "5. minconf               (numeric, optional, default=1) Only use funds with at least this many confirmations.\n"
            "6. \"comment\"           (string, optional) A comment used to store what the transaction is for. \n"
            "                                     This is not part of the transaction, just kept in your wallet.\n"
            "7. \"comment-to\"        (string, optional) An optional comment to store the name of the person or organization \n"
            "                                     to which you're sending the transaction. This is not part of the transaction, \n"
            "                                     it is just kept in your wallet.\n"
            "\nResult:\n"
            "\"transactionid\"        (string) The transaction id.\n"
            "\nExamples:\n"
            "\nSend 0.01 btc from the default account to the address, must have at least 1 confirmation\n"
            + HelpExampleCli("sendfrom", "\"\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.01") +
            "\nSend 0.01 from the tabby account to the given address, funds must have at least 6 confirmations\n"
            + HelpExampleCli("sendfrom", "\"tabby\" \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.01 6 \"donation\" \"seans outpost\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("sendfrom", "\"tabby\", \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.01, 6, \"donation\", \"seans outpost\"")
        );

    string strAccount = AccountFromValue(params[0]);
    CEvolveChainAddress address(params[1].get_str());
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");
    int64_t nAmount = AmountFromValue(params[2]);
	bool payTax = params[3].get_bool();

	
	
    int nMinDepth = 1;
    if (params.size() > 4)
        nMinDepth = params[4].get_int();

    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 5 && params[5].type() != null_type && !params[5].get_str().empty())
        wtx.mapValue["comment"] = params[5].get_str();
    if (params.size() > 6 && params[6].type() != null_type && !params[6].get_str().empty())
        wtx.mapValue["to"]      = params[6].get_str();


	///////////////////////////////////////////////////////////////
	/// Tax Information calculations 
	
	std::string strAmount = boost::lexical_cast<std::string>(nAmount);
	int64_t tax = 0;
	int64_t taxTo = 0;
	
	wallet_writeIdentificationLog("**Success","Amount: "+ strAmount);
	wallet_writeIdentificationLog("**Success","Address: "+ address.ToString());
	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;
	std::string parameters = "/search/code/" + identiCode ;	

	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");
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
			wallet_writeIdentificationLog("**Success","From Country: "+ fromCountry);
		}else{
			throw runtime_error("Unable to check tax calculations try later!");
		}
	}
	float rate = 0; ///this is the tax rate (in percentage)
	std::string taxAddress = ""; /// this is the address where tax is deposited
	std::string taxEmail = ""; /// this is the email address where tax is deposited
	
	float rateTo = 0; ///this is the tax rate (in percentage)
	std::string taxAddressTo = ""; /// this is the address where tax is deposited
	std::string taxEmailTo = ""; /// this is the email address where tax is deposited

	///Calculate in a loop for each recipient
	
	/// Get tax for each countries
	parameters = "/verify/address/" + address.ToString();	/// This will get the country to the recipient
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");	
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
			wallet_writeIdentificationLog("**Success","To Country: "+ toCountry);			
		}else{
			throw runtime_error("Unable to check tax calculations try later!");				
		}
	}
	
	parameters = "/search/tax/" + fromCountry + "/" + toCountry +  "/" +  strAmount;	/// This will get the tax information of the recipient country and calculate accordingly.
	wallet_writeIdentificationLog("**Success Email:",parameters);
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");				
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
			throw runtime_error(strError);	
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
			tax = nAmount * rate / 100;
			taxTo = nAmount * rateTo / 100;
			wallet_writeIdentificationLog("**Success Email:",taxEmail);
			wallet_writeIdentificationLog("**Success Rate:",boost::lexical_cast<std::string>(rate));
			wallet_writeIdentificationLog("**Success Address:",taxAddress);
			wallet_writeIdentificationLog("**Success EmailTo:",taxEmailTo);
			wallet_writeIdentificationLog("**Success RateTo:",boost::lexical_cast<std::string>(rateTo));
			wallet_writeIdentificationLog("**Success AddressTo:",taxAddressTo);
			
		}
		
	}

	///////////////////////////////////////////////////////////////	

    EnsureWalletIsUnlocked();
    // Check funds
    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (nAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

	
    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
    if (strError != ""){
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
	}else{
		std::string Transactions = wtx.GetHash().GetHex() + " - Main Tx\n";
		if(tax > 0)
		{
			if(payTax==true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTax (taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain taxAddress:",taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTax.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(tax));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTax.Get(), tax, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions +  wtx.GetHash().GetHex() + " - Tax to " + taxEmail;
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax not paid:",boost::lexical_cast<std::string>(tax));
			}
		}

		if(taxTo > 0)
		{
			if(payTax == true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTaxTo (taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain taxAddressTo:",taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTaxTo.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(taxTo));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTaxTo.Get(), taxTo, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions + "\n" + wtx.GetHash().GetHex() + " - Tax to " + taxEmailTo;
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax not paid:",boost::lexical_cast<std::string>(taxTo));
			}
		}
		return Transactions;
	}

}


Value sendmany(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "sendmany \"fromaccount\" {\"address\":amount,...} paytax ( minconf \"comment\" )\n"
            "\nSend multiple times. Amounts are double-precision floating point numbers."
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
            "1. \"fromaccount\"         (string, required) The account to send the funds from, can be \"\" for the default account\n"
            "2. \"amounts\"             (string, required) A json object with addresses and amounts\n"
            "    {\n"
            "      \"address\":amount   (numeric) The evolvechain address is the key, the numeric amount in btc is the value\n"
            "      ,...\n"
            "    }\n"
			"3. paytax bool (true - pay taxes, false - not to pay tax) \n"
            "4. minconf                 (numeric, optional, default=1) Only use the balance confirmed at least this many times.\n"
            "5. \"comment\"             (string, optional) A comment\n"
            "\nResult:\n"
            "\"transactionid\"          (string) The transaction id for the send. Only 1 transaction is created regardless of \n"
            "                                    the number of addresses.\n"
            "\nExamples:\n"
            "\nSend two amounts to two different addresses:\n"
            + HelpExampleCli("sendmany", "\"tabby\" \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"1353tsE8YMTA4EuV7dgUXGjNFf9KpVvKHz\\\":0.02}\"") +
            "\nSend two amounts to two different addresses setting the confirmation and comment:\n"
            + HelpExampleCli("sendmany", "\"tabby\" \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"1353tsE8YMTA4EuV7dgUXGjNFf9KpVvKHz\\\":0.02}\" 6 \"testing\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("sendmany", "\"tabby\", \"{\\\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\\\":0.01,\\\"1353tsE8YMTA4EuV7dgUXGjNFf9KpVvKHz\\\":0.02}\", 6, \"testing\"")
        );

	string strAccount = AccountFromValue(params[0]);
    Object sendTo = params[1].get_obj();
	bool payTax = params[2].get_bool();
	int nMinDepth = 1;
	if (params.size() > 3)
        nMinDepth = params[3].get_int();
    CWalletTx wtx;
    wtx.strFromAccount = strAccount;
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["comment"] = params[4].get_str();

    set<CEvolveChainAddress> setAddress;
    vector<pair<CScript, int64_t> > vecSend;

    int64_t totalAmount = 0;
	std::string Transactions = "";
	
    BOOST_FOREACH(const Pair& s, sendTo)
    {
        CEvolveChainAddress address(s.name_);
        if (!address.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, string("Invalid EvolveChain address: ")+s.name_);

        if (setAddress.count(address))
            throw JSONRPCError(RPC_INVALID_PARAMETER, string("Invalid parameter, duplicated address: ")+s.name_);
        setAddress.insert(address);

        CScript scriptPubKey;
        scriptPubKey.SetDestination(address.Get());
        int64_t nAmount = AmountFromValue(s.value_);
        totalAmount += nAmount;
	
        vecSend.push_back(make_pair(scriptPubKey, nAmount));
    }
	

    EnsureWalletIsUnlocked();

    // Check funds
    int64_t nBalance = GetAccountBalance(strAccount, nMinDepth);
    if (totalAmount > nBalance)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, "Account has insufficient funds");

    // Send
    CReserveKey keyChange(pwalletMain);
    int64_t nFeeRequired = 0;
    string strFailReason;
    bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
    if (!fCreated)
        throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
    if (!pwalletMain->CommitTransaction(wtx, keyChange))
        throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");
	
	BOOST_FOREACH(const Pair& sn, sendTo)
    {
		CEvolveChainAddress addressn(sn.name_);
     

        CScript scriptPubKey;
        scriptPubKey.SetDestination(addressn.Get());
        int64_t nAmount = AmountFromValue(sn.value_);
        totalAmount += nAmount;
		///////////////////////////////////////////////////////////////
		/// Tax Information calculations 
		
		std::string strAmount = boost::lexical_cast<std::string>(nAmount);
		int64_t tax = 0;
		int64_t taxTo = 0;
		
		wallet_writeIdentificationLog("**Success","Amount: "+ strAmount);
		wallet_writeIdentificationLog("**Success","Address: "+ addressn.ToString());
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		std::string parameters = "/search/code/" + identiCode ;	
		try
		{
			EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			throw runtime_error("Unable to check tax calculations try later!");
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
				wallet_writeIdentificationLog("**Success","From Country: "+ fromCountry);
			}else{
				throw runtime_error("Unable to check tax calculations try later!");
			}
		}
		float rate = 0; ///this is the tax rate (in percentage)
		std::string taxAddress = ""; /// this is the address where tax is deposited
		std::string taxEmail = ""; /// this is the email address where tax is deposited
		
		float rateTo = 0; ///this is the tax rate (in percentage)
		std::string taxAddressTo = ""; /// this is the address where tax is deposited
		std::string taxEmailTo = ""; /// this is the email address where tax is deposited

		///Calculate in a loop for each recipient
		
		/// Get tax for each countries
		parameters = "/verify/address/" + addressn.ToString();	/// This will get the country to the recipient
		try
		{
			EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			throw runtime_error("Unable to check tax calculations try later!");	
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
				wallet_writeIdentificationLog("**Success","To Country: "+ toCountry);			
			}else{
				throw runtime_error("Unable to check tax calculations try later!");				
			}
		}
		
		parameters = "/search/tax/" + fromCountry + "/" + toCountry +  "/" +  strAmount;	/// This will get the tax information of the recipient country and calculate accordingly.
		wallet_writeIdentificationLog("**Success Email:",parameters);
		try
		{
			EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			throw runtime_error("Unable to check tax calculations try later!");				
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
				throw runtime_error(strError);	
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
				tax = nAmount * rate / 100;
				taxTo = nAmount * rateTo / 100;
				wallet_writeIdentificationLog("**Success Email:",taxEmail);
				wallet_writeIdentificationLog("**Success Rate:",boost::lexical_cast<std::string>(rate));
				wallet_writeIdentificationLog("**Success Address:",taxAddress);
				wallet_writeIdentificationLog("**Success EmailTo:",taxEmailTo);
				wallet_writeIdentificationLog("**Success RateTo:",boost::lexical_cast<std::string>(rateTo));
				wallet_writeIdentificationLog("**Success AddressTo:",taxAddressTo);
				
			}
			
		}
		
		///////////////////////////////////////////////////////////////	
		if(tax > 0)
		{
			if(payTax==true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTax (taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain taxAddress:",taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTax.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(tax));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTax.Get(), tax, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions +  wtx.GetHash().GetHex() + " - Tax to " + taxEmail + "\n";				
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax not paid:",boost::lexical_cast<std::string>(tax));
			}
		}

		if(taxTo > 0)
		{
			if(payTax== true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTaxTo (taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain taxAddressTo:",taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTaxTo.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(taxTo));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTaxTo.Get(), taxTo, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions +  wtx.GetHash().GetHex() + " - Tax to " + taxEmailTo + "\n";								
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax not paid:",boost::lexical_cast<std::string>(taxTo));
			}
		}
	}
	
	Transactions = Transactions +  wtx.GetHash().GetHex() + " - Main Tx " ;				
    return Transactions;

}

// Defined in rpcmisc.cpp
extern CScript _createmultisig_redeemScript(const Array& params);

Value addmultisigaddress(const Array& params, bool fHelp)
{
	if(identiCode!=""){
    if (fHelp || params.size() < 2 || params.size() > 3)
    {
        string msg = "addmultisigaddress nrequired [\"key\",...] ( \"account\" )\n"
            "\nAdd a nrequired-to-sign multisignature address to the wallet.\n"
            "Each key is a EvolveChain address or hex-encoded public key.\n"
            "If 'account' is specified, assign address to that account.\n"

            "\nArguments:\n"
            "1. nrequired        (numeric, required) The number of required signatures out of the n keys or addresses.\n"
            "2. \"keysobject\"   (string, required) A json array of evolvechain addresses or hex-encoded public keys\n"
            "     [\n"
            "       \"address\"  (string) evolvechain address or hex-encoded public key\n"
            "       ...,\n"
            "     ]\n"
            "3. \"account\"      (string, optional) An account to assign the addresses to.\n"

            "\nResult:\n"
            "\"evolvechainaddress\"  (string) A evolvechain address associated with the keys.\n"

            "\nExamples:\n"
            "\nAdd a multisig address from 2 addresses\n"
            + HelpExampleCli("addmultisigaddress", "2 \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("addmultisigaddress", "2, \"[\\\"16sSauSf5pF2UkUwvKGq4qjNRzBZYqgEL5\\\",\\\"171sgjn4YtPu27adkKGrdDwzRTxnRkBfKV\\\"]\"")
        ;
        throw runtime_error(msg);
    }

    string strAccount;
    if (params.size() > 2)
        strAccount = AccountFromValue(params[2]);

    // Construct using pay-to-script-hash:
    CScript inner = _createmultisig_redeemScript(params);
    CScriptID innerID = inner.GetID();
    pwalletMain->AddCScript(inner);

    pwalletMain->SetAddressBook(innerID, strAccount, "send");
	
		// Record in identification.log
		////////////////////////////////
		std::string strEvolveChainAddress = CEvolveChainAddress(innerID).ToString();
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		
		std::string parameters = "/verify/addaddress/" + identiCode + "/" + strEvolveChainAddress;
		try
		{
			EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
			fListening = true;
		}
		catch(boost::system::system_error &e)
		{	
			return "Please connect to internet.";
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
			
			if(strSuccess=="1")
			{
				std::string Method = "addmultisigaddress "  + strAccount;
				wallet_writeIdentificationLog("Method:",Method);
				wallet_writeIdentificationLog("**Success","EvolveChain address: "+ strEvolveChainAddress + " is confirmed, you can receive coins on this address.");
				wallet_writeIdentificationLog("@@EvolveChain address:",strEvolveChainAddress);
				wallet_writeIdentificationLog("##CODE##",strSecretCode);
			}else{
				wallet_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
			}
		}
		return strEvolveChainAddress;
		////////////////////////////////
	}else{
		return "addmultisigaddress Error: You are not identified. Get yourself identified to get a new multisig address.\nYou can use Settings->Options->Identification Tab->Start Verification\n\nYou can also use RPC command startverification\n";
	}
}


struct tallyitem
{
    int64_t nAmount;
    int nConf;
    vector<uint256> txids;
    tallyitem()
    {
        nAmount = 0;
        nConf = std::numeric_limits<int>::max();
    }
};

Value ListReceived(const Array& params, bool fByAccounts)
{
    // Minimum confirmations
    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    // Whether to include empty accounts
    bool fIncludeEmpty = false;
    if (params.size() > 1)
        fIncludeEmpty = params[1].get_bool();

    // Tally
    map<CEvolveChainAddress, tallyitem> mapTally;
    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;

        if (wtx.IsCoinBase() || !IsFinalTx(wtx))
            continue;

        int nDepth = wtx.GetDepthInMainChain();
        if (nDepth < nMinDepth)
            continue;

        BOOST_FOREACH(const CTxOut& txout, wtx.vout)
        {
            CTxDestination address;
            if (!ExtractDestination(txout.scriptPubKey, address) || !IsMine(*pwalletMain, address))
                continue;

            tallyitem& item = mapTally[address];
            item.nAmount += txout.nValue;
            item.nConf = min(item.nConf, nDepth);
            item.txids.push_back(wtx.GetHash());
        }
    }

    // Reply
    Array ret;
    map<string, tallyitem> mapAccountTally;
    BOOST_FOREACH(const PAIRTYPE(CEvolveChainAddress, CAddressBookData)& item, pwalletMain->mapAddressBook)
    {
        const CEvolveChainAddress& address = item.first;
        const string& strAccount = item.second.name;
        map<CEvolveChainAddress, tallyitem>::iterator it = mapTally.find(address);
        if (it == mapTally.end() && !fIncludeEmpty)
            continue;

        int64_t nAmount = 0;
        int nConf = std::numeric_limits<int>::max();
        if (it != mapTally.end())
        {
            nAmount = (*it).second.nAmount;
            nConf = (*it).second.nConf;
        }

        if (fByAccounts)
        {
            tallyitem& item = mapAccountTally[strAccount];
            item.nAmount += nAmount;
            item.nConf = min(item.nConf, nConf);
        }
        else
        {
            Object obj;
            obj.push_back(Pair("address",       address.ToString()));
            obj.push_back(Pair("account",       strAccount));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            Array transactions;
            if (it != mapTally.end())
            {
                BOOST_FOREACH(const uint256& item, (*it).second.txids)
                {
                    transactions.push_back(item.GetHex());
                }
            }
            obj.push_back(Pair("txids", transactions));
            ret.push_back(obj);
        }
    }

    if (fByAccounts)
    {
        for (map<string, tallyitem>::iterator it = mapAccountTally.begin(); it != mapAccountTally.end(); ++it)
        {
            int64_t nAmount = (*it).second.nAmount;
            int nConf = (*it).second.nConf;
            Object obj;
            obj.push_back(Pair("account",       (*it).first));
            obj.push_back(Pair("amount",        ValueFromAmount(nAmount)));
            obj.push_back(Pair("confirmations", (nConf == std::numeric_limits<int>::max() ? 0 : nConf)));
            ret.push_back(obj);
        }
    }

    return ret;
}

Value listreceivedbyaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "listreceivedbyaddress ( minconf includeempty )\n"
            "\nList balances by receiving address.\n"
            "\nArguments:\n"
            "1. minconf       (numeric, optional, default=1) The minimum number of confirmations before payments are included.\n"
            "2. includeempty  (numeric, optional, dafault=false) Whether to include addresses that haven't received any payments.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"address\" : \"receivingaddress\",  (string) The receiving address\n"
            "    \"account\" : \"accountname\",       (string) The account of the receiving address. The default account is \"\".\n"
            "    \"amount\" : x.xxx,                  (numeric) The total amount in btc received by the address\n"
            "    \"confirmations\" : n                (numeric) The number of confirmations of the most recent transaction included\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("listreceivedbyaddress", "")
            + HelpExampleCli("listreceivedbyaddress", "6 true")
            + HelpExampleRpc("listreceivedbyaddress", "6, true")
        );

    return ListReceived(params, false);
}

Value listreceivedbyaccount(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "listreceivedbyaccount ( minconf includeempty )\n"
            "\nList balances by account.\n"
            "\nArguments:\n"
            "1. minconf      (numeric, optional, default=1) The minimum number of confirmations before payments are included.\n"
            "2. includeempty (boolean, optional, default=false) Whether to include accounts that haven't received any payments.\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"account\" : \"accountname\",  (string) The account name of the receiving account\n"
            "    \"amount\" : x.xxx,             (numeric) The total amount received by addresses with this account\n"
            "    \"confirmations\" : n           (numeric) The number of confirmations of the most recent transaction included\n"
            "  }\n"
            "  ,...\n"
            "]\n"

            "\nExamples:\n"
            + HelpExampleCli("listreceivedbyaccount", "")
            + HelpExampleCli("listreceivedbyaccount", "6 true")
            + HelpExampleRpc("listreceivedbyaccount", "6, true")
        );

    return ListReceived(params, true);
}

static void MaybePushAddress(Object & entry, const CTxDestination &dest)
{
    CEvolveChainAddress addr;
    if (addr.Set(dest))
        entry.push_back(Pair("address", addr.ToString()));
}

void ListTransactions(const CWalletTx& wtx, const string& strAccount, int nMinDepth, bool fLong, Array& ret)
{
    int64_t nFee;
    string strSentAccount;
    list<pair<CTxDestination, int64_t> > listReceived;
    list<pair<CTxDestination, int64_t> > listSent;

    wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);

    bool fAllAccounts = (strAccount == string("*"));

    // Sent
    if ((!listSent.empty() || nFee != 0) && (fAllAccounts || strAccount == strSentAccount))
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& s, listSent)
        {
            Object entry;
            entry.push_back(Pair("account", strSentAccount));
            MaybePushAddress(entry, s.first);
            entry.push_back(Pair("category", "send"));
            entry.push_back(Pair("amount", ValueFromAmount(-s.second)));
            entry.push_back(Pair("fee", ValueFromAmount(-nFee)));
            if (fLong)
                WalletTxToJSON(wtx, entry);
            ret.push_back(entry);
        }
    }

    // Received
    if (listReceived.size() > 0 && wtx.GetDepthInMainChain() >= nMinDepth)
    {
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& r, listReceived)
        {
            string account;
            if (pwalletMain->mapAddressBook.count(r.first))
                account = pwalletMain->mapAddressBook[r.first].name;
            if (fAllAccounts || (account == strAccount))
            {
                Object entry;
                entry.push_back(Pair("account", account));
                MaybePushAddress(entry, r.first);
                if (wtx.IsCoinBase())
                {
                    if (wtx.GetDepthInMainChain() < 1)
                        entry.push_back(Pair("category", "orphan"));
                    else if (wtx.GetBlocksToMaturity() > 0)
                        entry.push_back(Pair("category", "immature"));
                    else
                        entry.push_back(Pair("category", "generate"));
                }
                else
                {
                    entry.push_back(Pair("category", "receive"));
                }
                entry.push_back(Pair("amount", ValueFromAmount(r.second)));
                if (fLong)
                    WalletTxToJSON(wtx, entry);
                ret.push_back(entry);
            }
        }
    }
}

void AcentryToJSON(const CAccountingEntry& acentry, const string& strAccount, Array& ret)
{
    bool fAllAccounts = (strAccount == string("*"));

    if (fAllAccounts || acentry.strAccount == strAccount)
    {
        Object entry;
        entry.push_back(Pair("account", acentry.strAccount));
        entry.push_back(Pair("category", "move"));
        entry.push_back(Pair("time", acentry.nTime));
        entry.push_back(Pair("amount", ValueFromAmount(acentry.nCreditDebit)));
        entry.push_back(Pair("otheraccount", acentry.strOtherAccount));
        entry.push_back(Pair("comment", acentry.strComment));
        ret.push_back(entry);
    }
}

Value listtransactions(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3)
        throw runtime_error(
            "listtransactions ( \"account\" count from )\n"
            "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
            "\nArguments:\n"
            "1. \"account\"    (string, optional) The account name. If not included, it will list all transactions for all accounts.\n"
            "                                     If \"\" is set, it will list transactions for the default account.\n"
            "2. count          (numeric, optional, default=10) The number of transactions to return\n"
            "3. from           (numeric, optional, default=0) The number of transactions to skip\n"

            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"account\":\"accountname\",       (string) The account name associated with the transaction. \n"
            "                                                It will be \"\" for the default account.\n"
            "    \"address\":\"evolvechainaddress\",    (string) The evolvechain address of the transaction. Not present for \n"
            "                                                move transactions (category = move).\n"
            "    \"category\":\"send|receive|move\", (string) The transaction category. 'move' is a local (off blockchain)\n"
            "                                                transaction between accounts, and not associated with an address,\n"
            "                                                transaction id or block. 'send' and 'receive' transactions are \n"
            "                                                associated with an address, transaction id and block details\n"
            "    \"amount\": x.xxx,          (numeric) The amount in btc. This is negative for the 'send' category, and for the\n"
            "                                         'move' category for moves outbound. It is positive for the 'receive' category,\n"
            "                                         and for the 'move' category for inbound funds.\n"
            "    \"fee\": x.xxx,             (numeric) The amount of the fee in btc. This is negative and only available for the \n"
            "                                         'send' category of transactions.\n"
            "    \"confirmations\": n,       (numeric) The number of confirmations for the transaction. Available for 'send' and \n"
            "                                         'receive' category of transactions.\n"
            "    \"blockhash\": \"hashvalue\", (string) The block hash containing the transaction. Available for 'send' and 'receive'\n"
            "                                          category of transactions.\n"
            "    \"blockindex\": n,          (numeric) The block index containing the transaction. Available for 'send' and 'receive'\n"
            "                                          category of transactions.\n"
            "    \"txid\": \"transactionid\", (string) The transaction id. Available for 'send' and 'receive' category of transactions.\n"
            "    \"time\": xxx,              (numeric) The transaction time in seconds since epoch (midnight Jan 1 1970 GMT).\n"
            "    \"timereceived\": xxx,      (numeric) The time received in seconds since epoch (midnight Jan 1 1970 GMT). Available \n"
            "                                          for 'send' and 'receive' category of transactions.\n"
            "    \"comment\": \"...\",       (string) If a comment is associated with the transaction.\n"
            "    \"otheraccount\": \"accountname\",  (string) For the 'move' category of transactions, the account the funds came \n"
            "                                          from (for receiving funds, positive amounts), or went to (for sending funds,\n"
            "                                          negative amounts).\n"
            "  }\n"
            "]\n"

            "\nExamples:\n"
            "\nList the most recent 10 transactions in the systems\n"
            + HelpExampleCli("listtransactions", "") +
            "\nList the most recent 10 transactions for the tabby account\n"
            + HelpExampleCli("listtransactions", "\"tabby\"") +
            "\nList transactions 100 to 120 from the tabby account\n"
            + HelpExampleCli("listtransactions", "\"tabby\" 20 100") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("listtransactions", "\"tabby\", 20, 100")
        );

    string strAccount = "*";
    if (params.size() > 0)
        strAccount = params[0].get_str();
    int nCount = 10;
    if (params.size() > 1)
        nCount = params[1].get_int();
    int nFrom = 0;
    if (params.size() > 2)
        nFrom = params[2].get_int();

    if (nCount < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative count");
    if (nFrom < 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Negative from");

    Array ret;

    std::list<CAccountingEntry> acentries;
    CWallet::TxItems txOrdered = pwalletMain->OrderedTxItems(acentries, strAccount);

    // iterate backwards until we have nCount items to return:
    for (CWallet::TxItems::reverse_iterator it = txOrdered.rbegin(); it != txOrdered.rend(); ++it)
    {
        CWalletTx *const pwtx = (*it).second.first;
        if (pwtx != 0)
            ListTransactions(*pwtx, strAccount, 0, true, ret);
        CAccountingEntry *const pacentry = (*it).second.second;
        if (pacentry != 0)
            AcentryToJSON(*pacentry, strAccount, ret);

        if ((int)ret.size() >= (nCount+nFrom)) break;
    }
    // ret is newest to oldest

    if (nFrom > (int)ret.size())
        nFrom = ret.size();
    if ((nFrom + nCount) > (int)ret.size())
        nCount = ret.size() - nFrom;
    Array::iterator first = ret.begin();
    std::advance(first, nFrom);
    Array::iterator last = ret.begin();
    std::advance(last, nFrom+nCount);

    if (last != ret.end()) ret.erase(last, ret.end());
    if (first != ret.begin()) ret.erase(ret.begin(), first);

    std::reverse(ret.begin(), ret.end()); // Return oldest to newest

    return ret;
}

Value listaccounts(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "listaccounts ( minconf )\n"
            "\nReturns Object that has account names as keys, account balances as values.\n"
            "\nArguments:\n"
            "1. minconf     (numeric, optional, default=1) Only onclude transactions with at least this many confirmations\n"
            "\nResult:\n"
            "{                      (json object where keys are account names, and values are numeric balances\n"
            "  \"account\": x.xxx,  (numeric) The property name is the account name, and the value is the total balance for the account.\n"
            "  ...\n"
            "}\n"
            "\nExamples:\n"
            "\nList account balances where there at least 1 confirmation\n"
            + HelpExampleCli("listaccounts", "") +
            "\nList account balances including zero confirmation transactions\n"
            + HelpExampleCli("listaccounts", "0") +
            "\nList account balances for 6 or more confirmations\n"
            + HelpExampleCli("listaccounts", "6") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("listaccounts", "6")
        );

    int nMinDepth = 1;
    if (params.size() > 0)
        nMinDepth = params[0].get_int();

    map<string, int64_t> mapAccountBalances;
    BOOST_FOREACH(const PAIRTYPE(CTxDestination, CAddressBookData)& entry, pwalletMain->mapAddressBook) {
        if (IsMine(*pwalletMain, entry.first)) // This address belongs to me
            mapAccountBalances[entry.second.name] = 0;
    }

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); ++it)
    {
        const CWalletTx& wtx = (*it).second;
        int64_t nFee;
        string strSentAccount;
        list<pair<CTxDestination, int64_t> > listReceived;
        list<pair<CTxDestination, int64_t> > listSent;
        int nDepth = wtx.GetDepthInMainChain();
        if (wtx.GetBlocksToMaturity() > 0 || nDepth < 0)
            continue;
        wtx.GetAmounts(listReceived, listSent, nFee, strSentAccount);
        mapAccountBalances[strSentAccount] -= nFee;
        BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& s, listSent)
            mapAccountBalances[strSentAccount] -= s.second;
        if (nDepth >= nMinDepth)
        {
            BOOST_FOREACH(const PAIRTYPE(CTxDestination, int64_t)& r, listReceived)
                if (pwalletMain->mapAddressBook.count(r.first))
                    mapAccountBalances[pwalletMain->mapAddressBook[r.first].name] += r.second;
                else
                    mapAccountBalances[""] += r.second;
        }
    }

    list<CAccountingEntry> acentries;
    CWalletDB(pwalletMain->strWalletFile).ListAccountCreditDebit("*", acentries);
    BOOST_FOREACH(const CAccountingEntry& entry, acentries)
        mapAccountBalances[entry.strAccount] += entry.nCreditDebit;

    Object ret;
    BOOST_FOREACH(const PAIRTYPE(string, int64_t)& accountBalance, mapAccountBalances) {
        ret.push_back(Pair(accountBalance.first, ValueFromAmount(accountBalance.second)));
    }
    return ret;
}

Value listsinceblock(const Array& params, bool fHelp)
{
    if (fHelp)
        throw runtime_error(
            "listsinceblock ( \"blockhash\" target-confirmations )\n"
            "\nGet all transactions in blocks since block [blockhash], or all transactions if omitted\n"
            "\nArguments:\n"
            "1. \"blockhash\"   (string, optional) The block hash to list transactions since\n"
            "2. target-confirmations:    (numeric, optional) The confirmations required, must be 1 or more\n"
            "\nResult:\n"
            "{\n"
            "  \"transactions\": [\n"
            "    \"account\":\"accountname\",       (string) The account name associated with the transaction. Will be \"\" for the default account.\n"
            "    \"address\":\"evolvechainaddress\",    (string) The evolvechain address of the transaction. Not present for move transactions (category = move).\n"
            "    \"category\":\"send|receive\",     (string) The transaction category. 'send' has negative amounts, 'receive' has positive amounts.\n"
            "    \"amount\": x.xxx,          (numeric) The amount in btc. This is negative for the 'send' category, and for the 'move' category for moves \n"
            "                                          outbound. It is positive for the 'receive' category, and for the 'move' category for inbound funds.\n"
            "    \"fee\": x.xxx,             (numeric) The amount of the fee in btc. This is negative and only available for the 'send' category of transactions.\n"
            "    \"confirmations\": n,       (numeric) The number of confirmations for the transaction. Available for 'send' and 'receive' category of transactions.\n"
            "    \"blockhash\": \"hashvalue\",     (string) The block hash containing the transaction. Available for 'send' and 'receive' category of transactions.\n"
            "    \"blockindex\": n,          (numeric) The block index containing the transaction. Available for 'send' and 'receive' category of transactions.\n"
            "    \"blocktime\": xxx,         (numeric) The block time in seconds since epoch (1 Jan 1970 GMT).\n"
            "    \"txid\": \"transactionid\",  (string) The transaction id. Available for 'send' and 'receive' category of transactions.\n"
            "    \"time\": xxx,              (numeric) The transaction time in seconds since epoch (Jan 1 1970 GMT).\n"
            "    \"timereceived\": xxx,      (numeric) The time received in seconds since epoch (Jan 1 1970 GMT). Available for 'send' and 'receive' category of transactions.\n"
            "    \"comment\": \"...\",       (string) If a comment is associated with the transaction.\n"
            "    \"to\": \"...\",            (string) If a comment to is associated with the transaction.\n"
             "  ],\n"
            "  \"lastblock\": \"lastblockhash\"     (string) The hash of the last block\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("listsinceblock", "")
            + HelpExampleCli("listsinceblock", "\"000000000000000bacf66f7497b7dc45ef753ee9a7d38571037cdb1a57f663ad\" 6")
            + HelpExampleRpc("listsinceblock", "\"000000000000000bacf66f7497b7dc45ef753ee9a7d38571037cdb1a57f663ad\", 6")
        );

    CBlockIndex *pindex = NULL;
    int target_confirms = 1;

    if (params.size() > 0)
    {
        uint256 blockId = 0;

        blockId.SetHex(params[0].get_str());
        std::map<uint256, CBlockIndex*>::iterator it = mapBlockIndex.find(blockId);
        if (it != mapBlockIndex.end())
            pindex = it->second;
    }

    if (params.size() > 1)
    {
        target_confirms = params[1].get_int();

        if (target_confirms < 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter");
    }

    int depth = pindex ? (1 + chainActive.Height() - pindex->nHeight) : -1;

    Array transactions;

    for (map<uint256, CWalletTx>::iterator it = pwalletMain->mapWallet.begin(); it != pwalletMain->mapWallet.end(); it++)
    {
        CWalletTx tx = (*it).second;

        if (depth == -1 || tx.GetDepthInMainChain() < depth)
            ListTransactions(tx, "*", 0, true, transactions);
    }

    CBlockIndex *pblockLast = chainActive[chainActive.Height() + 1 - target_confirms];
    uint256 lastblock = pblockLast ? pblockLast->GetBlockHash() : 0;

    Object ret;
    ret.push_back(Pair("transactions", transactions));
    ret.push_back(Pair("lastblock", lastblock.GetHex()));

    return ret;
}

Value gettransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettransaction \"txid\"\n"
            "\nGet detailed information about in-wallet transaction <txid>\n"
            "\nArguments:\n"
            "1. \"txid\"    (string, required) The transaction id\n"
            "\nResult:\n"
            "{\n"
            "  \"amount\" : x.xxx,        (numeric) The transaction amount in btc\n"
            "  \"confirmations\" : n,     (numeric) The number of confirmations\n"
            "  \"blockhash\" : \"hash\",  (string) The block hash\n"
            "  \"blockindex\" : xx,       (numeric) The block index\n"
            "  \"blocktime\" : ttt,       (numeric) The time in seconds since epoch (1 Jan 1970 GMT)\n"
            "  \"txid\" : \"transactionid\",   (string) The transaction id.\n"
            "  \"time\" : ttt,            (numeric) The transaction time in seconds since epoch (1 Jan 1970 GMT)\n"
            "  \"timereceived\" : ttt,    (numeric) The time received in seconds since epoch (1 Jan 1970 GMT)\n"
            "  \"details\" : [\n"
            "    {\n"
            "      \"account\" : \"accountname\",  (string) The account name involved in the transaction, can be \"\" for the default account.\n"
            "      \"address\" : \"evolvechainaddress\",   (string) The evolvechain address involved in the transaction\n"
            "      \"category\" : \"send|receive\",    (string) The category, either 'send' or 'receive'\n"
            "      \"amount\" : x.xxx                  (numeric) The amount in btc\n"
            "    }\n"
            "    ,...\n"
            "  ],\n"
            "  \"hex\" : \"data\"         (string) Raw data for transaction\n"
            "}\n"

            "\nbExamples\n"
            + HelpExampleCli("gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
            + HelpExampleRpc("gettransaction", "\"1075db55d416d3ca199f55b6084e2115b9345e16c5cf302fc80e9d5fbf5d48d\"")
        );

    uint256 hash;
    hash.SetHex(params[0].get_str());

    Object entry;
    if (!pwalletMain->mapWallet.count(hash))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid or non-wallet transaction id");
    const CWalletTx& wtx = pwalletMain->mapWallet[hash];

    int64_t nCredit = wtx.GetCredit();
    int64_t nDebit = wtx.GetDebit();
    int64_t nNet = nCredit - nDebit;
    int64_t nFee = (wtx.IsFromMe() ? wtx.GetValueOut() - nDebit : 0);

    entry.push_back(Pair("amount", ValueFromAmount(nNet - nFee)));
    if (wtx.IsFromMe())
        entry.push_back(Pair("fee", ValueFromAmount(nFee)));

    WalletTxToJSON(wtx, entry);

    Array details;
    ListTransactions(wtx, "*", 0, false, details);
    entry.push_back(Pair("details", details));

    CDataStream ssTx(SER_NETWORK, PROTOCOL_VERSION);
    ssTx << static_cast<CTransaction>(wtx);
    string strHex = HexStr(ssTx.begin(), ssTx.end());
    entry.push_back(Pair("hex", strHex));

    return entry;
}


Value backupwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "backupwallet \"destination\"\n"
            "\nSafely copies wallet.dat to destination, which can be a directory or a path with filename.\n"
            "\nArguments:\n"
            "1. \"destination\"   (string) The destination directory or file\n"
            "\nExamples:\n"
            + HelpExampleCli("backupwallet", "\"backup.dat\"")
            + HelpExampleRpc("backupwallet", "\"backup.dat\"")
        );

    string strDest = params[0].get_str();
    if (!BackupWallet(*pwalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return Value::null;
}


Value keypoolrefill(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "keypoolrefill ( newsize )\n"
            "\nFills the keypool."
            + HelpRequiringPassphrase() + "\n"
            "\nArguments\n"
            "1. newsize     (numeric, optional, default=100) The new keypool size\n"
            "\nExamples:\n"
            + HelpExampleCli("keypoolrefill", "")
            + HelpExampleRpc("keypoolrefill", "")
        );

    // 0 is interpreted by TopUpKeyPool() as the default keypool size given by -keypool
    unsigned int kpSize = 0;
    if (params.size() > 0) {
        if (params[0].get_int() < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected valid size.");
        kpSize = (unsigned int)params[0].get_int();
    }
	if(identiCode!=""){
		EnsureWalletIsUnlocked();
		pwalletMain->TopUpKeyPool(kpSize);

		if (pwalletMain->GetKeyPoolSize() < kpSize)
			throw JSONRPCError(RPC_WALLET_ERROR, "Error refreshing keypool.");
	}
    return Value::null;
}


static void LockWallet(CWallet* pWallet)
{
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = 0;
    pWallet->Lock();
}

Value walletpassphrase(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 2))
        throw runtime_error(
            "walletpassphrase \"passphrase\" timeout\n"
            "\nStores the wallet decryption key in memory for 'timeout' seconds.\n"
            "This is needed prior to performing transactions related to private keys such as sending evolvechains\n"
            "\nArguments:\n"
            "1. \"passphrase\"     (string, required) The wallet passphrase\n"
            "2. timeout            (numeric, required) The time to keep the decryption key in seconds.\n"
            "\nNote:\n"
            "Issuing the walletpassphrase command while the wallet is already unlocked will set a new unlock\n"
            "time that overrides the old one.\n"
            "\nExamples:\n"
            "\nunlock the wallet for 60 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\" 60") +
            "\nLock the wallet again (before 60 seconds)\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("walletpassphrase", "\"my pass phrase\", 60")
        );

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() > 0)
    {
        if (!pwalletMain->Unlock(strWalletPass))
            throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
    }
    else
        throw runtime_error(
            "walletpassphrase <passphrase> <timeout>\n"
            "Stores the wallet decryption key in memory for <timeout> seconds.");

    pwalletMain->TopUpKeyPool();

    int64_t nSleepTime = params[1].get_int64();
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = GetTime() + nSleepTime;
    RPCRunLater("lockwallet", boost::bind(LockWallet, pwalletMain), nSleepTime);

    return Value::null;
}


Value walletpassphrasechange(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 2))
        throw runtime_error(
            "walletpassphrasechange \"oldpassphrase\" \"newpassphrase\"\n"
            "\nChanges the wallet passphrase from 'oldpassphrase' to 'newpassphrase'.\n"
            "\nArguments:\n"
            "1. \"oldpassphrase\"      (string) The current passphrase\n"
            "2. \"newpassphrase\"      (string) The new passphrase\n"
            "\nExamples:\n"
            + HelpExampleCli("walletpassphrasechange", "\"old one\" \"new one\"")
            + HelpExampleRpc("walletpassphrasechange", "\"old one\", \"new one\"")
        );

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

    // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strOldWalletPass;
    strOldWalletPass.reserve(100);
    strOldWalletPass = params[0].get_str().c_str();

    SecureString strNewWalletPass;
    strNewWalletPass.reserve(100);
    strNewWalletPass = params[1].get_str().c_str();

    if (strOldWalletPass.length() < 1 || strNewWalletPass.length() < 1)
        throw runtime_error(
            "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
            "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");

    if (!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
        throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");

    return Value::null;
}


Value walletlock(const Array& params, bool fHelp)
{
    if (pwalletMain->IsCrypted() && (fHelp || params.size() != 0))
        throw runtime_error(
            "walletlock\n"
            "\nRemoves the wallet encryption key from memory, locking the wallet.\n"
            "After calling this method, you will need to call walletpassphrase again\n"
            "before being able to call any methods which require the wallet to be unlocked.\n"
            "\nExamples:\n"
            "\nSet the passphrase for 2 minutes to perform a transaction\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\" 120") +
            "\nPerform a send (requires passphrase set)\n"
            + HelpExampleCli("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 1.0") +
            "\nClear the passphrase since we are done before 2 minutes is up\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("walletlock", "")
        );

    if (fHelp)
        return true;
    if (!pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletlock was called.");

    {
        LOCK(cs_nWalletUnlockTime);
        pwalletMain->Lock();
        nWalletUnlockTime = 0;
    }

    return Value::null;
}


Value encryptwallet(const Array& params, bool fHelp)
{
    if (!pwalletMain->IsCrypted() && (fHelp || params.size() != 1))
        throw runtime_error(
            "encryptwallet \"passphrase\"\n"
            "\nEncrypts the wallet with 'passphrase'. This is for first time encryption.\n"
            "After this, any calls that interact with private keys such as sending or signing \n"
            "will require the passphrase to be set prior the making these calls.\n"
            "Use the walletpassphrase call for this, and then walletlock call.\n"
            "If the wallet is already encrypted, use the walletpassphrasechange call.\n"
            "Note that this will shutdown the server.\n"
            "\nArguments:\n"
            "1. \"passphrase\"    (string) The pass phrase to encrypt the wallet with. It must be at least 1 character, but should be long.\n"
            "\nExamples:\n"
            "\nEncrypt you wallet\n"
            + HelpExampleCli("encryptwallet", "\"my pass phrase\"") +
            "\nNow set the passphrase to use the wallet, such as for signing or sending evolvechain\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\"") +
            "\nNow we can so something like sign\n"
            + HelpExampleCli("signmessage", "\"evolvechainaddress\" \"test message\"") +
            "\nNow lock the wallet again by removing the passphrase\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("encryptwallet", "\"my pass phrase\"")
        );

    if (fHelp)
        return true;
    if (pwalletMain->IsCrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an encrypted wallet, but encryptwallet was called.");

    // TODO: get rid of this .c_str() by implementing SecureString::operator=(std::string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() < 1)
        throw runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");

    if (!pwalletMain->EncryptWallet(strWalletPass))
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");

    // BDB seems to have a bad habit of writing old data into
    // slack space in .dat files; that is bad if the old data is
    // unencrypted private keys. So:
    StartShutdown();
    return "wallet encrypted; EvolveChain server stopping, restart to run with encrypted wallet. The keypool has been flushed, you need to make a new backup.";
}

Value lockunspent(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error(
            "lockunspent unlock [{\"txid\":\"txid\",\"vout\":n},...]\n"
            "\nUpdates list of temporarily unspendable outputs.\n"
            "Temporarily lock (unlock=false) or unlock (unlock=true) specified transaction outputs.\n"
            "A locked transaction output will not be chosen by automatic coin selection, when spending evolvechains.\n"
            "Locks are stored in memory only. Nodes start with zero locked outputs, and the locked output list\n"
            "is always cleared (by virtue of process exit) when a node stops or fails.\n"
            "Also see the listunspent call\n"
            "\nArguments:\n"
            "1. unlock            (boolean, required) Whether to unlock (true) or lock (false) the specified transactions\n"
            "2. \"transactions\"  (string, required) A json array of objects. Each object the txid (string) vout (numeric)\n"
            "     [           (json array of json objects)\n"
            "       {\n"
            "         \"txid\":\"id\",    (string) The transaction id\n"
            "         \"vout\": n         (numeric) The output number\n"
            "       }\n"
            "       ,...\n"
            "     ]\n"

            "\nResult:\n"
            "true|false    (boolean) Whether the command was successful or not\n"

            "\nExamples:\n"
            "\nList the unspent transactions\n"
            + HelpExampleCli("listunspent", "") +
            "\nLock an unspent transaction\n"
            + HelpExampleCli("lockunspent", "false \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nList the locked transactions\n"
            + HelpExampleCli("listlockunspent", "") +
            "\nUnlock the transaction again\n"
            + HelpExampleCli("lockunspent", "true \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("lockunspent", "false, \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"")
        );

    if (params.size() == 1)
        RPCTypeCheck(params, list_of(bool_type));
    else
        RPCTypeCheck(params, list_of(bool_type)(array_type));

    bool fUnlock = params[0].get_bool();

    if (params.size() == 1) {
        if (fUnlock)
            pwalletMain->UnlockAllCoins();
        return true;
    }

    Array outputs = params[1].get_array();
    BOOST_FOREACH(Value& output, outputs)
    {
        if (output.type() != obj_type)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected object");
        const Object& o = output.get_obj();

        RPCTypeCheck(o, map_list_of("txid", str_type)("vout", int_type));

        string txid = find_value(o, "txid").get_str();
        if (!IsHex(txid))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, expected hex txid");

        int nOutput = find_value(o, "vout").get_int();
        if (nOutput < 0)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid parameter, vout must be positive");

        COutPoint outpt(uint256(txid), nOutput);

        if (fUnlock)
            pwalletMain->UnlockCoin(outpt);
        else
            pwalletMain->LockCoin(outpt);
    }

    return true;
}

Value listlockunspent(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw runtime_error(
            "listlockunspent\n"
            "\nReturns list of temporarily unspendable outputs.\n"
            "See the lockunspent call to lock and unlock transactions for spending.\n"
            "\nResult:\n"
            "[\n"
            "  {\n"
            "    \"txid\" : \"transactionid\",     (string) The transaction id locked\n"
            "    \"vout\" : n                      (numeric) The vout value\n"
            "  }\n"
            "  ,...\n"
            "]\n"
            "\nExamples:\n"
            "\nList the unspent transactions\n"
            + HelpExampleCli("listunspent", "") +
            "\nLock an unspent transaction\n"
            + HelpExampleCli("lockunspent", "false \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nList the locked transactions\n"
            + HelpExampleCli("listlockunspent", "") +
            "\nUnlock the transaction again\n"
            + HelpExampleCli("lockunspent", "true \"[{\\\"txid\\\":\\\"a08e6907dbbd3d809776dbfc5d82e371b764ed838b5655e72f463568df1aadf0\\\",\\\"vout\\\":1}]\"") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("listlockunspent", "")
        );

    vector<COutPoint> vOutpts;
    pwalletMain->ListLockedCoins(vOutpts);

    Array ret;

    BOOST_FOREACH(COutPoint &outpt, vOutpts) {
        Object o;

        o.push_back(Pair("txid", outpt.hash.GetHex()));
        o.push_back(Pair("vout", (int)outpt.n));
        ret.push_back(o);
    }

    return ret;
}

Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "settxfee amount\n"
            "\nSet the transaction fee per kB.\n"
            "\nArguments:\n"
            "1. amount         (numeric, required) The transaction fee in EVC/kB rounded to the nearest 0.00000001\n"
            "\nResult\n"
            "true|false        (boolean) Returns true if successful\n"
            "\nExamples:\n"
            + HelpExampleCli("settxfee", "0.00001")
            + HelpExampleRpc("settxfee", "0.00001")
        );

    // Amount
    int64_t nAmount = 0;
    if (params[0].get_real() != 0.0)
        nAmount = AmountFromValue(params[0]);        // rejects 0.0 amounts

    nTransactionFee = nAmount;
    return true;
}

Value getwalletinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getwalletinfo\n"
            "Returns an object containing various wallet state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total evolvechain balance of the wallet\n"
            "  \"txcount\": xxxxxxx,         (numeric) the total number of transactions in the wallet\n"
            "  \"keypoololdest\": xxxxxx,    (numeric) the timestamp (seconds since GMT epoch) of the oldest pre-generated key in the key pool\n"
            "  \"keypoolsize\": xxxx,        (numeric) how many new keys are pre-generated\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getwalletinfo", "")
            + HelpExampleRpc("getwalletinfo", "")
        );

    Object obj;
    obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
    obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetBalance())));
    obj.push_back(Pair("txcount",       (int)pwalletMain->mapWallet.size()));
    obj.push_back(Pair("keypoololdest", pwalletMain->GetOldestKeyPoolTime()));
    obj.push_back(Pair("keypoolsize",   (int)pwalletMain->GetKeyPoolSize()));
    if (pwalletMain->IsCrypted())
        obj.push_back(Pair("unlocked_until", nWalletUnlockTime));
    return obj;
}

Value getkycinfo(const Array& params, bool fHelp)
{
if (fHelp || params.size() < 1 || params.size() > 1)
		throw runtime_error(
			"getkycinfo kycid\n"
			"gets kycinfo from https://EvolveChain.net for startverification\n\n"
			"Verification Process:\n"
			"getkycinfo kycid\n"
			"startverification start\n"
			"getemailcode <name@youremail.com> <receivedcode> \n"
			"getemailverify <name@youremail.com> <receivedcode> <emailcode> \n"
			"getphonecode <phonenumber> <receivedcode> \n"
			"getphoneverify <phonenumber> <receivedcode> <phonecode> \n"
			"setidentification <receivedcode> <emailcode> <phonecode> <ExtraInfo>\n"
			+ HelpRequiringPassphrase());
			
			std::string strKYCID = params[0].get_str();
			
			wallet_writeIdentificationLog("\n\nCheck KYC ID",DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());
		 wallet_writeIdentificationLog("**Success KYC ID:",strKYCID);

			std::string strerr;
			bool fListening = false;
			std::stringstream EvolveChainJSON;		
		//////// http://EvolveChain.net/kyc/info/
			
		try
			{
				EvolveChainJSON << wallet_get_http("EvolveChain.net","/kyc/info/" + strKYCID);
				fListening = true;
			}
			catch(boost::system::system_error &e)
			{	
				wallet_writeIdentificationLog("----Error","Please connect to internet. Oops! Try again later!");
				return("Please connect to internet.");
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
			std::string strKYC = NewPointer.get<std::string> ("kyc_id");
			
			identiKYC = strKYC;
			wallet_writeIdentificationLog("**Success",identiKYC);
			
			return identiKYC;//EvolveChainJSON.str();
		}else{
			wallet_writeIdentificationLog("----Error","Unable to get KYC info!");
			return ("Unable to get KYC info, please connect to internet.");
		}
	}	

			
}

Value startverification(const Array& params, bool fHelp)
{
/*
1. Get a unique code from the server by sending a call to the server: 
URL: http://evolvechainx.com/code/
startverification 
returns: JSON
{"success":1,"now":1415083994,"code":"TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F"}
*/
	if (fHelp || params.size() < 1 || params.size() > 1)
		throw runtime_error(
			"startverification start\n"
			"returns a unique <receivedcode>, which should be used for all verification\n\n"
			"Verification Process:\n"
			"startverification start\n"
			"getemailcode <name@youremail.com> <receivedcode> \n"
			"getemailverify <name@youremail.com> <receivedcode> <emailcode> \n"
			"getphonecode <phonenumber> <receivedcode> \n"
			"getphoneverify <phonenumber> <receivedcode> <phonecode> \n"
			"setidentification <receivedcode> <emailcode> <phonecode> <ExtraInfo>\n"
			+ HelpRequiringPassphrase());
	
	wallet_writeIdentificationLog("\n\nStart Verification",DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()).c_str());

	std::stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;
	
	if(identiKYC==""){
		throw runtime_error(
			"getkycinfo kycid\n"
			"gets kycinfo from https://EvolveChain.net for startverification\n\n"
			"Verification Process:\n"
			"getkycinfo kycid\n"
			"startverification start\n"
	
			+ HelpRequiringPassphrase());
	}
	
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org","/code/index/" + ExternalIPAddress +"/"+identiKYC);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		wallet_writeIdentificationLog("----Error","Please connect to internet. Oops! Try again later!");
		return("Please connect to internet.");
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
		std::string strInfoLabel = "Connected with IP:" + strIP + ", through " + strOrg + ", " + strCity + "(" + strLatLon + ") " + strCountry + ". Phone prefix: " + strPhone;
		
		if(strSuccess=="1")
		{
			wallet_writeIdentificationLog("**Success",strInfoLabel.c_str());
			return EvolveChainJSON.str();
		}else{
			wallet_writeIdentificationLog("----Error","Please connect to internet. Oops! Try again later!");
			return("Please connect to internet. Oops! Try again later!");			
		}
	}
	
}

Value getemailcode(const Array& params, bool fHelp)
{
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
	if (fHelp || params.size() < 2 || params.size() > 2)
		throw runtime_error(
			"getemailcode <name@youremail.com> <receivedcode> \n"
			"<name@youremail.com> should be an valid email address to receive the email code \n"
			"<receivedcode> is the code you received from [startverification] \n"
			+ HelpRequiringPassphrase());

	// Email
	std::string emailStr = params[0].get_str();

	// receivedcode
	std::string receivedStr = params[1].get_str();

	std::string strerr;
	bool fListening = false;
	std::stringstream EvolveChainJSON;		

	
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", "/code/email/"+emailStr+"/"+receivedStr);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		wallet_writeIdentificationLog("----Error","Unable to send email!");
		return "Unable to send email, please connect to internet.";
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
			wallet_writeIdentificationLog("**Success",emailStr);
			return EvolveChainJSON.str();
		}else{
			wallet_writeIdentificationLog("----Error","Unable to send email!");
			return ("Unable to send email, please connect to internet.");
		}
	}


}

Value getemailverify(const Array& params, bool fHelp)
{
/*
4. User enters the validation code: 382802 and clicks on “verify” button
URL: http://evolvechainx.com/verify/email/nd@evolvechainx.com/TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F/382802
getemailverify <name@youremail.com> <receivedcode> <code>
returns: JSON
{"success":1,"now":1415084457,"email":"Email is verified!","emailverify":"qw2345re9i903je9!"}
*/
	if (fHelp || params.size() < 3 || params.size() > 3)
		throw runtime_error(
			"getemailverify <name@youremail.com> <receivedcode> <emailcode> \n"
			"<name@youremail.com> should be an valid email address  \n"
			"<receivedcode> is the code you received from [startverification] \n"
			"<emailcode> is the code received on the email address"
			+ HelpRequiringPassphrase());

	// Email
	std::string emailStr = params[0].get_str();

	// receivedcode
	std::string receivedStr = params[1].get_str();
	
	// code
	std::string codeStr = params[2].get_str();
	
	stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org","/verify/email/"+emailStr+"/"+receivedStr+"/"+codeStr);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		wallet_writeIdentificationLog("----Error","Email not verified.");
		return "Please connect to internet. Email not verified.";
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
		
		if(strSuccess=="1")
		{
			wallet_writeIdentificationLog("**Success","Email is verified.");
			return EvolveChainJSON.str();
		}else{
			wallet_writeIdentificationLog("----Error","Email not verified.");
			return "Please connect to internet. Email not verified.";
		}
	}
}
Value getphonecode(const Array& params, bool fHelp)
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
	if (fHelp || params.size() < 2 || params.size() > 2)
		throw runtime_error(
			"getphonecode <phonenumber> <receivedcode> \n"
			"<phonenumber> valid phone number with international code [+998887776666] where 99 is international country code\n"
			"<receivedcode> is the code you received from [startverification] \n"
			+ HelpRequiringPassphrase());
	
	// Phone
	std::string phoneStr = params[0].get_str();

	// receivedcode
	std::string receivedStr = params[1].get_str();

	stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org","/code/phone/"+phoneStr+"/"+receivedStr);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		wallet_writeIdentificationLog("----Error","Unable to send code to phone.");
		return "Unable to send code to phone.";
			
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

		if(strSuccess=="1")
		{
			wallet_writeIdentificationLog("**Success",phoneStr);
			return EvolveChainJSON.str();
		}else{
			wallet_writeIdentificationLog("----Error","Unable to send code to phone.");
			return "Unable to send code to phone.";
		}
	}
}

Value getphoneverify(const Array& params, bool fHelp)
{
/*
6. Users enters the code: 448343 and clicks on “verify” button
URL: http://evolvechainx.com/verify/phone/+917597219319/TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F/448343
getphoneverify <phonenumber> <recivedcode> <code>
returns JSON
{"success":1,"now":1415084947,"phone":"Phone is verified!","phoneverify":"1we4rt6fghju8"}
*/
	if (fHelp || params.size() < 3 || params.size() > 3)
		throw runtime_error(
			"getphoneverify <phonenumber> <receivedcode> <phonecode> \n"
			"<phonenumber> valid phone number with international code [+998887776666]. where 99 is international code \n"
			"<receivedcode> is the code you received from [startverification] \n"
			"<phonecode> is the code you received on your phone\n"
			+ HelpRequiringPassphrase());

	// Phone
	std::string phoneStr = params[0].get_str();

	// receivedcode
	std::string receivedStr = params[1].get_str();
	
	// code
	std::string codeStr = params[2].get_str();
	
	stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;

	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org","/verify/phone/"+phoneStr+"/"+receivedStr+"/"+codeStr);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		wallet_writeIdentificationLog("----Error","Phone not verified.");
		return "Phone not verified.";	
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
		if(strSuccess=="1")
		{
			wallet_writeIdentificationLog("**Success","Phone verified.");
			return EvolveChainJSON.str();
		}else{
			wallet_writeIdentificationLog("----Error","Phone not verified.");
			return "Phone not verified.";	
		}
	}


}
Value setidentification(const Array& params, bool fHelp)
{
/*
7. Once the EvolveChain Client gets both the email / phone verified.
The EvolveChain client will get an address from the client using the method
getnewaddress nd@evolvechainx.com+917597219319 
1GKN8Ab6qNQnocGPDd7mgzMUBSCNqQHMgX
URL: http://evolvechainx.com/verify/verified/TBQ3MIRQFSGTG6VJIPLSEZFNQ6TDUMXKXG5O27VX5PIKVAFNBB6UK6XUI2RMRV5F/1GKN8Ab6qNQnocGPDd7mgzMUBSCNqQHMgX/
setidentification <receivedcode> <evolvechainaddress> <emailcode> <phonecode>
returns: JSON
{"success":1,"now":1415085451,"error":"Client verified","txid":"f0bcb6499347f1804335ad9608abb97ff674c634c8be2091a3039efabe64883f"}
*/
	if (fHelp || params.size() < 4 || params.size() > 4)
		throw runtime_error(
			"setidentification <receivedcode> <emailcode> <phonecode> <ExtraInfo>\n"
			"<receivedcode> is the code you received from [startverification] \n"
			"<emailcode> is the code you received on your email \n"
			"<phonecode> is the code you received on your phone \n"
			"<ExtraInfo> Any extra identification info you would like to give, string only"
			+ HelpRequiringPassphrase());

	// receivedcode
	std::string codeStr = params[0].get_str();
	
	// emailcode
	std::string emailStr = params[1].get_str();
	
	// phonecode
	std::string phoneStr = params[2].get_str();
	
	//extrainfo
	std::string extraStr = params[3].get_str();
	
	
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
	
		////////////////////////////////////////////////////////////////////////
		
	stringstream EvolveChainJSON;
	std::string strerr;
	bool fListening = false;
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org","/verify/verified/"+codeStr+"/"+emailStr+"/"+phoneStr+"/"+evolvechainAddressesStr+"/"+extraStr);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		wallet_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
		return "Unable to confirm your identification a this time, try again!";
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

			wallet_writeIdentificationLog("**Success","EvolveChain address: "+evolvechainAddressesStr + " is confirmed, you can receive coins on this address. Email: " + strEmailCode + " Phone: "+ strPhoneCode);
			wallet_writeIdentificationLog("@@EvolveChain address:",evolvechainAddressesStr);
			wallet_writeIdentificationLog("**ExtraInfo",extraStr);
			wallet_writeIdentificationLog("##EMAIL##",strEmailCode);			
			wallet_writeIdentificationLog("##PHONE##",strPhoneCode);			
			wallet_writeIdentificationLog("##CODE##",strSecretCode);
			
			return EvolveChainJSON.str();			
		}else{
			wallet_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
			return "Unable to confirm your identification a this time, try again!";
		}
	}

}

Value sendopreturn(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 2 || params.size() > 2)
		throw runtime_error(
			"sendopreturn <evolvechainaddress> <comment> \n"
			"<comment> A comment! "
			+ HelpRequiringPassphrase());

	// Address
	std::string addressStr = params[0].get_str();

	// Comment
	std::string comment = params[1].get_str();

	return SendMinValueToAddress(addressStr, comment, 10000);
}

// Minimun amount which can be included in a transaction 10000 equals to 0.0001 in double
std::string SendMinValueToAddress( std::string addressStr, std::string comment, int64_t nAmount=10000){

	CEvolveChainAddress address(addressStr);
  //  bool isValid = address.IsValid();
	
	if (!address.IsValid())
		throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");


		// Wallet comments
	CWalletTx wtx;
	wtx.mapValue["comment"] = comment;

	if (pwalletMain->IsLocked())
		throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, "Error: Please enter the wallet passphrase with walletpassphrase first.");

	if (pwalletMain->IsLocked())
			throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED,
					"Error: Please enter the wallet passphrase with walletpassphrase first.");

	vector<pair<CScript, int64_t> > vecSend;
	std::string strFailReason;
	CReserveKey keyChange(pwalletMain);
	int64_t nFeeRequired = 0;

	CScript scriptPubKey1;
	scriptPubKey1.SetDestination(address.Get());

	CScript scriptPubKey2;
	std::vector<unsigned char> data(comment.begin(), comment.end());
	scriptPubKey2 << OP_RETURN << data;

	vecSend.push_back(make_pair(scriptPubKey1, nAmount));
	vecSend.push_back(make_pair(scriptPubKey2, nAmount));

	bool fCreated = pwalletMain->CreateTransaction(vecSend, wtx, keyChange, nFeeRequired, strFailReason);
	if (!fCreated)
		throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, strFailReason);
	
	if (!pwalletMain->CommitTransaction(wtx, keyChange))
		throw JSONRPCError(RPC_WALLET_ERROR, "Transaction commit failed");
	

	return wtx.GetHash().GetHex();
//		return "True";
}

string wallet_get_http(const string &server,const string &path)
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
/*
Value searchaddress(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 1 || params.size() > 1)
		throw runtime_error(
			"searchaddress <evolvechainx address>\n"
			+ HelpRequiringPassphrase());
			
	int nBestHeight = (int)chainActive.Height();
	Object objO;
	int i = 1;
	LogPrintf("\n\nSearch Address start from Height : %d \n",  i);
	for (; i <= nBestHeight; i++) {
		Object obji;
		CBlockIndex* pblockindex = chainActive[i];
		CBlock block;
		ReadBlockFromDisk(block, pblockindex);
		BOOST_FOREACH(CTransaction& tx, block.vtx){
			for (unsigned int j = 0; j < tx.vout.size(); j++)
			{
//				LogPrintf("Block height %d:, vout %d:, Address %s\n",i,j,txout.address);					
			}
			for (unsigned int j = 0; j < tx.vin.size(); j++)
			{
//				LogPrintf("Block height %d:, vin %d:, Address %s\n",i,j,txout.address);					
			}
		}
	}
}
*/
Value searchtransactions(const Array& params, bool fHelp)
{
	if (fHelp || params.size() < 2 || params.size() > 2)
		throw runtime_error(
			"searchtransactions <from-blockheight> <to-blockheight>\n"
			"if <to-blockheight> is greater than current number of blocks than \n"
			"<to-blockheight> is replaced by current number of blocks\n"
			+ HelpRequiringPassphrase());
	
	int i = 0;i = params[0].get_int();
	int FinalHeight = 1; FinalHeight = params[1].get_int();
	int nBestHeight = (int)chainActive.Height();
	
	if(FinalHeight > nBestHeight){
		FinalHeight = nBestHeight;
	}
	Object objO;
	
	LogPrintf("\n\nSearch Transactions start from Height : %d \n",  i);
	
		for (; i <= FinalHeight; i++) {
			Object obji;
			CBlockIndex* pblockindex = chainActive[i];
			CBlock block;
			ReadBlockFromDisk(block, pblockindex);
			BOOST_FOREACH(CTransaction& tx, block.vtx){
			
			std::string decodedString = "";
			std::string unDecodedStr = "";
			bool isAddressOK = false;
			bool isMetaDataOK = false;
			for (unsigned int j = 0; j < tx.vout.size(); j++)
			{
				Object objj;
				const CTxOut& txout = tx.vout[j];
				const CScript& scriptPubKey = txout.scriptPubKey;
				std::string asmStr = scriptPubKey.ToString();
					
				if(txout.nValue <= 10000)
				{
					if(asmStr.length()>9)
					{
						std::size_t found = asmStr.find("OP_RETURN");
						if(found!=std::string::npos)
						{
							unDecodedStr = asmStr.substr(found+10);
							objj.push_back(Pair("vout",       (int)j));
							objj.push_back(Pair("amount",       (int)txout.nValue));
							LogPrintf("Block height %d:, vout %d:, Amount %d\n",i,j,txout.nValue);					
							LogPrintf("scriptPubKey : %s\n", asmStr);
							LogPrintf("UnDecoded String: '%s'\n" ,unDecodedStr);
							int len = unDecodedStr.length();
							for(int i=0; i< len; i+=2)
							{
								string byte = unDecodedStr.substr(i,2);
								char chr = (char) (int)strtol(byte.c_str(), NULL, 16);
								decodedString.push_back(chr);
							}
							LogPrintf("Decoded String: %s\n" ,decodedString);
							objj.push_back(Pair("decoded",    decodedString));
							isMetaDataOK = true;
							obji.push_back(Pair("tx", objj));
							objO.push_back(Pair("height",       (int)i));
						}
					}
				}
				if(txout.nValue <= 10000){
					objO.push_back(Pair("block", obji));
				}	
			}
		}	
	}
	LogPrintf("\nSearch Transactions end to Height : %d \n",  nBestHeight);
	return objO;
}

Value sendtoemail(const Array& params, bool fHelp)
{
		if(identiCode==""){
			return "Not Identified!";
		}
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "sendtoemail \"email\" amount paytax ( \"comment\" \"comment-to\" )\n"
            "\nSent an amount to a given email address. The amount is a real and is rounded to the nearest 0.00000001\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"email\"  (string, required) The email address to send to, will be converted to greecoin address.\n"
            "2. \"amount\"      (numeric, required) The amount in btc to send. eg 0.1\n"
			"3. \"paytax\"     bool (true - pay taxes, false - not to pay tax) \n"
            "4. \"comment\"     (string, optional) A comment used to store what the transaction is for. \n"
            "                             This is not part of the transaction, just kept in your wallet.\n"
            "5. \"comment-to\"  (string, optional) A comment to store the name of the person or organization \n"
            "                             to which you're sending the transaction. This is not part of the \n"
            "                             transaction, just kept in your wallet.\n"
            "\nResult:\n"
            "\"transactionid\"  (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("sendtoemail", "\"name@youremail.com\" 0.1 true")
            + HelpExampleCli("sendtoemail", "\"name@youremail.com\" 0.1 false \"donation\" \"seans outpost\"")
            + HelpExampleRpc("sendtoemail", "\"name@youremail.com\", 0.1, false \"donation\", \"seans outpost\"")
        );

	// convert email address to evolvechain address
	/////////////////////////////////////////////
	std::string emailStr = params[0].get_str();
	bool payTax = params[2].get_bool();
	std::string strAddress = "";
	std::string strerr;
	bool fListening = false;
	std::stringstream EvolveChainJSON;		
	std::string parameters = "/search/email/"  + emailStr;	
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		return "Unable to convert email to evolvechain address, try again!";
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
			strAddress = NewPointer.get<std::string> ("address");
		}else{
			return "Unable to convert email to evolvechain address, try again!";
		}
	}
	/////////////////////////////////////////////
		
    CEvolveChainAddress address(strAddress);
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");

    // Amount
    int64_t nAmount = AmountFromValue(params[1]);
	
	///////////////////////////////////////////////////////////////
	/// Tax Information calculations 
	
	std::string strAmount = boost::lexical_cast<std::string>(nAmount);
	int64_t tax = 0;
	int64_t taxTo = 0;
	
	wallet_writeIdentificationLog("**Success","Amount: "+ strAmount);
	wallet_writeIdentificationLog("**Success","Address: "+ address.ToString());
	
	
	
	parameters = "/search/code/" + identiCode ;	
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");
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
			wallet_writeIdentificationLog("**Success","From Country: "+ fromCountry);
		}else{
			throw runtime_error("Unable to check tax calculations try later!");
		}
	}
	float rate = 0; ///this is the tax rate (in percentage)
	std::string taxAddress = ""; /// this is the address where tax is deposited
	std::string taxEmail = ""; /// this is the email address where tax is deposited
	
	float rateTo = 0; ///this is the tax rate (in percentage)
	std::string taxAddressTo = ""; /// this is the address where tax is deposited
	std::string taxEmailTo = ""; /// this is the email address where tax is deposited

	///Calculate in a loop for each recipient
	
	/// Get tax for each countries
	parameters = "/verify/address/" + address.ToString();	/// This will get the country to the recipient
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");	
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
			wallet_writeIdentificationLog("**Success","To Country: "+ toCountry);			
		}else{
			throw runtime_error("Unable to check tax calculations try later!");				
		}
	}
	
	parameters = "/search/tax/" + fromCountry + "/" + toCountry +  "/" +  strAmount;	/// This will get the tax information of the recipient country and calculate accordingly.
	wallet_writeIdentificationLog("**Success Email:",parameters);
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");				
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
			throw runtime_error(strError);	
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
			tax = nAmount * rate / 100;
			taxTo = nAmount * rateTo / 100;
			wallet_writeIdentificationLog("**Success Email:",taxEmail);
			wallet_writeIdentificationLog("**Success Rate:",boost::lexical_cast<std::string>(rate));
			wallet_writeIdentificationLog("**Success Address:",taxAddress);
			wallet_writeIdentificationLog("**Success EmailTo:",taxEmailTo);
			wallet_writeIdentificationLog("**Success RateTo:",boost::lexical_cast<std::string>(rateTo));
			wallet_writeIdentificationLog("**Success AddressTo:",taxAddressTo);
			
		}
		
	}
	
	///////////////////////////////////////////////////////////////	
	
	

    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["to"]      = params[4].get_str();

    EnsureWalletIsUnlocked();

    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
    if (strError != ""){
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
	}else{
		std::string Transactions = wtx.GetHash().GetHex() + " - Main Tx\n";
		
		if(tax > 0)
		{
			if(payTax==true){
				
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTax (taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain taxAddress:",taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTax.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(tax));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTax.Get(), tax, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions +  wtx.GetHash().GetHex() + " - Tax to " + taxEmail;
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax: Not paid ",boost::lexical_cast<std::string>(tax));
			}
		}

		if(taxTo > 0)
		{
			if(payTax==true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTaxTo (taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain taxAddressTo:",taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTaxTo.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(taxTo));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTaxTo.Get(), taxTo, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions + "\n" + wtx.GetHash().GetHex() + " - Tax to " + taxEmailTo;
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax: Not paid ",boost::lexical_cast<std::string>(taxTo));
			}
		}
		
		return Transactions;
	}
}

Value sendtophone(const Array& params, bool fHelp)
{
		if(identiCode==""){
			return "Not Identified!";
		}
    if (fHelp || params.size() < 3 || params.size() > 5)
        throw runtime_error(
            "sendtophone \"phone\" amount paytax ( \"comment\" \"comment-to\" )\n"
            "\nSent an amount to a given phone. The amount is a real and is rounded to the nearest 0.00000001\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"phone\"  (string, required) The phone number to send to, will be converted to greecoin address.\n"
            "2. \"amount\"      (numeric, required) The amount in btc to send. eg 0.1\n"
			"3. \"paytax\"     bool (true - pay taxes, false - not to pay tax) \n"
            "4. \"comment\"     (string, optional) A comment used to store what the transaction is for. \n"
            "                             This is not part of the transaction, just kept in your wallet.\n"
            "5. \"comment-to\"  (string, optional) A comment to store the name of the person or organization \n"
            "                             to which you're sending the transaction. This is not part of the \n"
            "                             transaction, just kept in your wallet.\n"
            "\nResult:\n"
            "\"transactionid\"  (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("sendtophone", "\"998887776666\" 0.1 true")
            + HelpExampleCli("sendtophone", "\"998887776666\" 0.1 false \"donation\" \"seans outpost\"")
            + HelpExampleRpc("sendtophone", "\"998887776666\", 0.1, false \"donation\", \"seans outpost\"")
        );

	// convert phone address to evolvechain address
	/////////////////////////////////////////////
	std::string phoneStr = params[0].get_str();
	bool payTax = params[2].get_bool();
	std::string strAddress = "";
	std::string strerr;
	bool fListening = false;
	std::stringstream EvolveChainJSON;		
	std::string parameters = "/search/phone/+"  + phoneStr;	
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		return "Unable to convert phone to evolvechain address, try again!";
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
			strAddress = NewPointer.get<std::string> ("address");
		}else{
			return "Unable to convert phone to evolvechain address, try again!";
		}
	}
	/////////////////////////////////////////////
		
    CEvolveChainAddress address(strAddress);
    if (!address.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");

    // Amount
    int64_t nAmount = AmountFromValue(params[1]);

		///////////////////////////////////////////////////////////////
	/// Tax Information calculations 
	
	std::string strAmount = boost::lexical_cast<std::string>(nAmount);
	int64_t tax = 0;
	int64_t taxTo = 0;
	
	wallet_writeIdentificationLog("**Success","Amount: "+ strAmount);
	wallet_writeIdentificationLog("**Success","Address: "+ address.ToString());
	parameters = "/search/code/" + identiCode ;	
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");
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
			wallet_writeIdentificationLog("**Success","From Country: "+ fromCountry);
		}else{
			throw runtime_error("Unable to check tax calculations try later!");
		}
	}
	float rate = 0; ///this is the tax rate (in percentage)
	std::string taxAddress = ""; /// this is the address where tax is deposited
	std::string taxEmail = ""; /// this is the email address where tax is deposited
	
	float rateTo = 0; ///this is the tax rate (in percentage)
	std::string taxAddressTo = ""; /// this is the address where tax is deposited
	std::string taxEmailTo = ""; /// this is the email address where tax is deposited

	///Calculate in a loop for each recipient
	
	/// Get tax for each countries
	parameters = "/verify/address/" + address.ToString();	/// This will get the country to the recipient
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");	
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
			wallet_writeIdentificationLog("**Success","To Country: "+ toCountry);			
		}else{
			throw runtime_error("Unable to check tax calculations try later!");				
		}
	}
	
	parameters = "/search/tax/" + fromCountry + "/" + toCountry +  "/" +  strAmount;	/// This will get the tax information of the recipient country and calculate accordingly.
	wallet_writeIdentificationLog("**Success Email:",parameters);
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to check tax calculations try later!");				
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
			throw runtime_error(strError);	
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
			tax = nAmount * rate / 100;
			taxTo = nAmount * rateTo / 100;
			wallet_writeIdentificationLog("**Success Email:",taxEmail);
			wallet_writeIdentificationLog("**Success Rate:",boost::lexical_cast<std::string>(rate));
			wallet_writeIdentificationLog("**Success Address:",taxAddress);
			wallet_writeIdentificationLog("**Success EmailTo:",taxEmailTo);
			wallet_writeIdentificationLog("**Success RateTo:",boost::lexical_cast<std::string>(rateTo));
			wallet_writeIdentificationLog("**Success AddressTo:",taxAddressTo);
			
		}
		
	}
	
	///////////////////////////////////////////////////////////////	

	
    // Wallet comments
    CWalletTx wtx;
    if (params.size() > 3 && params[3].type() != null_type && !params[3].get_str().empty())
        wtx.mapValue["comment"] = params[3].get_str();
    if (params.size() > 4 && params[4].type() != null_type && !params[4].get_str().empty())
        wtx.mapValue["to"]      = params[4].get_str();

    EnsureWalletIsUnlocked();

    string strError = pwalletMain->SendMoneyToDestination(address.Get(), nAmount, wtx);
    if (strError != ""){
        throw JSONRPCError(RPC_WALLET_ERROR, strError);
	}else{
		std::string Transactions = wtx.GetHash().GetHex() + " - Main Tx\n";
		if(tax > 0)
		{
			if(payTax==true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTax (taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain taxAddress:",taxAddress);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTax.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(tax));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTax.Get(), tax, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions +  wtx.GetHash().GetHex() + " - Tax to " + taxEmail;
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax not paid:",boost::lexical_cast<std::string>(tax));
			}
		}

		if(taxTo > 0)
		{
			if(payTax==true){
				CWalletTx wtx;
				EnsureWalletIsUnlocked();
				CEvolveChainAddress AddressTaxTo (taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain taxAddressTo:",taxAddressTo);
				wallet_writeIdentificationLog("@@EvolveChain AddressTax:",AddressTaxTo.ToString());
				wallet_writeIdentificationLog("@@EvolveChain Tax:",boost::lexical_cast<std::string>(taxTo));
				
				string strError = pwalletMain->SendMoneyToDestination(AddressTaxTo.Get(), taxTo, wtx);
				if (strError != ""){
					throw JSONRPCError(RPC_WALLET_ERROR, strError);
				}else{
					Transactions = Transactions + "\n" + wtx.GetHash().GetHex() + " - Tax to " + taxEmailTo;
				}
			}else{
				wallet_writeIdentificationLog("@@EvolveChain Tax not paid:",boost::lexical_cast<std::string>(taxTo));
			}
		}
		return Transactions;
	}
}


Value searchemail(const Array& params, bool fHelp)
{
   if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "searchemail \"email\" \n"
            "\nSearch an email on EvolveChainX network\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"email\"  (string, required) Email address to search.\n"
            "\nExamples:\n"
            + HelpExampleCli("searchemail", "\"email@address.com\"")
            + HelpExampleCli("searchemail", "\"email@address.com\"")
            + HelpExampleRpc("searchemail", "\"email@address.com\"")
        );
		if(identiCode==""){
			return "Not Identified!";
		}


		std::string email = params[0].get_str();
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;

		
		std::string parameters = "/search/email/" + email;	/// Search Phone 
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to search email, try later!");	
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
			std::string StrEmail = NewPointer.get<std::string> ("email");
			std::string StrPhone = NewPointer.get<std::string> ("phone");
			std::string StrAddress = NewPointer.get<std::string> ("address");
			std::string StrIP = NewPointer.get<std::string> ("ip");
			std::string StrCountry = NewPointer.get<std::string> ("country");
			std::string StrCity = NewPointer.get<std::string> ("city");
			std::string StrDateTime = NewPointer.get<std::string> ("DateTime");
			std::string StrExtra = NewPointer.get<std::string> ("extra");
			return EvolveChainJSON.str();
			}else{
			throw runtime_error("Email not registered with EvolveChainX, try later!");	
		}
	}
}

Value searchphone(const Array& params, bool fHelp)
{
  if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "searchphone \"phone\" \n"
            "\nSearch a phone on EvolveChainX network\n"
            + HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1. \"phone\"  (string, required) Phone number to search.\n"
            "\nExamples:\n"
            + HelpExampleCli("searchphone", "\"1998887776666\"")
            + HelpExampleCli("searchphone", "\"1998887776666\"")
            + HelpExampleRpc("searchphone", "\"1998887776666\"")
        );
	if(identiCode==""){
		return "Not Identified!";
	}  
								
		std::string phone = params[0].get_str();
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		
		std::string parameters = "/search/phoneno/" + phone;	/// Search Phone 
	try
	{
		EvolveChainJSON << wallet_get_http("evolvechain.org", parameters);
		fListening = true;
	}
	catch(boost::system::system_error &e)
	{	
		throw runtime_error("Unable to search phone, try later!");	
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
			
			std::string StrEmail = NewPointer.get<std::string> ("email");
			std::string StrPhone = NewPointer.get<std::string> ("phone");
			std::string StrAddress = NewPointer.get<std::string> ("address");
			std::string StrIP = NewPointer.get<std::string> ("ip");
			std::string StrCountry = NewPointer.get<std::string> ("country");
			std::string StrCity = NewPointer.get<std::string> ("city");
			std::string StrDateTime = NewPointer.get<std::string> ("DateTime");
			std::string StrExtra = NewPointer.get<std::string> ("extra");
			return EvolveChainJSON.str();
		}else{
			throw runtime_error("Phone not registered with EvolveChainX, try later!");	
		}
	}
}

void wallet_writeIdentificationLog(std::string MessageStr,std::string Str)
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