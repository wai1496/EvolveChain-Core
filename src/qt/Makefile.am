include $(top_srcdir)/src/Makefile.include

AM_CPPFLAGS += -I$(top_srcdir)/src \
  -I$(top_builddir)/src/qt \
  -I$(top_builddir)/src/qt/forms \
  $(PROTOBUF_CFLAGS) \
  $(QR_CFLAGS)
bin_PROGRAMS = evolvechain-qt
noinst_LIBRARIES = libevolvechainqt.a
SUBDIRS = . $(BUILD_TEST_QT)
DIST_SUBDIRS = . test

# evolvechain qt core #
QT_TS = \
  locale/evolvechain_ach.ts \
  locale/evolvechain_af_ZA.ts \
  locale/evolvechain_ar.ts \
  locale/evolvechain_be_BY.ts \
  locale/evolvechain_bg.ts \
  locale/evolvechain_bs.ts \
  locale/evolvechain_ca_ES.ts \
  locale/evolvechain_ca.ts \
  locale/evolvechain_ca@valencia.ts \
  locale/evolvechain_cmn.ts \
  locale/evolvechain_cs.ts \
  locale/evolvechain_cy.ts \
  locale/evolvechain_da.ts \
  locale/evolvechain_de.ts \
  locale/evolvechain_el_GR.ts \
  locale/evolvechain_en.ts \
  locale/evolvechain_eo.ts \
  locale/evolvechain_es_CL.ts \
  locale/evolvechain_es_DO.ts \
  locale/evolvechain_es_MX.ts \
  locale/evolvechain_es.ts \
  locale/evolvechain_es_UY.ts \
  locale/evolvechain_et.ts \
  locale/evolvechain_eu_ES.ts \
  locale/evolvechain_fa_IR.ts \
  locale/evolvechain_fa.ts \
  locale/evolvechain_fi.ts \
  locale/evolvechain_fr_CA.ts \
  locale/evolvechain_fr.ts \
  locale/evolvechain_gl.ts \
  locale/evolvechain_gu_IN.ts \
  locale/evolvechain_he.ts \
  locale/evolvechain_hi_IN.ts \
  locale/evolvechain_hr.ts \
  locale/evolvechain_hu.ts \
  locale/evolvechain_id_ID.ts \
  locale/evolvechain_it.ts \
  locale/evolvechain_ja.ts \
  locale/evolvechain_ka.ts \
  locale/evolvechain_kk_KZ.ts \
  locale/evolvechain_ko_KR.ts \
  locale/evolvechain_ky.ts \
  locale/evolvechain_la.ts \
  locale/evolvechain_lt.ts \
  locale/evolvechain_lv_LV.ts \
  locale/evolvechain_mn.ts \
  locale/evolvechain_ms_MY.ts \
  locale/evolvechain_nb.ts \
  locale/evolvechain_nl.ts \
  locale/evolvechain_pam.ts \
  locale/evolvechain_pl.ts \
  locale/evolvechain_pt_BR.ts \
  locale/evolvechain_pt_PT.ts \
  locale/evolvechain_ro_RO.ts \
  locale/evolvechain_ru.ts \
  locale/evolvechain_sah.ts \
  locale/evolvechain_sk.ts \
  locale/evolvechain_sl_SI.ts \
  locale/evolvechain_sq.ts \
  locale/evolvechain_sr.ts \
  locale/evolvechain_sv.ts \
  locale/evolvechain_th_TH.ts \
  locale/evolvechain_tr.ts \
  locale/evolvechain_uk.ts \
  locale/evolvechain_ur_PK.ts \
  locale/evolvechain_uz@Cyrl.ts \
  locale/evolvechain_vi.ts \
  locale/evolvechain_vi_VN.ts \
  locale/evolvechain_zh_CN.ts \
  locale/evolvechain_zh_HK.ts \
  locale/evolvechain_zh_TW.ts

QT_FORMS_UI = \
  forms/aboutdialog.ui \
  forms/addressbookpage.ui \
  forms/askpassphrasedialog.ui \
  forms/coincontroldialog.ui \
  forms/editaddressdialog.ui \
  forms/helpmessagedialog.ui \
  forms/intro.ui \
  forms/openuridialog.ui \
  forms/optionsdialog.ui \
  forms/overviewpage.ui \
  forms/receivecoinsdialog.ui \
  forms/receiverequestdialog.ui \
  forms/rpcconsole.ui \
  forms/sendcoinsdialog.ui \
  forms/sendcoinsentry.ui \
  forms/signverifymessagedialog.ui \
  forms/transactiondescdialog.ui

QT_MOC_CPP = \
  moc_addressbookpage.cpp \
  moc_addresstablemodel.cpp \
  moc_askpassphrasedialog.cpp \
  moc_evolvechainaddressvalidator.cpp \
  moc_evolvechainamountfield.cpp \
  moc_evolvechaingui.cpp \
  moc_evolvechainunits.cpp \
  moc_clientmodel.cpp \
  moc_coincontroldialog.cpp \
  moc_coincontroltreewidget.cpp \
  moc_csvmodelwriter.cpp \
  moc_editaddressdialog.cpp \
  moc_guiutil.cpp \
  moc_intro.cpp \
  moc_macdockiconhandler.cpp \
  moc_macnotificationhandler.cpp \
  moc_monitoreddatamapper.cpp \
  moc_notificator.cpp \
  moc_openuridialog.cpp \
  moc_optionsdialog.cpp \
  moc_optionsmodel.cpp \
  moc_overviewpage.cpp \
  moc_paymentserver.cpp \
  moc_qvalidatedlineedit.cpp \
  moc_qvaluecombobox.cpp \
  moc_receivecoinsdialog.cpp \
  moc_receiverequestdialog.cpp \
  moc_recentrequeststablemodel.cpp \
  moc_rpcconsole.cpp \
  moc_sendcoinsdialog.cpp \
  moc_sendcoinsentry.cpp \
  moc_signverifymessagedialog.cpp \
  moc_splashscreen.cpp \
  moc_trafficgraphwidget.cpp \
  moc_transactiondesc.cpp \
  moc_transactiondescdialog.cpp \
  moc_transactionfilterproxy.cpp \
  moc_transactiontablemodel.cpp \
  moc_transactionview.cpp \
  moc_utilitydialog.cpp \
  moc_walletframe.cpp \
  moc_walletmodel.cpp \
  moc_walletview.cpp

EVOLVECHAIN_MM = \
  macdockiconhandler.mm \
  macnotificationhandler.mm

QT_MOC = \
  evolvechain.moc \
  intro.moc \
  overviewpage.moc \
  rpcconsole.moc

QT_QRC_CPP = qrc_evolvechain.cpp
QT_QRC = evolvechain.qrc

PROTOBUF_CC = paymentrequest.pb.cc
PROTOBUF_H = paymentrequest.pb.h
PROTOBUF_PROTO = paymentrequest.proto

EVOLVECHAIN_QT_H = \
  addressbookpage.h \
  addresstablemodel.h \
  askpassphrasedialog.h \
  evolvechainaddressvalidator.h \
  evolvechainamountfield.h \
  evolvechaingui.h \
  evolvechainunits.h \
  clientmodel.h \
  coincontroldialog.h \
  coincontroltreewidget.h \
  csvmodelwriter.h \
  editaddressdialog.h \
  guiconstants.h \
  guiutil.h \
  intro.h \
  macdockiconhandler.h \
  macnotificationhandler.h \
  monitoreddatamapper.h \
  notificator.h \
  openuridialog.h \
  optionsdialog.h \
  optionsmodel.h \
  overviewpage.h \
  paymentrequestplus.h \
  paymentserver.h \
  qvalidatedlineedit.h \
  qvaluecombobox.h \
  receivecoinsdialog.h \
  receiverequestdialog.h \
  recentrequeststablemodel.h \
  rpcconsole.h \
  sendcoinsdialog.h \
  sendcoinsentry.h \
  signverifymessagedialog.h \
  splashscreen.h \
  trafficgraphwidget.h \
  transactiondesc.h \
  transactiondescdialog.h \
  transactionfilterproxy.h \
  transactionrecord.h \
  transactiontablemodel.h \
  transactionview.h \
  utilitydialog.h \
  walletframe.h \
  walletmodel.h \
  walletmodeltransaction.h \
  walletview.h \
  winshutdownmonitor.h

RES_ICONS = \
  res/icons/add.png \
  res/icons/address-book.png \
  res/icons/evolvechain.ico \
  res/icons/evolvechain.png \
  res/icons/evolvechain_testnet.ico \
  res/icons/evolvechain_testnet.png \
  res/icons/clock1.png \
  res/icons/clock2.png \
  res/icons/clock3.png \
  res/icons/clock4.png \
  res/icons/clock5.png \
  res/icons/configure.png \
  res/icons/connect0_16.png \
  res/icons/connect1_16.png \
  res/icons/connect2_16.png \
  res/icons/connect3_16.png \
  res/icons/connect4_16.png \
  res/icons/debugwindow.png \
  res/icons/edit.png \
  res/icons/editcopy.png \
  res/icons/editpaste.png \
  res/icons/export.png \
  res/icons/filesave.png \
  res/icons/history.png \
  res/icons/key.png \
  res/icons/lock_closed.png \
  res/icons/lock_open.png \
  res/icons/overview.png \
  res/icons/qrcode.png \
  res/icons/quit.png \
  res/icons/receive.png \
  res/icons/remove.png \
  res/icons/send.png \
  res/icons/synced.png \
  res/icons/toolbar.png \
  res/icons/toolbar_testnet.png \
  res/icons/transaction0.png \
  res/icons/transaction2.png \
  res/icons/transaction_conflicted.png \
  res/icons/tx_inout.png \
  res/icons/tx_input.png \
  res/icons/tx_output.png \
  res/icons/tx_mined.png

EVOLVECHAIN_QT_CPP = \
  evolvechain.cpp \
  evolvechainaddressvalidator.cpp \
  evolvechainamountfield.cpp \
  evolvechaingui.cpp \
  evolvechainunits.cpp \
  clientmodel.cpp \
  csvmodelwriter.cpp \
  guiutil.cpp \
  intro.cpp \
  monitoreddatamapper.cpp \
  notificator.cpp \
  optionsdialog.cpp \
  optionsmodel.cpp \
  qvalidatedlineedit.cpp \
  qvaluecombobox.cpp \
  rpcconsole.cpp \
  splashscreen.cpp \
  trafficgraphwidget.cpp \
  utilitydialog.cpp \
  winshutdownmonitor.cpp

if ENABLE_WALLET
EVOLVECHAIN_QT_CPP += \
  addressbookpage.cpp \
  addresstablemodel.cpp \
  askpassphrasedialog.cpp \
  coincontroldialog.cpp \
  coincontroltreewidget.cpp \
  editaddressdialog.cpp \
  openuridialog.cpp \
  overviewpage.cpp \
  paymentrequestplus.cpp \
  paymentserver.cpp \
  receivecoinsdialog.cpp \
  receiverequestdialog.cpp \
  recentrequeststablemodel.cpp \
  sendcoinsdialog.cpp \
  sendcoinsentry.cpp \
  signverifymessagedialog.cpp \
  transactiondesc.cpp \
  transactiondescdialog.cpp \
  transactionfilterproxy.cpp \
  transactionrecord.cpp \
  transactiontablemodel.cpp \
  transactionview.cpp \
  walletframe.cpp \
  walletmodel.cpp \
  walletmodeltransaction.cpp \
  walletview.cpp
endif

RES_IMAGES = \
  res/images/about.png \
  res/images/splash.png \
  res/images/splash_testnet.png

RES_MOVIES = $(wildcard res/movies/spinner-*.png)

EVOLVECHAIN_RC = res/evolvechain-qt-res.rc

libevolvechainqt_a_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms $(QT_DBUS_INCLUDES)
libevolvechainqt_a_SOURCES = $(EVOLVECHAIN_QT_CPP) $(EVOLVECHAIN_QT_H) $(QT_FORMS_UI) \
  $(QT_QRC) $(QT_TS) $(PROTOBUF_PROTO) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES)

nodist_libevolvechainqt_a_SOURCES = $(QT_MOC_CPP) $(QT_MOC) $(PROTOBUF_CC) \
  $(PROTOBUF_H) $(QT_QRC_CPP)

BUILT_SOURCES = $(nodist_libevolvechainqt_a_SOURCES)

#Generating these with a half-written protobuf header leads to wacky results.
#This makes sure it's done.
$(QT_MOC): $(PROTOBUF_H)
$(QT_MOC_CPP): $(PROTOBUF_H)

# evolvechain-qt binary #
evolvechain_qt_CPPFLAGS = $(AM_CPPFLAGS) $(QT_INCLUDES) \
  -I$(top_srcdir)/src/qt/forms
evolvechain_qt_SOURCES = evolvechain.cpp
if TARGET_DARWIN
  evolvechain_qt_SOURCES += $(EVOLVECHAIN_MM)
endif
if TARGET_WINDOWS
  evolvechain_qt_SOURCES += $(EVOLVECHAIN_RC)
endif
evolvechain_qt_LDADD = libevolvechainqt.a $(LIBEVOLVECHAIN_SERVER)
if ENABLE_WALLET
evolvechain_qt_LDADD += $(LIBEVOLVECHAIN_WALLET)
endif
evolvechain_qt_LDADD += $(LIBEVOLVECHAIN_CLI) $(LIBEVOLVECHAIN_COMMON) $(LIBLEVELDB) $(LIBMEMENV) \
  $(BOOST_LIBS) $(QT_LIBS) $(QT_DBUS_LIBS) $(QR_LIBS) $(PROTOBUF_LIBS) $(BDB_LIBS)
evolvechain_qt_LDFLAGS = $(QT_LDFLAGS)

# forms/foo.h -> forms/ui_foo.h
QT_FORMS_H=$(join $(dir $(QT_FORMS_UI)),$(addprefix ui_, $(notdir $(QT_FORMS_UI:.ui=.h))))

#locale/foo.ts -> locale/foo.qm
QT_QM=$(QT_TS:.ts=.qm)

.PHONY: FORCE
.SECONDARY: $(QT_QM)

evolvechainstrings.cpp: FORCE
	$(MAKE) -C $(top_srcdir)/src qt/evolvechainstrings.cpp

translate: evolvechainstrings.cpp $(QT_FORMS_UI) $(QT_FORMS_UI) $(EVOLVECHAIN_QT_CPP) $(EVOLVECHAIN_QT_H) $(EVOLVECHAIN_MM)
	@test -n $(LUPDATE) || echo "lupdate is required for updating translations"
	@QT_SELECT=$(QT_SELECT) $(LUPDATE) $^ -locations relative -no-obsolete -ts locale/evolvechain_en.ts

$(QT_QRC_CPP): $(QT_QRC) $(QT_QM) $(QT_FORMS_H) $(RES_ICONS) $(RES_IMAGES) $(RES_MOVIES) $(PROTOBUF_H)
	@cd $(abs_srcdir); test -f $(RCC) && QT_SELECT=$(QT_SELECT) $(RCC) -name evolvechain -o $(abs_builddir)/$@ $< || \
	  echo error: could not build $@
	$(SED) -e '/^\*\*.*Created:/d' $@ > $@.n && mv $@{.n,}
	$(SED) -e '/^\*\*.*by:/d' $@  > $@.n && mv $@{.n,}

CLEANFILES = $(BUILT_SOURCES) $(QT_QM) $(QT_FORMS_H) *.gcda *.gcno
