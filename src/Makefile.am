include Makefile.include

AM_CPPFLAGS += -I$(builddir)

noinst_LIBRARIES = \
  libevolvechain_server.a \
  libevolvechain_common.a \
  libevolvechain_cli.a
if ENABLE_WALLET
noinst_LIBRARIES += libevolvechain_wallet.a
endif

bin_PROGRAMS =

if BUILD_EVOLVECHAIND
  bin_PROGRAMS += evolvechaind
endif

if BUILD_EVOLVECHAIN_CLI
  bin_PROGRAMS += evolvechain-cli
endif

SUBDIRS = . $(BUILD_QT) $(BUILD_TEST)
DIST_SUBDIRS = . qt test
.PHONY: FORCE
# evolvechain core #
EVOLVECHAIN_CORE_H = \
  addrman.h \
  alert.h \
  allocators.h \
  base58.h bignum.h \
  bloom.h \
  chainparams.h \
  checkpoints.h \
  checkqueue.h \
  clientversion.h \
  coincontrol.h \
  coins.h \
  compat.h \
  core.h \
  crypter.h \
  db.h \
  hash.h \
  init.h \
  key.h \
  keystore.h \
  leveldbwrapper.h \
  limitedmap.h \
  main.h \
  miner.h \
  mruset.h \
  netbase.h \
  net.h \
  noui.h \
  protocol.h \
  rpcclient.h \
  rpcprotocol.h \
  rpcserver.h \
  script.h \
  serialize.h \
  sync.h \
  threadsafety.h \
  tinyformat.h \
  txdb.h \
  txmempool.h \
  ui_interface.h \
  uint256.h \
  util.h \
  version.h \
  walletdb.h \
  wallet.h

JSON_H = \
  json/json_spirit.h \
  json/json_spirit_error_position.h \
  json/json_spirit_reader.h \
  json/json_spirit_reader_template.h \
  json/json_spirit_stream_reader.h \
  json/json_spirit_utils.h \
  json/json_spirit_value.h \
  json/json_spirit_writer.h \
  json/json_spirit_writer_template.h

obj/build.h: FORCE
	@$(MKDIR_P) $(abs_top_builddir)/src/obj
	@$(top_srcdir)/share/genbuild.sh $(abs_top_builddir)/src/obj/build.h \
	  $(abs_top_srcdir)
version.o: obj/build.h

libevolvechain_server_a_SOURCES = \
  addrman.cpp \
  alert.cpp \
  bloom.cpp \
  checkpoints.cpp \
  coins.cpp \
  init.cpp \
  keystore.cpp \
  leveldbwrapper.cpp \
  main.cpp \
  miner.cpp \
  net.cpp \
  noui.cpp \
  rpcblockchain.cpp \
  rpcmining.cpp \
  rpcmisc.cpp \
  rpcnet.cpp \
  rpcrawtransaction.cpp \
  rpcserver.cpp \
  txdb.cpp \
  txmempool.cpp \
  $(JSON_H) \
  $(EVOLVECHAIN_CORE_H)

libevolvechain_wallet_a_SOURCES = \
  db.cpp \
  crypter.cpp \
  rpcdump.cpp \
  rpcwallet.cpp \
  wallet.cpp \
  walletdb.cpp \
  $(EVOLVECHAIN_CORE_H)

libevolvechain_common_a_SOURCES = \
  base58.cpp \
  allocators.cpp \
  chainparams.cpp \
  core.cpp \
  hash.cpp \
  key.cpp \
  netbase.cpp \
  protocol.cpp \
  rpcprotocol.cpp \
  script.cpp \
  sync.cpp \
  util.cpp \
  version.cpp \
  $(EVOLVECHAIN_CORE_H)

if GLIBC_BACK_COMPAT
libevolvechain_common_a_SOURCES += compat/glibc_compat.cpp
libevolvechain_common_a_SOURCES += compat/glibcxx_compat.cpp
endif

libevolvechain_cli_a_SOURCES = \
  rpcclient.cpp \
  $(EVOLVECHAIN_CORE_H)

nodist_libevolvechain_common_a_SOURCES = $(top_srcdir)/src/obj/build.h
#

# evolvechaind binary #
evolvechaind_LDADD = \
  libevolvechain_server.a \
  libevolvechain_cli.a \
  libevolvechain_common.a \
  $(LIBLEVELDB) \
  $(LIBMEMENV)
if ENABLE_WALLET
evolvechaind_LDADD += libevolvechain_wallet.a
endif
evolvechaind_SOURCES = evolvechaind.cpp
#

if TARGET_WINDOWS
evolvechaind_SOURCES += evolvechaind-res.rc
endif

AM_CPPFLAGS += $(BDB_CPPFLAGS)
evolvechaind_LDADD += $(BOOST_LIBS) $(BDB_LIBS)

# evolvechain-cli binary #
evolvechain_cli_LDADD = \
  libevolvechain_cli.a \
  libevolvechain_common.a \
  $(BOOST_LIBS)
evolvechain_cli_SOURCES = evolvechain-cli.cpp
#

if TARGET_WINDOWS
evolvechain_cli_SOURCES += evolvechain-cli-res.rc
endif

# NOTE: This dependency is not strictly necessary, but without it make may try to build both in parallel, which breaks the LevelDB build system in a race
leveldb/libleveldb.a: leveldb/libmemenv.a

leveldb/%.a:
	@echo "Building LevelDB ..." && $(MAKE) -C $(@D) $(@F) CXX="$(CXX)" \
	  CC="$(CC)" PLATFORM=$(TARGET_OS) AR="$(AR)" $(LEVELDB_TARGET_FLAGS) \
	  OPT="$(CXXFLAGS) $(CPPFLAGS)"

qt/evolvechainstrings.cpp: $(libevolvechain_server_a_SOURCES) $(libevolvechain_common_a_SOURCES) $(libevolvechain_cli_a_SOURCES)
	@test -n $(XGETTEXT) || echo "xgettext is required for updating translations"
	@cd $(top_srcdir); XGETTEXT=$(XGETTEXT) share/qt/extract_strings_qt.py

CLEANFILES = leveldb/libleveldb.a leveldb/libmemenv.a *.gcda *.gcno

DISTCLEANFILES = obj/build.h

EXTRA_DIST = leveldb Makefile.include

clean-local:
	-$(MAKE) -C leveldb clean
	rm -f leveldb/*/*.gcno leveldb/helpers/memenv/*.gcno
