// Copyright (c) 2014-2015 The EvolveChainX developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "main.h"
#include "sync.h"
#include "wallet.h"

#include <fstream>
#include <stdint.h>

#include <stdint.h>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "json/json_spirit_value.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

void EnsureWalletIsUnlocked();
string dump_get_http(const string &server,const string &path);
void dump_writeIdentificationLog(std::string MessageStr,std::string Str);

std::string static EncodeDumpTime(int64_t nTime) {
    return DateTimeStrFormat("%Y-%m-%dT%H:%M:%SZ", nTime);
}

int64_t static DecodeDumpTime(const std::string &str) {
    static const boost::posix_time::ptime epoch = boost::posix_time::from_time_t(0);
    static const std::locale loc(std::locale::classic(),
        new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%SZ"));
    std::istringstream iss(str);
    iss.imbue(loc);
    boost::posix_time::ptime ptime(boost::date_time::not_a_date_time);
    iss >> ptime;
    if (ptime.is_not_a_date_time())
        return 0;
    return (ptime - epoch).total_seconds();
}

std::string static EncodeDumpString(const std::string &str) {
    std::stringstream ret;
    BOOST_FOREACH(unsigned char c, str) {
        if (c <= 32 || c >= 128 || c == '%') {
            ret << '%' << HexStr(&c, &c + 1);
        } else {
            ret << c;
        }
    }
    return ret.str();
}

std::string DecodeDumpString(const std::string &str) {
    std::stringstream ret;
    for (unsigned int pos = 0; pos < str.length(); pos++) {
        unsigned char c = str[pos];
        if (c == '%' && pos+2 < str.length()) {
            c = (((str[pos+1]>>6)*9+((str[pos+1]-'0')&15)) << 4) | 
                ((str[pos+2]>>6)*9+((str[pos+2]-'0')&15));
            pos += 2;
        }
        ret << c;
    }
    return ret.str();
}

Value importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "importprivkey \"evolvechainprivkey\" ( \"label\" rescan )\n"
            "\nAdds a private key (as returned by dumpprivkey) to your wallet.\n"
            "\nArguments:\n"
            "1. \"evolvechainprivkey\"   (string, required) The private key (see dumpprivkey)\n"
            "2. \"label\"            (string, optional) an optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nExamples:\n"
            "\nDump a private key\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"") +
            "\nImport the private key\n"
            + HelpExampleCli("importprivkey", "\"mykey\"") +
            "\nImport using a label\n"
            + HelpExampleCli("importprivkey", "\"mykey\" \"testing\" false") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("importprivkey", "\"mykey\", \"testing\", false")
        );
	if(identiCode!=""){
		EnsureWalletIsUnlocked();

		string strSecret = params[0].get_str();
		string strLabel = "";
		if (params.size() > 1)
			strLabel = params[1].get_str();

		// Whether to perform rescan after import
		bool fRescan = true;
		if (params.size() > 2)
			fRescan = params[2].get_bool();

		CEvolveChainSecret vchSecret;
		bool fGood = vchSecret.SetString(strSecret);

		if (!fGood) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

		CKey key = vchSecret.GetKey();
		if (!key.IsValid()) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");

		CPubKey pubkey = key.GetPubKey();
		CKeyID vchAddress = pubkey.GetID();
		{
			LOCK2(cs_main, pwalletMain->cs_wallet);

			pwalletMain->MarkDirty();
			pwalletMain->SetAddressBook(vchAddress, strLabel, "receive");
			
			// Don't throw error in case a key is already there
			if (pwalletMain->HaveKey(vchAddress))
				return Value::null;

			pwalletMain->mapKeyMetadata[vchAddress].nCreateTime = 1;

			if (!pwalletMain->AddKeyPubKey(key, pubkey))
				throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");

			// whenever a key is imported, we need to scan the whole chain
			pwalletMain->nTimeFirstKey = 1; // 0 would be considered 'no value'
			



		// Record in identification.log
		////////////////////////////////
		std::string strEvolveChainAddress = CEvolveChainAddress(pubkey.GetID()).ToString();
		std::stringstream EvolveChainJSON;
		std::string strerr;
		bool fListening = false;
		
		std::string parameters = "/verify/addaddress/" + identiCode + "/" + strEvolveChainAddress;
		try
		{
			EvolveChainJSON << dump_get_http("evolvechain.org", parameters);
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
				std::string Method = "importprivkey "  + strLabel;
				dump_writeIdentificationLog("Method:",Method);
				dump_writeIdentificationLog("**Success","EvolveChain address: "+ strEvolveChainAddress + " is confirmed, you can receive coins on this address.");
				dump_writeIdentificationLog("@@EvolveChain address:",strEvolveChainAddress);
				dump_writeIdentificationLog("##CODE##",strSecretCode);
			}else{
				dump_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
			}
		}

		////////////////////////////////

			
			if (fRescan) {
				pwalletMain->ScanForWalletTransactions(chainActive.Genesis(), true);
			}
		}

		return Value::null;
	}else{
		return "importprivkey Error: You are not identified. Get yourself identified to import a private key.\nYou can use Settings->Options->Identification Tab->Start Verification\n\nYou can also use RPC command startverification\n";
	}	
}

Value importwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "importwallet \"filename\"\n"
            "\nImports keys from a wallet dump file (see dumpwallet).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file\n"
            "\nExamples:\n"
            "\nDump the wallet\n"
            + HelpExampleCli("dumpwallet", "\"test\"") +
            "\nImport the wallet\n"
            + HelpExampleCli("importwallet", "\"test\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("importwallet", "\"test\"")
        );
	if(identiCode!=""){
		EnsureWalletIsUnlocked();

		ifstream file;
		file.open(params[0].get_str().c_str(), std::ios::in | std::ios::ate);
		if (!file.is_open())
			throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

		int64_t nTimeBegin = chainActive.Tip()->nTime;

		bool fGood = true;

		int64_t nFilesize = std::max((int64_t)1, (int64_t)file.tellg());
		file.seekg(0, file.beg);

		pwalletMain->ShowProgress(_("Importing..."), 0); // show progress dialog in GUI
		while (file.good()) {
			pwalletMain->ShowProgress("", std::max(1, std::min(99, (int)(((double)file.tellg() / (double)nFilesize) * 100))));
			std::string line;
			std::getline(file, line);
			if (line.empty() || line[0] == '#')
				continue;

			std::vector<std::string> vstr;
			boost::split(vstr, line, boost::is_any_of(" "));
			if (vstr.size() < 2)
				continue;
			CEvolveChainSecret vchSecret;
			if (!vchSecret.SetString(vstr[0]))
				continue;
			CKey key = vchSecret.GetKey();
			CPubKey pubkey = key.GetPubKey();
			CKeyID keyid = pubkey.GetID();
			if (pwalletMain->HaveKey(keyid)) {
				LogPrintf("Skipping import of %s (key already present)\n", CEvolveChainAddress(keyid).ToString());
				continue;
			}
			int64_t nTime = DecodeDumpTime(vstr[1]);
			std::string strLabel;
			bool fLabel = true;
			for (unsigned int nStr = 2; nStr < vstr.size(); nStr++) {
				if (boost::algorithm::starts_with(vstr[nStr], "#"))
					break;
				if (vstr[nStr] == "change=1")
					fLabel = false;
				if (vstr[nStr] == "reserve=1")
					fLabel = false;
				if (boost::algorithm::starts_with(vstr[nStr], "label=")) {
					strLabel = DecodeDumpString(vstr[nStr].substr(6));
					fLabel = true;
				}
			}
			LogPrintf("Importing %s...\n", CEvolveChainAddress(keyid).ToString());
			
			
			// Record in identification.log
			////////////////////////////////
			std::string strEvolveChainAddress = CEvolveChainAddress(keyid).ToString();
			std::stringstream EvolveChainJSON;
			std::string strerr;
			bool fListening = false;
			
			std::string parameters = "/verify/addaddress/" + identiCode + "/" + strEvolveChainAddress;
			try
			{
				EvolveChainJSON << dump_get_http("evolvechain.org", parameters);
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
					std::string Method = "importwallet "  + strLabel;
					dump_writeIdentificationLog("Method:",Method);
					dump_writeIdentificationLog("**Success","EvolveChain address: "+ strEvolveChainAddress + " is confirmed, you can receive coins on this address.");
					dump_writeIdentificationLog("@@EvolveChain address:",strEvolveChainAddress);
					dump_writeIdentificationLog("##CODE##",strSecretCode);
				}else{
					dump_writeIdentificationLog("----Error","Unable to confirm your identification a this time, try again!");
				}
			}

			////////////////////////////////
			
			
			
			
			if (!pwalletMain->AddKeyPubKey(key, pubkey)) {
				fGood = false;
				continue;
			}
			pwalletMain->mapKeyMetadata[keyid].nCreateTime = nTime;
			if (fLabel)
				pwalletMain->SetAddressBook(keyid, strLabel, "receive");
			nTimeBegin = std::min(nTimeBegin, nTime);
		}
		file.close();
		pwalletMain->ShowProgress("", 100); // hide progress dialog in GUI

		CBlockIndex *pindex = chainActive.Tip();
		while (pindex && pindex->pprev && pindex->nTime > nTimeBegin - 7200)
			pindex = pindex->pprev;

		if (!pwalletMain->nTimeFirstKey || nTimeBegin < pwalletMain->nTimeFirstKey)
			pwalletMain->nTimeFirstKey = nTimeBegin;

		LogPrintf("Rescanning last %i blocks\n", chainActive.Height() - pindex->nHeight + 1);
		pwalletMain->ScanForWalletTransactions(pindex);
		pwalletMain->MarkDirty();

		if (!fGood)
			throw JSONRPCError(RPC_WALLET_ERROR, "Error adding some keys to wallet");

		return Value::null;
	}else{
		return "importwallet Error: You are not identified. Get yourself identified to get a new raw change address.\nYou can use Settings->Options->Identification Tab->Start Verification\n\nYou can also use RPC command startverification\n";
	}	
}

Value dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey \"evolvechainaddress\"\n"
            "\nReveals the private key corresponding to 'evolvechainaddress'.\n"
            "Then the importprivkey can be used with this output\n"
            "\nArguments:\n"
            "1. \"evolvechainaddress\"   (string, required) The evolvechain address for the private key\n"
            "\nResult:\n"
            "\"key\"                (string) The private key\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"")
            + HelpExampleCli("importprivkey", "\"mykey\"")
            + HelpExampleRpc("dumpprivkey", "\"myaddress\"")
        );
	
    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    CEvolveChainAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid EvolveChain address");
    CKeyID keyID;
    if (!address.GetKeyID(keyID))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to a key");
    CKey vchSecret;
    if (!pwalletMain->GetKey(keyID, vchSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");
    return CEvolveChainSecret(vchSecret).ToString();
}


Value dumpwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpwallet \"filename\"\n"
            "\nDumps all wallet keys in a human-readable format.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpwallet", "\"test\"")
            + HelpExampleRpc("dumpwallet", "\"test\"")
        );

    EnsureWalletIsUnlocked();

    ofstream file;
    file.open(params[0].get_str().c_str());
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    std::map<CKeyID, int64_t> mapKeyBirth;
    std::set<CKeyID> setKeyPool;
    pwalletMain->GetKeyBirthTimes(mapKeyBirth);
    pwalletMain->GetAllReserveKeys(setKeyPool);

    // sort time/key pairs
    std::vector<std::pair<int64_t, CKeyID> > vKeyBirth;
    for (std::map<CKeyID, int64_t>::const_iterator it = mapKeyBirth.begin(); it != mapKeyBirth.end(); it++) {
        vKeyBirth.push_back(std::make_pair(it->second, it->first));
    }
    mapKeyBirth.clear();
    std::sort(vKeyBirth.begin(), vKeyBirth.end());

    // produce output
    file << strprintf("# Wallet dump created by EvolveChain %s (%s)\n", CLIENT_BUILD, CLIENT_DATE);
    file << strprintf("# * Created on %s\n", EncodeDumpTime(GetTime()));
    file << strprintf("# * Best block at time of backup was %i (%s),\n", chainActive.Height(), chainActive.Tip()->GetBlockHash().ToString());
    file << strprintf("#   mined on %s\n", EncodeDumpTime(chainActive.Tip()->nTime));
    file << "\n";
    for (std::vector<std::pair<int64_t, CKeyID> >::const_iterator it = vKeyBirth.begin(); it != vKeyBirth.end(); it++) {
        const CKeyID &keyid = it->second;
        std::string strTime = EncodeDumpTime(it->first);
        std::string strAddr = CEvolveChainAddress(keyid).ToString();
        CKey key;
        if (pwalletMain->GetKey(keyid, key)) {
            if (pwalletMain->mapAddressBook.count(keyid)) {
                file << strprintf("%s %s label=%s # addr=%s\n", CEvolveChainSecret(key).ToString(), strTime, EncodeDumpString(pwalletMain->mapAddressBook[keyid].name), strAddr);
            } else if (setKeyPool.count(keyid)) {
                file << strprintf("%s %s reserve=1 # addr=%s\n", CEvolveChainSecret(key).ToString(), strTime, strAddr);
            } else {
                file << strprintf("%s %s change=1 # addr=%s\n", CEvolveChainSecret(key).ToString(), strTime, strAddr);
            }
        }
    }
    file << "\n";
    file << "# End of dump\n";
    file.close();
    return Value::null;
}

string dump_get_http(const string &server,const string &path)
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

void dump_writeIdentificationLog(std::string MessageStr,std::string Str)
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