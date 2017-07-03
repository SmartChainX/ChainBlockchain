#include <thread>
#include <fstream>
#include <iostream>
#include <signal.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/trim_all.hpp>
#include <boost/filesystem.hpp>

#include <libdevcore/FileSystem.h>
#include <libethashseal/EashAux.h>
#include <libevm/VM.h>
#include <libevm/VMFactory.h>
#include <libethcore/KeyManager.h>
#include <libethcore/ICAP.h>
#include <libethereum/Defaults.h>
#include <libethereum/BlockChainSync.h>
#include <libethashseal/ashClient.h>
#include <libethashseal/GenesisInfo.h>
#include <libwebthree/WebThree.h>

#include <libdevcrypto/LibSnark.h>

#include <libweb3jsonrpc/AccountHolder.h>
#include <libweb3jsonrpc/chainx.h>
#include <libweb3jsonrpc/SafeHttpServer.h>
#include <libweb3jsonrpc/ModularServer.h>
#include <libweb3jsonrpc/IpcServer.h>
#include <libweb3jsonrpc/LevelDB.h>
#include <libweb3jsonrpc/Whisper.h>
#include <libweb3jsonrpc/Net.h>
#include <libweb3jsonrpc/Web3.h>
#include <libweb3jsonrpc/AdminNet.h>
#include <libweb3jsonrpc/Adminchainx.h>
#include <libweb3jsonrpc/AdminUtils.h>
#include <libweb3jsonrpc/Personal.h>
#include <libweb3jsonrpc/Debug.h>
#include <libweb3jsonrpc/Test.h>

#include "MinerAux.h"
#include "BuildInfo.h"
#include "AccountManager.h"

using namespace std;
using namespace dev;
using namespace dev::p2p;
using namespace dev::chainx;
using namespace boost::algorithm;

static std::atomic<bool> g_silence = {false};

void help()
{
	cout
		<< "Usage [OPTIONS]" << endl
		<< "Options:" << endl << endl
		<< "Wallet usage:" << endl;
	AccountManager::streamAccountHelp(cout);
	AccountManager::streamWallelp(cout);
	cout
		<< endl;
	cout
		<< "Client mode (default):" << endl
		<< "    --mainnet  Use the main network protocol." << endl
		<< "    --ropsten  Use the Ropsten testnet." << endl
		<< "    --private <name>  Use a private chain." << endl
		<< "    --test  Testing mode: Disable PoW and provide test rpc interface." << endl
		<< "    --config <file>  Configure specialised blockchain using given JSON information." << endl
		<< "    --oppose-dao-fork  Ignore DAO hard fork (default is to participate)." << endl
		<< endl
		<< "    -o,--mode <full/peer>  Start a full node or a peer node (default: full)." << endl
		<< endl
		<< "    -j,--json-rpc  Enable JSON-RPC server (default: off)." << endl
		<< "    --ipc  Enable IPC server (default: on)." << endl
		<< "    --ipcpath Set .ipc socket path (default: data directory)" << endl
		<< "    --admin-via-http  Expose admin interface via http - UNSAFE! (default: off)." << endl
		<< "    --no-ipc  Disable IPC server." << endl
		<< "    --json-rpc-port <n>  Specify JSON-RPC server port (implies '-j', default: " << SensibleHttpPort << ")." << endl
		<< "    --rpccorsdomain <domain>  Domain on which to send Access-Control-Allow-Origin header." << endl
		<< "    --admin <password>  Specify admin session key for JSON-RPC (default: auto-generated and printed at start-up)." << endl
		<< "    -K,--kill  Kill the blockchain first." << endl
		<< "    -R,--rebuild  Rebuild the blockchain from the existing database." << endl
		<< "    --rescue  Attempt to rescue a corrupt database." << endl
		<< endl
		<< "    --import-presale <file>  Import a pre-sale key; you'll need to specify the password to this key." << endl
		<< "    -s,--import-secret <secret>  Import a secret key into the key store." << endl
		<< "    --master <password>  Give the master password for the key store. Use --master \"\" to show a prompt." << endl
		<< "    --password <password>  Give a password for a private key." << endl
		<< endl
		<< "Client transacting:" << endl
		/*<< "    -B,--block-fees <n>  Set the block fee profit in the reference unit, e.g. ¢ (default: 15)." << endl
		<< "    -e,---price <n>  Set the price in the reference unit, e.g. ¢ (default: 30.679)." << endl
		<< "    -P,--priority <0 - 100>  Set the default priority percentage (%) of a transaction (default: 50)." << endl*/
		<< "    --ask <wei>  Set the minimum ask gas price under which no transaction will be mined (default " << toString(DefaultGasPrice) << " )." << endl
		<< "    --bid <wei>  Set the bid gas price to pay for transactions (default " << toString(DefaultGasPrice) << " )." << endl
		<< "    --unsafe-transactions  Allow all transactions to proceed without verification. EXTREMELY UNSAFE."
		<< endl
		<< "Client mining:" << endl
		<< "    -a,--address <addr>  Set the author (mining payout) address to given address (default: auto)." << endl
		<< "    -m,--mining <on/off/number>  Enable mining, optionally for a specified number of blocks (default: off)." << endl
		<< "    -f,--force-mining  Mine even when there are no transactions to mine (default: off)." << endl
		<< "    -C,--cpu  When mining, use the CPU." << endl
		<< "    -t, --mining-threads <n>  Limit number of CPU/GPU miners to n (default: use everything available on selected platform)." << endl
		<< endl
		<< "Client networking:" << endl
		<< "    --client-name <name>  Add a name to your client's version string (default: blank)." << endl
		<< "    --bootstrap  Connect to the default  peer servers (default unless --no-discovery used)." << endl
		<< "    --no-bootstrap  Do not connect to the default  peer servers (default only when --no-discovery is used)." << endl
		<< "    -x,--peers <number>  Attempt to connect to a given number of peers (default: 11)." << endl
		<< "    --peer-stretch <number>  Give the accepted connection multiplier (default: 7)." << endl

		<< "    --public-ip <ip>  Force advertised public IP to the given IP (default: auto)." << endl
		<< "    --listen-ip <ip>(:<port>)  Listen on the given IP for incoming connections (default: 0.0.0.0)." << endl
		<< "    --listen <port>  Listen on the given port for incoming connections (default: 30303)." << endl
		<< "    -r,--remote <host>(:<port>)  Connect to the given remote host (default: none)." << endl
		<< "    --port <port>  Connect to the given remote port (default: 30303)." << endl
		<< "    --network-id <n>  Only connect to other hosts with this network id." << endl
		<< "    --upnp <on/off>  Use UPnP for NAT (default: on)." << endl

		<< "    --peerset <list>  Space delimited list of peers; element format: type:publickey@ipAddress[:port]." << endl
		<< "        Types:" << endl
		<< "        default		Attempt connection when no other peers are available and pinning is disabled." << endl
		<< "        required		Keep connected at all times." << endl
// TODO:
//		<< "	--trust-peers <filename>  Space delimited list of publickeys." << endl

		<< "    --no-discovery  Disable node discovery, implies --no-bootstrap." << endl
		<< "    --pin  Only accept or connect to trusted peers." << endl
		<< "    --hermit  Equivalent to --no-discovery --pin." << endl
		<< "    --sociable  Force discovery and no pinning." << endl
		<< endl;
	MinerCLI::streamHelp(cout);
	cout
		<< "Import/export modes:" << endl
		<< "    --from <n>  Export only from block n; n may be a decimal, a '0x' prefixed hash, or 'latest'." << endl
		<< "    --to <n>  Export only to block n (inclusive); n may be a decimal, a '0x' prefixed hash, or 'latest'." << endl
		<< "    --only <n>  Equivalent to --export-from n --export-to n." << endl
		<< "    --dont-check  Prevent checking some block aspects. Faster importing, but to apply only when the data is known to be valid." << endl
		<< endl
		<< "General Options:" << endl
		<< "    -d,--db-path,--datadir <path>  Load database from path (default: " << getDataDir() << ")." << endl
#if CHAINX_EVMJIT
		<< "    --vm <vm-kind>  Select VM; options are: interpreter, jit or smart (default: interpreter)." << endl
#endif // chainx_EVMJIT
		<< "    -v,--verbosity <0 - 9>  Set the log verbosity from 0 to 9 (default: 8)." << endl
		<< "    -V,--version  Show the version and exit." << endl
		<< "    -h,--help  Show this help message and exit." << endl
		<< endl
		<< "Experimental / Proof of Concept:" << endl
		<< "    --shh  Enable Whisper." << endl
		<< endl
		;
		exit(0);
}

string chainxCredits(bool _interactive = false)
{
	std::ostringstream cout;
	if (_interactive)
		cout
			<< "Type 'exit' to quit" << endl << endl;
	return credits() + cout.str();
}

void version()
{
	cout << "chainx version " << dev::Version << endl;
	cout << "chainx network protocol version: " << dev::chainx::c_protocolVersion << endl;
	cout << "Client database version: " << dev::chainx::c_databaseVersion << endl;
	cout << "Build: " << DEV_QUOTED(chainx_BUILD_PLATFORM) << "/" << DEV_QUOTED(chainx_BUILD_TYPE) << endl;
	exit(0);
}

/*
The equivalent of setlocale(LC_ALL, “C”) is called before any user code is run.
If the user has an invalid environment setting then it is possible for the call
to set locale to fail, so there are only two possible actions, the first is to
throw a runtime exception and cause the program to quit (default behaviour),
or the second is to modify the environment to something sensible (least
surprising behaviour).
The follow code produces the least surprising behaviour. It will use the user
specified default locale if it is valid, and if not then it will modify the
environment the process is running in to use a sensible default. This also means
that users do not need to install language packs for their OS.
*/
void setDefaultOrCLocale()
{
#if __unix__
	if (!std::setlocale(LC_ALL, ""))
	{
		setenv("LC_ALL", "C", 1);
	}
#endif
