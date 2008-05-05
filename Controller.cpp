#include "stdafx.h"
#include "Controller.h"
#include "Helpers.h"
#include "Dialogs.h"
#include "UserList.h"

namespace GG {
	//debug: To tylko tymczasowe.
	void handler (int level, const char * format, ...) {
		va_list p;
		va_start(p, format);
		int size = _vscprintf(format, p);
		char * buff = new char [size + 2];
		buff[size + 1] = 0;
		size = _vsnprintf_s(buff, size + 2, size + 1, format, p);
		buff[size] = 0;
		Singleton<Controller>::getInstance()->getCtrl()->logMsg(Stamina::logWarn, "", "", buff);
		delete [] buff;
		va_end(p);
	}

	Controller::Controller() : gg(0), thread(0) {
		gg_debug_level = GG_DEBUG_FUNCTION;
		gg_debug = &handler;
		IMessageDispatcher& d = getIMessageDispatcher();
		ActionDispatcher& a = getActionDispatcher();
		Config& c = getConfig();

		//statyczne
		d.setStaticValue(IM_PLUG_NET, GG::net);
		d.setStaticValue(IM_PLUG_SIG, (int) GG::sig);
		d.setStaticValue(IM_PLUG_NAME, (int) GG::name);
		d.setStaticValue(IM_PLUG_TYPE, imtMessage | imtProtocol | imtContact | imtConfig | imtUI | imtNet | imtNetSearch | imtMessageAck | imtMsgUI| imtNetUID);
		d.setStaticValue(IM_PLUG_PRIORITY, priorityLow);
		d.setStaticValue(IM_PLUG_NETNAME, (int) "Gadu-Gadu");
		d.setStaticValue(IM_PLUG_NETSHORTNAME, (int) "GG");
		d.setStaticValue(IM_PLUG_UIDNAME, (int) "#");
		d.setStaticValue(IM_MSG_CHARLIMIT, 2000);

		//IMessage
		d.connect(IM_UI_PREPARE, bind(&Controller::onPrepareUI, this, _1));
		d.connect(IM_START, bind(&Controller::onStart, this, _1));
		d.connect(IM_BEFOREEND, bind(&Controller::onBeforeEnd, this, _1));
		d.connect(IM_END, bind(&Controller::onEnd, this, _1));
		d.connect(IM_DISCONNECT, bind(&Controller::onDisconnect, this, _1));
		d.connect(IM_GET_STATUS, bind(&Controller::onGetStatus, this, _1));
		d.connect(IM_GET_STATUSINFO, bind(&Controller::onGetStatusInfo, this, _1));
		d.connect(IM_GET_UID, bind(&Controller::onGetUID, this, _1));
		d.connect(IM_ISCONNECTED, bind(&Controller::onIsConnected, this, _1));
		//todo: Pamiêtaæ, ¿e zamiast IM_MSG_RCV i IM_MSG_SEND jest nowa MQ.
		d.connect(IM_CNT_ADD, bind(&Controller::onCntAdd, this, _1));
		d.connect(IM_CNT_CHANGED, bind(&Controller::onCntChanged, this, _1));
		d.connect(IM_CNT_REMOVE, bind(&Controller::onCntRemove, this, _1));
		d.connect(IM_IGN_CHANGED, bind(&Controller::onIgnChanged, this, _1));
		//d.connect(IM_CNT_DOWNLOAD, bind(&Controller::onCntDownload, this, _1));
		//d.connect(IM_CNT_UPLOAD, bind(&Controller::onCntUpload, this, _1));
		//d.connect(IM_CNT_SEARCH, bind(&Controller::onCntSearch, this, _1));
		d.connect(IM_CHANGESTATUS, bind(&Controller::onChangeStatus, this, _1));

		//API
		//d.connect(api::isEnabled, bind(&Controller::apiEnabled, this, _1));

		//Akcje
		a.connect(CFG::group, bind(&Controller::handleConfig, this, _1));
		a.connect(ACT::setDefaultServers, bind(&Controller::handleSetDefaultServers, this, _1));
		a.connect(ACT::createGGAccount, bind(&Controller::handleCreateGGAccount, this, _1));
		a.connect(ACT::removeGGAccount, bind(&Controller::handleRemoveGGAccount, this, _1));
		a.connect(ACT::changePassword, bind(&Controller::handleChangePassword, this, _1));
		a.connect(ACT::remindPassword, bind(&Controller::handleRemindPassword, this, _1));
		a.connect(ACT::importCntList, bind(&Controller::handleImportCntList, this, _1));
		a.connect(ACT::exportCntList, bind(&Controller::handleExportCntList, this, _1));
		a.connect(ACT::statusDescripton, bind(&Controller::handleStatusDescription, this, _1));
		for (unsigned i = 0; i < serversCount; ++i) {
			a.connect(ACT::statusServer + i, bind(&Controller::handleStatusServer, this, _1));
		}
		a.connect(ACT::statusOnline, bind(&Controller::handleStatusOnline, this, _1));
		a.connect(ACT::statusAway, bind(&Controller::handleStatusAway, this, _1));
		a.connect(ACT::statusInvisible, bind(&Controller::handleStatusInvisible, this, _1));
		a.connect(ACT::statusOffline, bind(&Controller::handleStatusOffline, this, _1));

		//Konfiguracja
		c.setColumn(tableConfig, CFG::login, ctypeString, "", "GG/login");
		c.setColumn(tableConfig, CFG::password, ctypeString | cflagSecret | cflagXor, "", "GG/password");
		c.setColumn(tableConfig, CFG::status, ctypeInt, 0, "GG/status");
		c.setColumn(tableConfig, CFG::startStatus, ctypeInt, 0, "GG/startStatus");
		c.setColumn(tableConfig, CFG::friendsOnly, ctypeInt, 0, "GG/friedsOnly");
		c.setColumn(tableConfig, CFG::description, ctypeString, "", "GG/description");
		c.setColumn(tableConfig, CFG::servers, ctypeString, GG::defaultServers, "GG/servers");
		c.setColumn(tableConfig, CFG::selectedServer, ctypeString, "", "GG/servers");
		c.setColumn(tableConfig, CFG::useSSL, ctypeInt, 0, "GG/useSSL");
		c.setColumn(tableConfig, CFG::resumeDisconnected, ctypeInt, 1, "GG/resumeDisconnected");
	}

	void Controller::onStart(IMEvent &ev) {
		IMLOG("[onStart:]");
	}

	void Controller::onPrepareUI(IMEvent &ev) {
		IMLOG("[onPrepareUI:]");
		//Ikony
		IconRegister(IML_16, ICO::logo, Ctrl->hDll(), IDI_LOGO);
		IconRegister(IML_16, ICO::server, Ctrl->hDll(), IDI_SERVER);
		IconRegister(IML_ICO, ICO::overlay, Ctrl->hDll(), IDI_OVERLAY);
		IconRegister(IML_16, ICO::online, Ctrl->hDll(), IDI_ONLINE);
		IconRegister(IML_16, ICO::away, Ctrl->hDll(), IDI_AWAY);
		IconRegister(IML_16, ICO::invisible, Ctrl->hDll(), IDI_INVISIBLE);
		IconRegister(IML_16, ICO::offline, Ctrl->hDll(), IDI_OFFLINE);
		IconRegister(IML_16, ICO::blocking, Ctrl->hDll(), IDI_BLOCKING);
		IconRegister(IML_16, ICO::connecting, Ctrl->hDll(), IDI_CONNECTING);

		//Konfiguracja
		UIGroupAdd(IMIG_CFG_USER, CFG::group, ACTR_SAVE, "GG", ICO::logo);
		UIActionCfgAddPluginInfoBox2(CFG::group,
			"Wtyczka umo¿liwia komunikacjê przy pomocy najpopularniejszego protoko³u w Polsce."
			, "U¿yto biblioteki <b>libgadu</b> - http://toxygen.net/libgadu/<br/>"
			"Modyfikacje - <b>Micha³ \"Dulek\" Dulko</b><br/>"
			"Copyright © 2003-2008 <b>Stamina</b><br/><br/>"
			"<span class='note'>Skompilowano: <b>" __DATE__ "</b> [<b>" __TIME__ "</b>]</span>"
			, ("reg://IML16/" + inttostr(ICO::logo) + ".ico").c_str(), -3
		);

		UIActionCfgAdd(CFG::group, 0, ACTT_GROUP, "Konto na serwerze GG");
		UIActionCfgAdd(CFG::group, 0, ACTT_COMMENT | ACTSC_INLINE, "Numer GG", 0, 0, 0, 60);
		UIActionCfgAdd(CFG::group, CFG::login, ACTT_EDIT | ACTSC_INT, "", CFG::login);
		UIActionCfgAdd(CFG::group, 0, ACTT_COMMENT | ACTSC_INLINE, "Has³o", 0, 0, 0, 60);
		UIActionCfgAdd(CFG::group, CFG::password, ACTT_PASSWORD, "", CFG::password);

		UIActionAdd(CFG::group, 0, ACTT_SEPARATOR);

		UIActionAdd(CFG::group, ACT::createGGAccount, ACTT_BUTTON | ACTSC_INLINE | ACTSC_BOLD, SetActParam("Za³ó¿ konto", AP_ICO, inttostr(ICON_ACCOUNTCREATE)).c_str(), 0, 155, 30);
		UIActionAdd(CFG::group, ACT::importCntList, ACTT_BUTTON | ACTSC_BOLD | ACTSC_FULLWIDTH, SetActParam("Importuj listê kontaktów", AP_ICO, inttostr(ICON_IMPORT)).c_str(), 0, 180, 30);
		UIActionAdd(CFG::group, ACT::changePassword, ACTT_BUTTON | ACTSC_INLINE, SetActParam("Zmieñ has³o", AP_ICO, inttostr(ICON_CHANGEPASSWORD)).c_str(), 0, 155, 30);
		UIActionAdd(CFG::group, ACT::remindPassword, ACTT_BUTTON | ACTSC_FULLWIDTH, SetActParam("Przypomnij has³o", AP_ICO, inttostr(ICON_REMINDPASSWORD)).c_str(), 0, 180, 30);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");

		UIActionAdd(CFG::group, 0, ACTT_GROUP, "Ustawienia");
		UIActionAdd(CFG::group, 0, ACTT_COMMENT | ACTSC_INLINE, "Status startowy");
		UIActionAdd(CFG::group, IMIB_CFG, ACTT_COMBO | ACTSCOMBO_LIST,
			"Ostatni" CFGICO "#74" CFGVALUE "0\n"
			"Niedostêpny" CFGICO "0x40A00000" CFGVALUE "1\n"
			"Dostêpny" CFGICO "0x40A00400" CFGVALUE "2\n"
			"Zajêty" CFGICO "0x40A00410" CFGVALUE "3\n"
			"Ukryty" CFGICO "0x40A00420" CFGVALUE "4"
			AP_PARAMS AP_TIP "Status, który zostanie ustawiony po uruchomieniu programu", 
			CFG::startStatus
		);
		UIActionAdd(CFG::group, CFG::useSSL, ACTT_CHECK, "U¿ywaj bezpiecznego po³¹czenia (SSL)", CFG::useSSL);
		UIActionAdd(CFG::group, CFG::friendsOnly, ACTT_CHECK, "Mój status widoczny tylko u znajomych z listy kontaktów", CFG::friendsOnly);
		UIActionAdd(CFG::group, CFG::resumeDisconnected, ACTT_CHECK, "£¹cz ponownie, je¿eli serwer zakoñczy po³¹czenie." AP_TIP "Wy³¹cz t¹ opcjê, je¿eli czêsto korzystasz z konta w ró¿nych miejscach. Zapobiega cyklicznemu \"prze³¹czaniu\" pomiêdzy w³¹czonymi programami.", CFG::resumeDisconnected);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND);

		UIActionAdd(CFG::group, 0, ACTT_GROUP, "Serwery");
		UIActionAdd(CFG::group, CFG::servers, ACTT_TEXT | ACTSC_INLINE, "" CFGTIP "Je¿eli zostawisz to pole puste - zostanie u¿yty serwer wskazany przez hub GG.", CFG::servers, 150);
		UIActionAdd(CFG::group, 0, ACTT_TIPBUTTON | ACTSC_INLINE, AP_TIPRICH "W ka¿dej linijce jeden serwer. Pusta linijka oznacza HUB (system zwracaj¹cy najmniej obci¹¿ony serwer)."
			"<br/><b>Format</b> (zawartoœæ [...] jest opcjonalna):"
			"<br/><i>Adres</i>[:<i>port</i>]"
			"<br/>SSL[ <i>Adres</i>[:<i>port</i>]] (po³¹czenie szyfrowane)"
			"<br/><br/>Aby wy³¹czyæ serwer dodaj <b>!</b> na pocz¹tku."
			AP_TIPRICH_WIDTH "300"
		);
		UIActionAdd(CFG::group, ACT::setDefaultServers, ACTT_BUTTON, SetActParam("Domyœlne", AP_ICO, inttostr(ICON_DEFAULT)).c_str(), 0, 0, 25);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");

		UIActionAdd(CFG::group, 0, ACTT_GROUP, "");
		UIActionAdd(CFG::group, 0, ACTT_TIPBUTTON | ACTSC_INLINE, SetActParam(AP_TIPRICH "<b>UWAGA!</b> Konto zostanie <u>bezpowrotnie</u> usuniête z serwera GG! Nie bêdziesz móg³ wiêcej korzystaæ z tego numeru!", AP_ICO, inttostr(ICON_WARNING)).c_str(), 0, 30, 30);
		UIActionAdd(CFG::group, ACT::removeGGAccount, ACTT_BUTTON, SetActParam("Usuñ konto z serwera", AP_ICO, inttostr(ICON_ACCOUNTREMOVE)).c_str(), 0, 170, 30);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");

		//zmiana statusu
		UIGroupAdd(IMIG_STATUS, ACT::status, 0, "GG", ICO::offline);
		UIGroupAdd(ACT::status, ACT::statusServers, 0, "Serwer", ICO::server);
		for (unsigned i = 0; i < serversCount; ++i) {
			UIActionAdd(ACT::statusServers, ACT::statusServer + i, ACTS_HIDDEN, "");
		}
		UIActionAdd(ACT::status, ACT::statusDescripton, 0, "Opis", 0);
		UIActionAdd(ACT::status, ACT::statusOnline, 0, "Dostêpny", ICO::online);
		UIActionAdd(ACT::status, ACT::statusAway, 0, "Zaraz wracam", ICO::away);
		UIActionAdd(ACT::status, ACT::statusInvisible, 0, "Ukryty", ICO::invisible);
		UIActionAdd(ACT::status, ACT::statusOffline, 0, "Niedostêpny", ICO::offline);

		refreshServers(GETSTR(CFG::servers));

		ev.setSuccess();
	}

	void Controller::onBeforeEnd(IMEvent &ev) {
		IMLOG("[onBeforeEnd:]");
	}

	void Controller::onEnd(IMEvent &ev) {
		IMLOG("[onEnd:]");
		if (gg)
			delete gg;
	}

	void Controller::onDisconnect(IMEvent& ev) {
		IMLOG("[onDisconnect:]");
		if (gg)
			gg->setStatus(ST_OFFLINE, 0);
		if (WaitForSingleObject(thread, 1000) != WAIT_OBJECT_0)
			TerminateThread(thread, 0);
		CloseHandle(thread);
	}

	void Controller::onChangeStatus(IMEvent &ev) {
		IMLOG("[onChangeStatus:]");
		if (!gg || (!gg->isConnected() && !gg->isConnecting())) {
			string login = GETSTR(CFG::login);
			string password = getPassword();
			if (gg && (gg->getLogin() != login || gg->getPassword() != password))
				delete gg;
			if (!gg)
				gg = new Session(login, password, &ggEventHandler, GETINT(CFG::friendsOnly));

			gg->setServers(servers);

			thread = threads.runEx(ggWatchThread, new tStatusInfo(ev.getP1(), (char*)ev.getP2()), "ggWatchThread");
		} else {
			gg->setStatus(ev.getP1(), (char*)ev.getP2());
		}
	}

	void Controller::onGetStatus(IMEvent &ev) {
		IMLOG("[onGetStatus:]");
			ev.setReturnValue(gg ? gg->getStatus() : ST_OFFLINE);
	}

	void Controller::onGetStatusInfo(IMEvent &ev) {
		IMLOG("[onGetStatusInfo:]");
			ev.setReturnValue(gg ? gg->getStatusDescription() : "");
	}

	void Controller::onGetUID(IMEvent &ev) {
		IMLOG("[onGetUID:]");
		ev.setReturnValue(GETSTR(CFG::login));
	}
	
	void Controller::onIsConnected(IMEvent& ev) {
		IMLOG("[onIsConnected:]");
		ev.setReturnValue(gg && gg->isConnected());
	}

	void Controller::onCntAdd(IMEvent& ev) {
		IMLOG("[onCntAdd:]");
		if (gg && gg->isConnected()) {
			Contact cnt(ev.getP1());
			if (cnt.getNet() == net && !(cnt.getStatus() & (ST_HIDEMYSTATUS|ST_NOTINLIST))) {
				gg->addContact(cnt);
			}
		}
	}

	void Controller::onCntChanged(IMEvent& ev) {
		IMLOG("[onCntChanged:]");
		if (gg && gg->isConnected()) {
			Contact cnt(ev.getP1());
			sIMessage_CntChanged cntChanged(ev.getIMessage());

			if (cntChanged._changed.net && cntChanged._oldNet == net) {
				gg->removeContact(cntChanged._oldUID, ICMessage(IMC_IGN_FIND, cntChanged._oldNet, (int)(cntChanged._changed.net ? cntChanged._oldUID : cnt.getUidString().a_str())) ? GG_USER_BLOCKED : GG_USER_NORMAL);
				return;
			} else if (cntChanged._changed.net && cntChanged._changed.net == net && !(cnt.getStatus() & (ST_HIDEMYSTATUS|ST_NOTINLIST))) {
				setCntStatus(cnt, ST_OFFLINE);
				gg->addContact(cnt.getUidString().a_string(), getCntType(cnt.getStatus()));
				return;
			}

			if (cntChanged._changed.uid && !(cnt.getStatus() & (ST_HIDEMYSTATUS|ST_NOTINLIST))) {
				setCntStatus(cnt, ST_OFFLINE);
				gg->removeContact(cntChanged._oldUID);
				gg->addContact(cnt);
			}
		}
	}

	void Controller::onCntRemove(IMEvent& ev) {
		IMLOG("[onCntRemove:]");
		if (gg && gg->isConnected()) {
			Contact cnt(ev.getP1());
			if (cnt.getNet() == net && !(cnt.getStatus() & (ST_HIDEMYSTATUS|ST_NOTINLIST))) {
				gg->removeContact(cnt);
			}
		}
	}
	
	void Controller::onIgnChanged(IMEvent& ev) {
		IMLOG("[onIgnChanged:]");
		if (gg && gg->isConnected() && abs(ev.getP1()) == net) {
			if (ev.getP1() > 0) {
				IMLOG("Dodajê");
				gg->addContact((char*)ev.getP2(), GG_USER_BLOCKED);
			} else {
				IMLOG("Usuwam");
				gg->removeContact((char*)ev.getP2(), GG_USER_BLOCKED);
				try {
					gg->addContact((char*)ev.getP2(), getCntType(Contact::find((tNet)abs(ev.getP1()), (char*)ev.getP2()).getStatus()));
				} catch (Exception&) {
					gg->addContact((char*)ev.getP2(), GG_USER_NORMAL);
				}
			}
		}
	}

	void Controller::handleConfig(ActionEvent &ev) {
		if (ev.withCode(ACTN_SAVE)) {
			char* buff = new char[1000];
			refreshServers(UIActionCfgGetValue(sUIAction(CFG::group, CFG::servers), buff, 1000));
			delete[] buff;
		}
	}

	void Controller::handleSetDefaultServers(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			UIActionCfgSetValue(sUIAction(CFG::group, CFG::servers), GG::defaultServers);
	}

	//todo: Te f-cje w¹tków s¹ do przepisania.
	//todo: Timeouty.
	void Controller::handleCreateGGAccount(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			threads.runEx(createAccount, 0, "CreateAccount");
	}

	void Controller::handleRemoveGGAccount(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			threads.runEx(removeAccount, 0, "RemoveAccount");
	}
	
	void Controller::handleChangePassword(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			threads.runEx(changePassword, 0, "ChangePassword");
	}

	void Controller::handleRemindPassword(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			threads.runEx(remindPassword, 0, "RemindPassword");
	}

	void Controller::handleImportCntList(ActionEvent &ev) {
		/*if (ev.withCode(ACTN_ACTION))
			importList();*/
	}

	void Controller::handleExportCntList(ActionEvent &ev) {
		/*if (ev.withCode(ACTN_ACTION))
			exportList();*/
	}

	void Controller::handleStatusDescription(ActionEvent& ev) {
		if (ev.withCode(ACTN_ACTION)) {
			sDIALOG_enter sde;
			sde.title = "Ustaw opis";
			sde.value = (char*)GETSTR(CFG::description);
			sde.info = "Podaj opis.";
			if (!ICMessage(IMI_DLGENTER, (int)&sde))
				return;
			IMessage(IM_CHANGESTATUS, (tNet)net, imtProtocol, gg->isConnected() ? -1 : ST_ONLINE, (int)sde.value);
			SETSTR(CFG::description, sde.value);
		}
	}

	void Controller::handleStatusServer(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			if ((ACTS_CHECKED & UIActionGetStatus(ev.getAction())) == ACTS_CHECKED) {
				UIActionSetStatus(ev.getAction(), 0);
				servers[ev.getAction().id - ACT::statusServer].default = false;
				SETSTR(CFG::selectedServer, "");
			} else {
				for (unsigned i = 0; i < serversCount; ++i) {
					if ((ACTS_CHECKED & UIActionGetStatus(sUIAction(ev.getParent(), ACT::statusServer + i))) == ACTS_CHECKED) {
						UIActionSetStatus(ev.getParent(), ACT::statusServer + i, 0);
						servers[i].default = false;
					}
				}
				UIActionSetStatus(ev.getAction(), ACTS_CHECKED);
				servers[ev.getAction().id - ACT::statusServer].default = true;
				SETSTR(CFG::selectedServer, servers[ev.getAction().id - ACT::statusServer].ip.c_str());
			}
		}
	}

	void Controller::handleStatusOnline(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			IMessage(IM_CHANGESTATUS, (tNet)net, imtProtocol, ST_ONLINE, (gg ? gg->isConnected() : false) ? 0 : (int)GETSTR(CFG::description));
		}	
	}

	void Controller::handleStatusAway(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			IMessage(IM_CHANGESTATUS, (tNet)net, imtProtocol, ST_AWAY, (gg ? gg->isConnected() : false) ? 0 : (int)GETSTR(CFG::description));
		}
	}

	void Controller::handleStatusInvisible(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			IMessage(IM_CHANGESTATUS, (tNet)net, imtProtocol, ST_HIDDEN, (gg ? gg->isConnected() : false) ? 0 : (int)GETSTR(CFG::description));
		}	
	}

	void Controller::handleStatusOffline(ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			IMessage(IM_CHANGESTATUS, (tNet)net, imtProtocol, ST_OFFLINE, 0);
		}	
	}

	void Controller::refreshServers(string serversString) {
		string default;
		if (!servers.empty()) {
			for (tServers::iterator i = servers.begin(); i != servers.end(); ++i) {
				if (i->default) {
					default = i->ip;
					break;
				}
			}
		} else {
			default = GETSTR(CFG::selectedServer);
		}

		servers.clear();
		servers = getServers(serversString, default);

		for (unsigned i = 0; i < serversCount; ++i) {
			if (i + 1 <= servers.size()) {
				UIActionSet(
					sUIActionInfo(ACT::statusServers, ACT::statusServer + i, -1, servers[i].default ? ACTS_CHECKED : 0, (char*)servers[i].ip.c_str())
				);
			} else {
				UIActionSet(
					sUIActionInfo(ACT::statusServers, ACT::statusServer + i, -1, ACTS_HIDDEN)
				);
			}
		}
	}

	void Controller::setProxy() {
		static char host[100];
		static char login[100];
		static char password[100];

		gg_proxy_http_only = GETINT(CFG_PROXY_HTTP_ONLY);
		gg_proxy_enabled = GETINT(CFG_PROXY);

		if (gg_proxy_enabled) {
			gg_proxy_host = strcpy(host, GETSTR(CFG_PROXY_HOST));
			gg_proxy_port = GETINT(CFG_PROXY_PORT);
			if (GETINT(CFG_PROXY_AUTH)) {
				gg_proxy_username = strcpy(login, (char*)GETSTR(CFG_PROXY_LOGIN));
				gg_proxy_password = strcpy(password, (char*)GETSTR(CFG_PROXY_PASS));
			} else {
				gg_proxy_username = 0;
				gg_proxy_password = 0;
			}
		} else {
			gg_proxy_host = 0;
			gg_proxy_port = 0;
		}
	}

	string Controller::getPassword() {
		string password = GETSTR(CFG::password);
		if (password.empty()) {
			sDIALOG_access sda;
			sda.flag = DFLAG_SAVE;
			sda.title = "Has³o do konta GG";
			sda.info = "Aby wykonaæ wybran¹ operacjê, musisz podaæ has³o do swojego konta GG.";
			sda.save = false;

			if (!ICMessage(IMI_DLGPASS, (int)&sda))
				return "";
			password = sda.pass;
			if (sda.save) {
				SETSTR(CFG::password, password.c_str());
			}
		}
		return password;
	}

	bool Controller::checkConnection(unsigned short criterion, bool warnUser) {
		if ((ccInternet & criterion) == ccInternet) {
			if (!ICMessage(IMC_CONNECTED)) {
				if (warnUser)
					ICMessage(IMI_ERROR, (int)"Operacja wymaga po³¹czenia z Internetem!\nSprawdŸ czy prawid³owo skonfigurowa³eœ po³¹czenie w konfiguracji.", MB_TASKMODAL | MB_OK);
				return false;
			}
		}

		if ((ccData & criterion) == ccData) {
			if (!strlen(GETSTR(CFG::login))) {
				if (warnUser)
					ICMessage(IMI_ERROR, (int)"Musisz podaæ numer Gadu-Gadu w konfiguracji.", MB_TASKMODAL | MB_OK);
				return false;
			}
		}

		if ((ccServer & criterion) == ccServer) {
			if (!gg->isConnected()) {
				if (warnUser)
					ICMessage(IMI_ERROR, (int)"Musisz byæ po³aczony z serwerem GG.", MB_TASKMODAL | MB_OK);
				return false;
			}
		}
		return true;
	}

	string Controller::getToken(Account& account, string title, string info) {
		ICMessage(IMC_RESTORECURDIR);
		string filename = (string)(char*)ICMessage(IMC_TEMPDIR) + "\\gg_token.gif";

		try {
			account.getToken(filename);
		} catch (Exception& e) {
			ICMessage(IMI_ERROR, (int)e.getReason().a_str());
			return "";
		}

		sDIALOG_token dt;
		dt.title = title.c_str();
		dt.info = info.c_str();
		string URL = "file://" + filename;
		dt.imageURL = URL.c_str();
		ICMessage(IMI_DLGTOKEN, (int)&dt);

		return dt.token;
	}

	unsigned __stdcall Controller::ggWatchThread(LPVOID lParam) {
		PlugStatusChange(ST_CONNECTING);

		Controller* c = Singleton<Controller>::getInstance();
		tStatus status = ((tStatusInfo*)lParam)->first;
		string statusDescription = ((tStatusInfo*)lParam)->second;
		delete lParam;

		if (c->gg->connect(status, statusDescription)) {
			PlugStatusChange(status, statusDescription.c_str());

			unsigned cntCount = IMessage(IMC_CNT_COUNT);
			tContacts cnts;
			for (unsigned i = 1; i < cntCount; ++i) {
				Contact cnt(i);
				if (cnt.getNet() == net) {
					cnts.push_back(cnt);
				}
			}
			c->gg->sendContacts(cnts);

			//hack: Dziwne, ale po wys³aniu pinga siê disconnectuje…
			//c->timer.repeat(ggPingTimer, c, 60000);

			c->gg->startListening();
		}

		PlugStatusChange(ST_OFFLINE);
		c->resetCnts();
		return 0;
	}
	
	unsigned __stdcall Controller::createAccount(LPVOID lParam) {
		Controller* c = (Controller*)lParam;

		if (ICMessage(IMI_CONFIRM, (int)"Zostanie za³o¿one nowe konto w sieci Gadu-Gadu™.\nKontynuowaæ?", MB_TASKMODAL | MB_YESNO) == IDNO) 
			return 0;

		sDIALOG_access sda;
		sda.save = 1;
		sda.title = "GG - nowe konto [krok 1/3]";
		sda.info = "Podaj has³o do nowego konta.";
		if (!ICMessage(IMI_DLGSETPASS, (int)&sda, 1) || !*sda.pass)
			return 0;
		string password = sda.pass;
		bool save = sda.save;

		sDIALOG_enter sde;
		sde.title = "GG - nowe konto [krok 2/3]";
		sde.info = "Podaj swój adres email, na który bêdzie mo¿na kiedyœ przes³aæ has³o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde) || !*sde.value)
			return 0;
		string email = sde.value;
		
		Account account;

		string tokenVal = c->getToken(account, "GG - nowe konto [krok 3/3]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.");
		if (tokenVal.empty())
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - zak³adanie konta";
		sdl.info = "Komunikujê siê z serwerem…";
		sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
		/*sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;*/
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		c->setProxy();

		if (!account.createAccount(password, email, tokenVal)) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)"Wyst¹pi³ b³¹d podczas zak³adania konta. SprawdŸ po³¹czenie i spróbuj ponownie.", 0);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		UIActionCfgSetValue(sUIAction(CFG::group, CFG::login), account.getLogin().c_str());
		SETINT(CFG::login, account.getUid());
		UIActionCfgSetValue(sUIAction(CFG::group, CFG::password), save ? account.getPassword().c_str() : "");
		SETSTR(CFG::password, save ? account.getPassword().c_str() : "");
		ICMessage(IMC_SAVE_CFG);

		ICMessage(IMI_INFORM, (int)stringf("Konto zosta³o za³o¿one!\nTwój numer UID to %d", account.getUid()).c_str());
		return 0;
	}
	
	unsigned __stdcall Controller::removeAccount(LPVOID lParam) {
		Controller* c = (Controller*)lParam;
		
		if (ICMessage(IMI_CONFIRM, (int)"Twoje konto na gadu-gadu zostanie usuniête!\nKontynuowaæ?", MB_TASKMODAL | MB_YESNO) == IDNO) 
			return 0;

		sDIALOG_access sda;
		sda.title = "GG - usuwanie konta [krok 1/2]";
		sda.info = "Podaj has³o do konta";
		if (!ICMessage(IMI_DLGPASS, (int)&sda))
			return 0;
		string password = sda.pass;

		Account account(GETINT(CFG::login), password);

		string tokenVal = c->getToken(account, "GG - usuwanie konta [krok 2/2]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.");
		if (tokenVal.empty())
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - usuwanie konta";
		sdl.info = "Komunikujê siê z serwerem…";
		sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
		/*sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;*/
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);
		
		c->setProxy();

		if (!account.removeAccount(tokenVal)) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)"Wyst¹pi³ b³¹d podczas usuwania konta. SprawdŸ po³¹czenie oraz czy poda³eœ prawid³owy numer i has³o i spróbuj ponownie.", 0);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		UIActionCfgSetValue(sUIAction(CFG::group, CFG::login), "");
		SETSTR(CFG::login, "");
		UIActionCfgSetValue(sUIAction(CFG::group, CFG::password), "");
		SETSTR(CFG::password, "");

		ICMessage(IMI_INFORM, (int)"Konto zosta³o usuniête.");
		return 0;
	}
	
	unsigned __stdcall Controller::changePassword(LPVOID lParam) {
		Controller* c = (Controller*)lParam;

		if (ICMessage(IMI_CONFIRM, (int)"Czy na pewno chcesz zmieniæ has³o?", MB_TASKMODAL | MB_YESNO) == IDNO)
			return 0;

		sDIALOG_access sda;
		string oldPassword;
		sda.save = 0;
		sda.flag = 0;
		sda.title = "GG - zmiana has³a [1/4]";
		sda.info = "Podaj aktualne has³o.";
		if (!ICMessage(IMI_DLGPASS, (int)&sda, 0))
			return 0;
		oldPassword = sda.pass;

		sda.pass = "";
		sda.save = 1;
		sda.flag = DFLAG_SAVE;
		sda.title = "GG - zmiana has³a [2/4]";
		sda.info = "Podaj nowe has³o do twojego konta.";
		if (!ICMessage(IMI_DLGSETPASS, (int)&sda, 1)) 
			return 0;
		string password = sda.pass;
		bool save = sda.save;

		sDIALOG_enter sde;
		sde.title = "GG - zmiana has³a [3/4]";
		sde.info = "Podaj swój adres email, na który bêdzie mo¿na kiedyœ przes³aæ has³o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde)) return 0;
		string email = sde.value;

		Account account(GETINT(CFG::login), oldPassword);

		string tokenVal = c->getToken(account, "GG - zmiana has³a [krok 4/4]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.");
		if (tokenVal.empty())
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - zmiana has³a";
		sdl.info = "Komunikujê siê z serwerem…";
		sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
		/*sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;*/
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		c->setProxy();
		if (!account.changePassword(password, email, tokenVal)) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)"Wyst¹pi³ b³¹d podczas zmiany has³a dla konta %d . SprawdŸ po³¹czenie oraz czy poda³eœ prawid³owe has³o i spróbuj ponownie.", 0);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		UIActionCfgSetValue(sUIAction(CFG::group, CFG::password), save ? account.getPassword().c_str() : "");
		SETSTR(CFG::password, save ? account.getPassword().c_str() : "");

		ICMessage(IMI_INFORM, (int)stringf("Has³o dla konta %d zosta³o zmienione!", account.getUid()).c_str());
		return 0;
	}
	
	unsigned __stdcall Controller::remindPassword(LPVOID lParam) {
		Controller* c = (Controller*)lParam;

		sDIALOG_enter sde;
		sde.title = "GG - przypomnienie has³a [1/2]";
		sde.info = "Podaj adres email, który wpisa³eœ podczas zak³adania konta. Na ten email otrzymasz has³o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde))
			return 0;
		string email = sde.value;

		Account account(GETINT(CFG::login), GETSTR(CFG::password));

		string tokenVal = c->getToken(account, "GG - przypomnienie has³a [2/2]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.");
		if (tokenVal.empty())
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - przypomnienie has³a";
		sdl.info = "Komunikujê siê z serwerem…";
		sdl.flag = DLONG_AINET | DLONG_CANCEL;
		/*sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;*/
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		c->setProxy();
		if (!account.remindPassword(email, tokenVal)) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)"Wyst¹pi³ b³¹d! SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy adres email i spróbuj ponownie.", MB_TASKMODAL | MB_OK);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		ICMessage(IMI_INFORM, (int)"Has³o zosta³o wys³ane.");
		return 0;
	}
	
	void __stdcall Controller::ggPingTimer(LPVOID lParam, DWORD dwTimerLowValue, DWORD dwTimerHighValue) {
		((Controller*)lParam)->gg->ping();
	}

	void Controller::ggEventHandler(gg_event* event) {
		Controller* c = Singleton<Controller>::getInstance();

		switch (event->type) {
			case GG_EVENT_CONN_FAILED: {
				IMLOG("GG_EVENT_CONN_FAILED");
				ICMessage(IMI_ERROR, (int)"Poda³eœ z³y numer konta lub has³o dla GG.\r\nSprawdŸ w konfiguracji i spróbuj po³¹czyæ siê ponownie.", (int)"Konnekt - GG");
				break;
			} case GG_EVENT_DISCONNECT: {
				IMLOG("GG_EVENT_DISCONNECT");
				break;
			} case GG_EVENT_NOTIFY60: {
				IMLOG("GG_EVENT_NOTIFY60");
				for (unsigned i = 0; event->event.notify60[i].uin; ++i) {
					try {
						Contact cnt(Contact::find((tNet)GG::net, inttostr(event->event.notify60[i].uin)));
						c->setCntStatus(cnt, convertGGStatus(event->event.notify60[i].status),
							event->event.notify60[i].descr ? event->event.notify60[i].descr : "",
							event->event.notify60[i].remote_ip,
							event->event.notify60[i].remote_port
						);
					} catch (Exception& e) {
						IMLOG("%s", e.getReason().a_str());
					}
				}
				ICMessage(IMI_REFRESH_LST);
				break;
			} case GG_EVENT_STATUS60: {
				IMLOG("GG_EVENT_STATUS60");
				try {
					Contact cnt(Contact::find((tNet)net, inttostr(event->event.status60.uin)));
					c->setCntStatus(cnt, convertGGStatus(event->event.status60.status),
						event->event.status60.descr ? event->event.status60.descr : "",
						event->event.status60.remote_ip,
						event->event.status60.remote_port
					);
				} catch (Exception& e) {
					IMLOG("%s", e.getReason().a_str());
					break;
				}
				ICMessage(IMI_REFRESH_LST);
				break;
			} case GG_EVENT_STATUS: {
				IMLOG("GG_EVENT_STATUS");
				try {
					Contact cnt(Contact::find((tNet)net, inttostr(event->event.status.uin)));
					c->setCntStatus(cnt, convertGGStatus(event->event.status.status),
						event->event.status.descr ? event->event.status.descr : ""
					);
				} catch (ExceptionString& e) {
					IMLOG("%s", e.getReason().a_str());
					break;
				}
				ICMessage(IMI_REFRESH_LST);
				break;
			} default: {
				IMLOG("GG_EVENT_… = %i", event->type);
			}
		}
	}

	void Controller::setCntStatus(Contact& cnt, tStatus status, string description, long ip, int port) {
		ICMessage(IMI_CNT_ACTIVITY, cnt.getID());
		CntSetStatus(cnt.getID(), status, description.c_str());
		if (ip)
			cnt.setHost(longToIp(ip));
		if (port)
			cnt.setPort(port);
	}

	void Controller::resetCnts() {
		IMLOG("[Controller::resetCnts()]");
		unsigned cntCount = IMessage(IMC_CNT_COUNT);
		for (unsigned i = 1; i < cntCount; ++i) {
			Contact cnt(i);
			if (cnt.getNet() == net) {
				setCntStatus(cnt, ST_OFFLINE, "");
				ICMessage(IMI_CNT_DEACTIVATE, i);
			}
		}
		ICMessage(IMI_REFRESH_LST);
	}
}