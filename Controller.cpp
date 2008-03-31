#include "stdafx.h"
#include "Controller.h"

namespace GG {
	Controller::Controller() {
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
		/*d.connect(IM_START, bind(&Controller::onStart, this, _1));
		d.connect(IM_END, bind(&Controller::onEnd, this, _1));
		d.connect(IM_DISCONNECT, bind(&Controller::onDisconnect, this, _1));
		d.connect(IM_GET_STATUS, bind(&Controller::onGetStatus, this, _1));
		d.connect(IM_GET_STATUSINFO, bind(&Controller::onGetStatusInfo, this, _1));
		d.connect(IM_GET_UID, bind(&Controller::onGetUID, this, _1));
		//TODO: Pamiêtaæ, ¿e zamiast IM_MSG_RCV i IM_MSG_SEND jest nowa MQ
		d.connect(IM_CNT_ADD, bind(&Controller::onCntAdd, this, _1));
		d.connect(IM_CNT_REMOVE, bind(&Controller::onCntRemove, this, _1));
		d.connect(IM_CNT_CHANGED, bind(&Controller::onCntChanged, this, _1));
		d.connect(IM_CNT_DOWNLOAD, bind(&Controller::onCntDownload, this, _1));
		d.connect(IM_CNT_UPLOAD, bind(&Controller::onCntUpload, this, _1));
		d.connect(IM_CNT_SEARCH, bind(&Controller::onCntSearch, this, _1));
		d.connect(IM_IGN_CHANGED, bind(&Controller::onIgnChanged, this, _1));
		d.connect(IM_ISCONNECTED, bind(&Controller::onIsConnected, this, _1));
		d.connect(IM_CHANGESTATUS, bind(&Controller::onChangeStatus, this, _1));*/

		//API
		//d.connect(api::isEnabled, bind(&Controller::apiEnabled, this, _1));

		//Akcje
		//a.connect(ui::cntCfgGroup, bind(&Controller::_handleCntGroup, this, _1));

		//Konfiguracja
		c.setColumn(tableConfig, CFG::login, ctypeString, "", "GG/login");
		c.setColumn(tableConfig, CFG::password, ctypeString | cflagSecret | cflagXor, "", "GG/password");
		c.setColumn(tableConfig, CFG::status, ctypeInt, 0, "GG/status");
		c.setColumn(tableConfig, CFG::startStatus, ctypeInt, 0, "GG/startStatus");
		c.setColumn(tableConfig, CFG::friendsOnly, ctypeInt, 0, "GG/friedsOnly");
		c.setColumn(tableConfig, CFG::description, ctypeString, "", "GG/description");
		c.setColumn(tableConfig, CFG::servers, ctypeString, "s1.gadu-gadu.pl\r\n\r\n217.17.41.83\r\n217.17.41.84\r\n217.17.41.85\r\n217.17.41.86\r\n217.17.41.87\r\n217.17.41.88\r\n217.17.41.92", "GG/servers");
		c.setColumn(tableConfig, CFG::trayMenu, ctypeInt, 0, "GG/trayMenu");
		c.setColumn(tableConfig, CFG::useSSL, ctypeInt, 0, "GG/useSSL");
		c.setColumn(tableConfig, CFG::resumeDisconnected, ctypeInt, 0, "GG/resumeDisconnected");
	}
	
	void Controller::onPrepareUI(IMEvent &ev) {
		//Ikony
		IconRegister(IML_16, ICO::logo, Ctrl->hDll(), IDI_LOGO);
		IconRegister(IML_16, ICO::server, Ctrl->hDll(), IDI_SERVER);
		IconRegister(IML_16, ICO::overlay, Ctrl->hDll(), IDI_OVERLAY);
		IconRegister(IML_16, ICO::online, Ctrl->hDll(), IDI_ONLINE);
		IconRegister(IML_16, ICO::away, Ctrl->hDll(), IDI_AWAY);
		IconRegister(IML_16, ICO::invisible, Ctrl->hDll(), IDI_INVISIBLE);
		IconRegister(IML_16, ICO::offline, Ctrl->hDll(), IDI_OFFLINE);
		IconRegister(IML_16, ICO::blocking, Ctrl->hDll(), IDI_BLOCKING);
		IconRegister(IML_16, ICO::connecting, Ctrl->hDll(), IDI_CONNECTING);

		//Config
		UIGroupAdd(IMIG_CFG_USER, CFG::group, 0, "GG", UIIcon(2, GG::net,0,0));
		UIActionCfgAddPluginInfoBox2(CFG::group, 
			"<div>Wtyczka umo¿liwia komunikacjê przy pomocy najpopularniejszego protoko³u w Polsce."
			, "Wykorzystano bibliotekê <b>LibGadu</b> (http://toxygen.net/libgadu/)"
			"<br/>Strona domowa protoko³u GG - http://www.gadu-gadu.pl/"
			"<span class='note'>Skompilowano: <b>" __DATE__ "</b> [<b>" __TIME__ "</b>]</span><br/>"
			"<br/>Copyright © 2003-2008 <b>Stamina</b>"
			, ("reg://IML16/" + inttostr(ICO::logo) + ".ico").c_str(), -3
		);

		UIActionCfgAdd(CFG::group, 0, ACTT_GROUP, "Konto na serwerze GG");
		UIActionCfgAdd(CFG::group, 0, ACTT_COMMENT | ACTSC_INLINE, "Numer GG", 0, 70);
		UIActionCfgAdd(CFG::group, CFG::login, ACTT_EDIT | ACTSC_INT, "", CFG::login, 65);
		UIActionCfgAdd(CFG::group, 0, ACTT_COMMENT | ACTSC_INLINE, "Has³o", 0, 70);
		UIActionCfgAdd(CFG::group, CFG::password, ACTT_PASSWORD, "", CFG::password, 65);

		UIActionAdd(CFG::group, 0, ACTT_SEPARATOR);

		/*UIActionAdd(CFG::group, IMIC_GG_ACCOUNT, ACTT_BUTTON | ACTSC_INLINE | ACTSC_BOLD, SetActParam("Za³ó¿ konto", AP_ICO, inttostr(ICON_ACCOUNTCREATE)), 0, 155, 30);
		UIActionAdd(CFG::group, IMIA_LIST_GG_IMPORT, ACTT_BUTTON | ACTSC_BOLD | ACTSC_FULLWIDTH, SetActParam("Importuj listê kontaktów", AP_ICO, inttostr(ICON_IMPORT)), 0, 180, 30);
		UIActionAdd(CFG::group, IMIC_GG_NEWPASS, ACTT_BUTTON|ACTSC_INLINE, txt, 0, 155,30);
		txt = SetActParam("Przypomnij has³o", AP_ICO, inttostr(ICON_REMINDPASSWORD));
		UIActionAdd(CFG::group, IMIC_GG_REMINDPASS ,ACTT_BUTTON | ACTSC_FULLWIDTH,txt,0,180, 30);

		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");
		UIActionAdd(CFG::group, 0, ACTT_GROUP, "Ustawienia");
		UIActionAdd(CFG::group, IMIB_CFG, ACTT_COMBO|ACTSCOMBO_LIST|ACTSC_INLINE,
			"Ostatni" CFGICO "#74" CFGVALUE "0\n"
			"Niedostêpny" CFGICO "0x40A00000" CFGVALUE "1\n"
			"Dostêpny" CFGICO "0x40A00400" CFGVALUE "2\n"
			"Zajêty" CFGICO "0x40A00410" CFGVALUE "3\n"
			"Ukryty" CFGICO "0x40A00420" CFGVALUE "20"
			AP_PARAMS AP_TIP "Status, który zostanie ustawiony po uruchomieniu programu", 
			CFG_GG_STARTSTATUS
		);
			
		UIActionAdd(CFG::group, 0, ACTT_COMMENT, "Status startowy");
		UIActionAdd(CFG::group, 0, ACTT_CHECK, "U¿ywaj bezpiecznego po³¹czenia, jeœli to mo¿liwe", CFG_GG_USESSL);

		UIActionAdd(CFG::group, IMIB_CFG, ACTT_CHECK,"Mój status widoczny tylko u znajomych z mojej listy",CFG_GG_FRIENDSONLY);

		UIActionAdd(CFG::group, 0 ,ACTT_CHECK | ACTSC_NEEDRESTART,"Pe³ne menu w zasobniku systemowym" AP_TIP "Wszystkie statusy bêd¹ dostêpne bezpoœrednio w menu zasobnika (tray)",CFG_GG_TRAYMENU);
		UIActionAdd(CFG::group, 0, ACTT_CHECK, "Nie ³¹cz ponownie je¿eli serwer zakoñczy po³¹czenie." AP_TIP "W³¹cz t¹ opcjê, je¿eli czêsto korzystasz z konta w ró¿nych miejscach. Zapobiega cyklicznemu \"prze³¹czaniu\" pomiêdzy w³¹czonymi programami.", CFG_GG_DONTRESUMEDISCONNECTED);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND);

		UIActionAdd(CFG::group, 0, ACTT_GROUP, "Serwery");
		UIActionAdd(CFG::group, IMIB_CFG, ACTT_TEXT|ACTSC_INLINE, "" CFGTIP "Je¿eli zostawisz to pole puste - zostanie u¿yty serwer wskazany przez hub GG.", CFG_GG_SERVER, 150);
		UIActionAdd(CFG::group, 0 ,ACTT_TIPBUTTON|ACTSC_INLINE, AP_TIPRICH "W ka¿dej linijce jeden serwer. Pusta linijka oznacza HUB (system zwracaj¹cy najmniej obci¹¿ony serwer)."
			"<br/><b>Format</b> (zawartoœæ [...] jest opcjonalna):"
			"<br/><u>Adres</u>[:<u>port</u>]"
			"<br/><b>SSL</b>[ <u>Adres</u>[:<u>port</u>]] <i>(po³¹czenie szyfrowane)</i>"
			"<br/><br/>Aby wy³¹czyæ serwer dodaj <b>!</b> na pocz¹tku."
			AP_TIPRICH_WIDTH "300"
		);
		txt = SetActParam("Domyœlne", AP_ICO, inttostr(ICON_DEFAULT));
		UIActionAdd(CFG::group, IMIC_GG_DEFSERVERS, ACTT_BUTTON|ACTSC_INLINE, txt, 0, 0, 25);
		txt = SetActParam("Tylko SSL", AP_ICO, inttostr(ICON_SECURE));
		UIActionAdd(CFG::group, IMIC_GG_SERVERSSSLONLY, ACTT_BUTTON, txt, 0, 0, 25);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");

		UIActionAdd(CFG::group, 0, ACTT_GROUP, "");
		txt = SetActParam(AP_TIPRICH "<b>UWAGA!</b> Konto zostanie <u>bezpowrotnie</u> usuniête z serwera GG! Nie bêdziesz móg³ wiêcej korzystaæ z tego numeru!", AP_ICO, inttostr(ICON_WARNING));
		UIActionAdd(CFG::group, 0 ,ACTT_TIPBUTTON | ACTSC_INLINE,txt,0,30, 30);
		txt = SetActParam("Usuñ konto z serwera", AP_ICO, inttostr(ICON_ACCOUNTREMOVE));
		UIActionAdd(CFG::group, IMIC_GG_REMOVEACCOUNT ,ACTT_BUTTON,txt,0,170, 30);
		UIActionAdd(CFG::group, 0, ACTT_GROUPEND, "");*/
	}
}