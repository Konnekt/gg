#include "stdafx.h"
#include "Controller.h"

namespace GG {
	//debug: To tylko tymczasowe.
	void handler (int level, const char * format, va_list p) {
		int size = _vscprintf(format, p);
		char * buff = new char [size + 2];
		buff[size + 1] = 0;
		size = _vsnprintf_s(buff, size + 2, size + 1, format, p);
		buff[size] = 0;
		Singleton<Controller>::getInstance()->getCtrl()->logMsg(Stamina::logWarn, "", "", buff);
		delete [] buff;
	}

	Controller::Controller() : connected(false), status(ST_OFFLINE), session(0) {
		gg_debug_level = GG_DEBUG_MISC;
		gg_debug_handler = &handler;
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
		//d.connect(IM_END, bind(&Controller::onEnd, this, _1));
		//d.connect(IM_DISCONNECT, bind(&Controller::onDisconnect, this, _1));
		d.connect(IM_GET_STATUS, bind(&Controller::onGetStatus, this, _1));
		d.connect(IM_GET_STATUSINFO, bind(&Controller::onGetStatusInfo, this, _1));
		//d.connect(IM_GET_UID, bind(&Controller::onGetUID, this, _1));
		//todo: Pami�ta�, �e zamiast IM_MSG_RCV i IM_MSG_SEND jest nowa MQ
		//d.connect(IM_CNT_ADD, bind(&Controller::onCntAdd, this, _1));
		//d.connect(IM_CNT_REMOVE, bind(&Controller::onCntRemove, this, _1));
		//d.connect(IM_CNT_CHANGED, bind(&Controller::onCntChanged, this, _1));
		//d.connect(IM_CNT_DOWNLOAD, bind(&Controller::onCntDownload, this, _1));
		//d.connect(IM_CNT_UPLOAD, bind(&Controller::onCntUpload, this, _1));
		//d.connect(IM_CNT_SEARCH, bind(&Controller::onCntSearch, this, _1));
		//d.connect(IM_IGN_CHANGED, bind(&Controller::onIgnChanged, this, _1));
		//d.connect(IM_ISCONNECTED, bind(&Controller::onIsConnected, this, _1));
		d.connect(IM_CHANGESTATUS, bind(&Controller::onChangeStatus, this, _1));

		//API
		//d.connect(api::isEnabled, bind(&Controller::apiEnabled, this, _1));

		//Akcje
		a.connect(ACT::setDefaultServers, bind(&Controller::handleSetDefaultServers, this, _1));
		a.connect(ACT::createGGAccount, bind(&Controller::handleCreateGGAccount, this, _1));
		a.connect(ACT::removeGGAccount, bind(&Controller::handleRemoveGGAccount, this, _1));
		a.connect(ACT::changePassword, bind(&Controller::handleChangePassword, this, _1));
		a.connect(ACT::remindPassword, bind(&Controller::handleRemindPassword, this, _1));
		a.connect(ACT::importCntList, bind(&Controller::handleImportCntList, this, _1));
		a.connect(ACT::exportCntList, bind(&Controller::handleExportCntList, this, _1));
		a.connect(ACT::statusDescripton, bind(&Controller::handleStatusDescription, this, _1));
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
		c.setColumn(tableConfig, CFG::useSSL, ctypeInt, 0, "GG/useSSL");
		c.setColumn(tableConfig, CFG::resumeDisconnected, ctypeInt, 1, "GG/resumeDisconnected");
	}

	void Controller::onStart(IMEvent &ev) {
		this->statusDescription = GETSTR(CFG::description);
	}

	void Controller::onPrepareUI(IMEvent &ev) {
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
		UIGroupAdd(IMIG_CFG_USER, CFG::group, 0, "GG", ICO::logo);
		UIActionCfgAddPluginInfoBox2(CFG::group,
			"Wtyczka umo�liwia komunikacj� przy pomocy najpopularniejszego protoko�u w Polsce."
			, "U�yto biblioteki <b>libgadu</b> - http://toxygen.net/libgadu/<br/>"
			"Modyfikacje - <b>Micha� \"Dulek\" Dulko</b><br/>"
			"Copyright � 2003-2008 <b>Stamina</b><br/><br/>"
			"<span class='note'>Skompilowano: <b>" __DATE__ "</b> [<b>" __TIME__ "</b>]</span>"
			, ("reg://IML16/" + inttostr(ICO::logo) + ".ico").c_str(), -3
		);

		UIActionCfgAdd(CFG::group, 0, ACTT_GROUP, "Konto na serwerze GG");
		UIActionCfgAdd(CFG::group, 0, ACTT_COMMENT | ACTSC_INLINE, "Numer GG", 0, 0, 0, 60);
		UIActionCfgAdd(CFG::group, CFG::login, ACTT_EDIT | ACTSC_INT, "", CFG::login);
		UIActionCfgAdd(CFG::group, 0, ACTT_COMMENT | ACTSC_INLINE, "Has�o", 0, 0, 0, 60);
		UIActionCfgAdd(CFG::group, CFG::password, ACTT_PASSWORD, "", CFG::password);

		UIActionAdd(CFG::group, 0, ACTT_SEPARATOR);

		UIActionAdd(CFG::group, ACT::createGGAccount, ACTT_BUTTON | ACTSC_INLINE | ACTSC_BOLD, SetActParam("Za�� konto", AP_ICO, inttostr(ICON_ACCOUNTCREATE)).c_str(), 0, 155, 30);
		UIActionAdd(CFG::group, ACT::importCntList, ACTT_BUTTON | ACTSC_BOLD | ACTSC_FULLWIDTH, SetActParam("Importuj list� kontakt�w", AP_ICO, inttostr(ICON_IMPORT)).c_str(), 0, 180, 30);
		UIActionAdd(CFG::group, ACT::changePassword, ACTT_BUTTON | ACTSC_INLINE, SetActParam("Zmie� has�o", AP_ICO, inttostr(ICON_CHANGEPASSWORD)).c_str(), 0, 155, 30);
		UIActionAdd(CFG::group, ACT::remindPassword, ACTT_BUTTON | ACTSC_FULLWIDTH, SetActParam("Przypomnij has�o", AP_ICO, inttostr(ICON_REMINDPASSWORD)).c_str(), 0, 180, 30);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");

		UIActionAdd(CFG::group, 0, ACTT_GROUP, "Ustawienia");
		UIActionAdd(CFG::group, 0, ACTT_COMMENT | ACTSC_INLINE, "Status startowy");
		UIActionAdd(CFG::group, IMIB_CFG, ACTT_COMBO | ACTSCOMBO_LIST,
			"Ostatni" CFGICO "#74" CFGVALUE "0\n"
			"Niedost�pny" CFGICO "0x40A00000" CFGVALUE "1\n"
			"Dost�pny" CFGICO "0x40A00400" CFGVALUE "2\n"
			"Zaj�ty" CFGICO "0x40A00410" CFGVALUE "3\n"
			"Ukryty" CFGICO "0x40A00420" CFGVALUE "4"
			AP_PARAMS AP_TIP "Status, kt�ry zostanie ustawiony po uruchomieniu programu", 
			CFG::startStatus
		);
		UIActionAdd(CFG::group, CFG::useSSL, ACTT_CHECK, "U�ywaj bezpiecznego po��czenia (SSL)", CFG::useSSL);
		UIActionAdd(CFG::group, CFG::friendsOnly, ACTT_CHECK, "M�j status widoczny tylko u znajomych z listy kontakt�w", CFG::friendsOnly);
		UIActionAdd(CFG::group, CFG::resumeDisconnected, ACTT_CHECK, "��cz ponownie, je�eli serwer zako�czy po��czenie." AP_TIP "Wy��cz t� opcj�, je�eli cz�sto korzystasz z konta w r�nych miejscach. Zapobiega cyklicznemu \"prze��czaniu\" pomi�dzy w��czonymi programami.", CFG::resumeDisconnected);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND);

		UIActionAdd(CFG::group, 0, ACTT_GROUP, "Serwery");
		UIActionAdd(CFG::group, CFG::servers, ACTT_TEXT | ACTSC_INLINE, "" CFGTIP "Je�eli zostawisz to pole puste - zostanie u�yty serwer wskazany przez hub GG.", CFG::servers, 150);
		UIActionAdd(CFG::group, 0, ACTT_TIPBUTTON | ACTSC_INLINE, AP_TIPRICH "W ka�dej linijce jeden serwer. Pusta linijka oznacza HUB (system zwracaj�cy najmniej obci��ony serwer)."
			"<br/><b>Format</b> (zawarto�� [...] jest opcjonalna):"
			"<br/><i>Adres</i>[:<i>port</i>]"
			"<br/>SSL[ <i>Adres</i>[:<i>port</i>]] (po��czenie szyfrowane)"
			"<br/><br/>Aby wy��czy� serwer dodaj <b>!</b> na pocz�tku."
			AP_TIPRICH_WIDTH "300"
		);
		UIActionAdd(CFG::group, ACT::setDefaultServers, ACTT_BUTTON, SetActParam("Domy�lne", AP_ICO, inttostr(ICON_DEFAULT)).c_str(), 0, 0, 25);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");

		UIActionAdd(CFG::group, 0, ACTT_GROUP, "");
		UIActionAdd(CFG::group, 0, ACTT_TIPBUTTON | ACTSC_INLINE, SetActParam(AP_TIPRICH "<b>UWAGA!</b> Konto zostanie <u>bezpowrotnie</u> usuni�te z serwera GG! Nie b�dziesz m�g� wi�cej korzysta� z tego numeru!", AP_ICO, inttostr(ICON_WARNING)).c_str(), 0, 30, 30);
		UIActionAdd(CFG::group, ACT::removeGGAccount, ACTT_BUTTON, SetActParam("Usu� konto z serwera", AP_ICO, inttostr(ICON_ACCOUNTREMOVE)).c_str(), 0, 170, 30);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");

		//zmiana statusu
		UIGroupAdd(IMIG_STATUS, ACT::status, 0, "GG", ICO::offline);
		UIActionAdd(ACT::status, ACT::statusDescripton, 0, "Opis", 0);
		UIActionAdd(ACT::status, ACT::statusOnline, 0, "Dost�pny", ICO::online);
		UIActionAdd(ACT::status, ACT::statusAway, 0, "Zaraz wracam", ICO::away);
		UIActionAdd(ACT::status, ACT::statusInvisible, 0, "Ukryty", ICO::invisible);
		UIActionAdd(ACT::status, ACT::statusOffline, 0, "Niedost�pny", ICO::offline);

		ev.setSuccess();
	}

	void Controller::onChangeStatus(IMEvent &ev) {
		if (!isConnected())
			connect(ev.getP1(), (char*)ev.getP2());
		else
			setStatus(ev.getP1(), (char*)ev.getP2());
	}

	void Controller::onGetStatus(IMEvent &ev) {
    ev.setReturnValue(status);
	}
	
	void Controller::onGetStatusInfo(IMEvent &ev) {
		ev.setReturnValue(strcpy((char*)Ctrl->GetTempBuffer(statusDescription.size() + 1), statusDescription.c_str()));
	}

	void Controller::handleSetDefaultServers(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			UIActionCfgSetValue(sUIAction(CFG::group, CFG::servers), GG::defaultServers);
	}

	void Controller::handleCreateGGAccount(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			CloseHandle(Ctrl->BeginThread("CreateGGAccount", 0, 0, createGGAccount, 0, 0, 0));
	}

	void Controller::handleRemoveGGAccount(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			CloseHandle(Ctrl->BeginThread("RemoveGGAccount", 0, 0, removeGGAccount, 0, 0, 0));
	}
	
	void Controller::handleChangePassword(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			CloseHandle(Ctrl->BeginThread("ChangePassword", 0, 0, changePassword, 0, 0, 0));
	}

	void Controller::handleRemindPassword(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			CloseHandle(Ctrl->BeginThread("RemindPassword", 0, 0, remindPassword, 0, 0, 0));
	}

	void Controller::handleImportCntList(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			importList();
	}

	void Controller::handleExportCntList(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION))
			exportList();
	}

	void Controller::handleStatusDescription(ActionEvent& ev) {
		if (ev.withCode(ACTN_ACTION)) {
			sDIALOG_enter sde;
			sde.title = "Ustaw opis";
			sde.value = (char*)GETSTR(CFG::description);
			sde.info = "Podaj opis.";
			if (!ICMessage(IMI_DLGENTER, (int)&sde))
				return;
			//todo: Net::gg, z tym b�d� cyrki. ;)
			IMessage(IM_CHANGESTATUS, Net::gg, imtProtocol, -1, (int)sde.value);
			SETSTR(CFG::description, sde.value);
		}
	}

	void Controller::handleStatusOnline(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			IMessage(IM_CHANGESTATUS, Net::gg, imtProtocol, ST_ONLINE, isConnected() ? (int)statusDescription.c_str() : (int)GETSTR(CFG::description));
		}	
	}

	void Controller::handleStatusAway(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			IMessage(IM_CHANGESTATUS, Net::gg, imtProtocol, ST_AWAY, isConnected() ? (int)statusDescription.c_str() : (int)GETSTR(CFG::description));
		}
	}

	void Controller::handleStatusInvisible(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			IMessage(IM_CHANGESTATUS, Net::gg, imtProtocol, ST_HIDDEN, isConnected() ? (int)statusDescription.c_str() : (int)GETSTR(CFG::description));
		}	
	}

	void Controller::handleStatusOffline(Konnekt::ActionEvent &ev) {
		if (ev.withCode(ACTN_ACTION)) {
			disconnect(statusDescription.c_str());
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
			sda.title = "Has�o do konta GG";
			sda.info = "Aby wykona� wybran� operacj�, musisz poda� has�o do swojego konta GG.";
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
					ICMessage(IMI_ERROR, (int)"Operacja wymaga po��czenia z Internetem!\nSprawd� czy prawid�owo skonfigurowa�e� po��czenie w konfiguracji.", MB_TASKMODAL | MB_OK);
				return false;
			}
		}
		
		if ((ccData & criterion) == ccData) {
			if (!strlen(GETSTR(CFG::login))) {
				if (warnUser)
					ICMessage(IMI_ERROR, (int)"Musisz poda� numer Gadu-Gadu w konfiguracji.", MB_TASKMODAL | MB_OK);
				return false;
			}
		}
		
		if ((ccServer & criterion) == ccServer) {
			if (!isConnected()) {
				if (warnUser)
					ICMessage(IMI_ERROR, (int)"Musisz by� po�aczony z serwerem GG.", MB_TASKMODAL | MB_OK);
				return false;
			}
		}
		return true;
	}

	bool Controller::connect(tStatus status, const char* description) {
		if (isConnected() || this->status != ST_OFFLINE)
			return false;
		if (!checkConnection(ccInternet | ccData))
			return false;

		PlugStatusChange(ST_CONNECTING);
		this->status = ST_CONNECTING;

		gg_login_params params;
		memset(&params, 0, sizeof(params));
		params.uin = atoi(GETSTR(CFG::login));
		string password = getPassword();
		if (password.empty()) {
			PlugStatusChange(ST_OFFLINE);
			this->status = ST_OFFLINE;
			this->statusDescription = "";
			return false;
		}
		params.password = (char*)password.c_str();
		params.status = convertKStatus(status, description);
		params.status_descr = (char*)description;
		params.async = false;
		params.client_version = GG_DEFAULT_CLIENT_VERSION;
		params.protocol_version = GG_DEFAULT_PROTOCOL_VERSION;
		params.last_sysmsg = 0;
		params.image_size = 0;
		params.tls = false;

		/*hostent* host = gethostbyname("91.197.13.17");
		if (host) {
			memcpy(&params.server_addr, host->h_addr, 4);
		}*/

		session = gg_login(&params);
		if (session) {
			PlugStatusChange(status, description);
			this->status = status;
			this->statusDescription = description;

			//debug: Do usuni�cia, co ciekawe - nie wysy�a.
			gg_send_message(session, GG_CLASS_CHAT, 1169042, (const unsigned char*)"Cze��!");

			return connected = true;
		}
		else {
			PlugStatusChange(ST_OFFLINE);
			this->status = ST_OFFLINE;
			this->statusDescription = "";
			return false;
		}
	}

	void Controller::setStatus(tStatus status, const char* description) {
		PlugStatusChange(status, description);

		string setDescription = description ? description : statusDescription.c_str();
		int setStatus = convertKStatus(status, setDescription.c_str()) | GETINT(CFG::friendsOnly) ? GG_STATUS_FRIENDS_MASK : 0;
		gg_change_status_descr(session, setStatus, setDescription.c_str());

		this->status = status;
		this->statusDescription = description;
	}

	void Controller::disconnect(const char* description) {
		if (!isConnected())
			return;
		setStatus(ST_OFFLINE, description);
		gg_logoff(session);
		gg_free_session(session);
		connected = false;
		this->statusDescription = "";
	}
}