#include "stdafx.h"
#include <konnekt\lib.h>

using namespace std;
using Stamina::inttostr;

#include "resource.h"
#include "gg_main.h"
using namespace Konnekt::GG;

bool GG::onRequest = false;
HANDLE GG::ggThread;
int GG::ggThreadId;
UINT_PTR GG::timer = 0;
struct gg_session *GG::sess = 0;
int GG::loop = 0;
bool GG::inAutoAway = false;
CStdString GG::lastServer = "-------";
CStdString GG::currentServer = "-------";

userListRequest GG::onUserListRequest = ulrNone;
CStdString GG::userListBuffer = "";

CStdString GG::curStatusInfo = "";
int GG::curStatus = 0;

tEventHandler GG::eventHandler;

int GG::sessionUsage=0;
// Sprawdzanie dostarczenia wiadomosci
CRITICAL_SECTION GG::msgSent_CS;
tMsgSent GG::msgSent;
CRITICAL_SECTION GG::searchMap_CS;
map <int, cGGSearch*> GG::searchMap;


int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved) {
	return 1;
}

int Init() {
	InitializeCriticalSection(&msgSent_CS);
	InitializeCriticalSection(&searchMap_CS);
	gg_debug_level = 255 & ~(GG_DEBUG_DUMP | GG_DEBUG_FUNCTION);
	//timer = CreateWaitableTimer(0,0,0);
	return 1;
}

int ISetCols() {
	SetColumn(DTCFG, CFG_GG_LOGIN, DT_CT_INT, 0, "GG/login");
	SetColumn(DTCFG, CFG_GG_PASS, DT_CT_PCHAR|DT_CF_CXOR|DT_CF_SECRET, 0, "GG/pass");
	SetColumn(DTCFG, CFG_GG_STATUS, DT_CT_INT, GG_STATUS_AVAIL, "GG/status");
	SetColumn(DTCFG, CFG_GG_STARTSTATUS, DT_CT_INT, 0, "GG/startStatus");
	SetColumn(DTCFG, CFG_GG_FRIENDSONLY, DT_CT_INT, 0, "GG/friendsOnly");

	SetColumn(DTCFG, CFG_GG_DESCR, DT_CT_PCHAR|DT_CF_CXOR, "", "GG/description");
	SetColumn(DTCFG, CFG_GG_SERVER, DT_CT_PCHAR|DT_CF_CXOR, GG_DEF_SERVERS, "GG/server");
	SetColumn(DTCFG, CFG_GG_TRAYMENU, DT_CT_INT, 0, "GG/trayMenu");
	SetColumn(DTCFG, CFG_GG_USESSL, DT_CT_INT, 0, "GG/useSSL");
	SetColumn(DTCFG, CFG_GG_DONTRESUMEDISCONNECTED, DT_CT_INT, 0, "GG/dontResumeDisconnected");	
	SetColumn(DTCFG, -1, DT_CT_INT, 0, "GG/imOmnix");

  return 1;
}

// *********************************************************
int IPrepare() {
	ICMessage(IMC_SETCONNECT, 1, 1);
	IconRegister(IML_ICO, UIIcon(IT_OVERLAY,NET_GG,0,0), Ctrl->hDll(), IDI_OVERLAY);
	IMessage(IMI_ICONRES, 0, 0, UIIcon(4,NET_GG,ST_CONNECTING,0), IDI_CONNECTING);
	IMessage(IMI_ICONRES, 0, 0, UIIcon(2,NET_GG,0,0), IDI_ONLINE);
	IMessage(IMI_ICONRES, 0, 0, UIIcon(IT_STATUS, NET_GG, ST_BLOCKING, 0), IDI_BLOCKING);
	IconRegister(IML_16, IDI_SERVER, Ctrl->hDll(), IDI_SERVER);
	IconRegister(IML_32, UIIcon(IT_LOGO,NET_GG,0,0), Ctrl->hDll(), IDI_SERVER);

	int status = GETINT(CFG_GG_STATUS);
	UIGroupInsert(IMIG_STATUS, IMIG_GGSTATUS, -1, 0, "GG", UIIcon(4,NET_GG,ST_OFFLINE,0));
	UIActionAdd(IMIG_GGSTATUS, IMIA_GGSTATUS_DESC, 0, "Opis", 0);

	UIActionAdd(IMIG_GGSTATUS, IMIG_GGSTATUS_SERVER, ACTR_INIT, "", IDI_SERVER);
	UIActionAdd(IMIG_GGSTATUS, IMIA_GGSTATUS_ONLINE, 0, "dostêpny", IMessage(IMI_ICONRES, 0, 0, UIIcon(4, NET_GG, ST_ONLINE, 0), IDI_ONLINE));
	UIActionAdd(IMIG_GGSTATUS, IMIA_GGSTATUS_AWAY, 0, "zaraz wracam", IMessage(IMI_ICONRES, 0, 0, UIIcon(4, NET_GG, ST_AWAY, 0), IDI_AWAY));
	UIActionAdd(IMIG_GGSTATUS, IMIA_GGSTATUS_HIDDEN, 0, "ukryty", IMessage(IMI_ICONRES, 0, 0, UIIcon(4, NET_GG, ST_HIDDEN, 0), IDI_HIDDEN));
	UIActionAdd(IMIG_GGSTATUS, IMIA_GGSTATUS_OFFLINE, 0, "niedostêpny", IMessage(IMI_ICONRES, 0, 0, UIIcon(4, NET_GG, ST_OFFLINE, 0), IDI_OFFLINE));

	//tray
	if (GETINT(CFG_GG_TRAYMENU)) {
		UIActionInsert(IMIG_TRAY, IMIA_GGSTATUS_DESC, 0, 0, "Opis", 0 );
		UIActionInsert(IMIG_TRAY, IMIA_GGSTATUS_ONLINE, 1, 0, "dostêpny", UIIcon(4, NET_GG, ST_ONLINE, 0));
		UIActionInsert(IMIG_TRAY, IMIA_GGSTATUS_AWAY, 2, 0, "zaraz wracam", UIIcon(4, NET_GG, ST_AWAY, 0));
		UIActionInsert(IMIG_TRAY, IMIA_GGSTATUS_HIDDEN, 3, 0, "ukryty", UIIcon(4, NET_GG, ST_HIDDEN, 0));
		UIActionInsert(IMIG_TRAY, IMIA_GGSTATUS_OFFLINE, 4 , ACTT_BARBREAK, "niedostêpny", UIIcon(4, NET_GG, ST_OFFLINE, 0));
	}

	UIActionAdd(IMIG_NFO_SAVE, IMIA_NFO_GGSAVE, ACTR_INIT, "W katalogu publicznym", UIIcon(IT_LOGO, NET_GG, 0, 0));
	UIActionAdd(IMIG_NFO_REFRESH, IMIA_NFO_GGREFRESH, ACTR_INIT, "Katalog publiczny", UIIcon(IT_LOGO, NET_GG, 0, 0));

	// Kontakty (g³ówny toolbar)
	UIGroupAdd(IMIG_MAIN_CNT, IMIG_MAIN_OPTIONS_LIST_GG,0, "GG", UIIcon(2,NET_GG,0,0));
	UIActionAdd(IMIG_MAIN_OPTIONS_LIST_GG, IMIA_LIST_GG_IMPORT, 0, "Importuj listê");
	UIActionAdd(IMIG_MAIN_OPTIONS_LIST_GG, IMIA_LIST_GG_EXPORT, 0, "Exportuj listê");
	UIActionAdd(IMIG_MAIN_OPTIONS_LIST_GG, IMIA_LIST_GG_CLEAR , 0, "Usuñ kontakty");
	UIActionAdd(IMIG_MAIN_OPTIONS_LIST_GG, IMIA_LIST_GG_REFRESH, 0, "Odœwie¿ (kp)");

	// Lista kontaktow
	UIActionAdd(IMIG_CNT, IMIA_GGHIDESTATUS, ACTR_INIT, "Ukryj status przed nim", 0);

	// Config
	UIGroupAdd(IMIG_CFG_USER, IMIG_GGCFG_USER,0,"GG",UIIcon(2,NET_GG,0,0)); {
		UIActionCfgAddPluginInfoBox2(IMIG_GGCFG_USER, 
				"<div>Wtyczka umo¿liwia komunikacjê przy pomocy najpopularniejszego protoko³u w Polsce."
				, "Wykorzystano bibliotekê <b>LibGadu</b> (http://toxygen.net/libgadu/)"
				"<br/>Strona domowa protoko³u GG - http://www.gadu-gadu.pl/"
				"<br/><br/>Copyright ©2003,2004 <b>Stamina</b>"
				, "res://dll/gglogo.ico", -3);

		CStdString txt;

		UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_GROUP,"Konto na serwerze GG"); {
			UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_COMMENT | ACTSC_INLINE,"Numerek GG",0,70);
			txt = ShowBits::checkBits(ShowBits::showTooltipsBeginner) ? "Tu wpisz swój numer konta GG, lub za³ó¿ nowe konto przyciskiem poni¿ej." : "";
			UIActionAdd(IMIG_GGCFG_USER, IMIB_CFG ,ACTT_EDIT | ACTSC_INLINE | ACTSC_INT,txt,CFG_GG_LOGIN,65);
			UIActionAdd(IMIG_GGCFG_USER, IMIB_CFG ,ACTT_PASSWORD | ACTSC_INLINE,"",CFG_GG_PASS,65);
			UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_COMMENT ,"Has³o",0,30);

			UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_SEPARATOR);

			txt = SetActParam("Za³ó¿ konto", AP_ICO, inttostr(ICON_ACCOUNTCREATE));
			UIActionAdd(IMIG_GGCFG_USER, IMIC_GG_ACCOUNT ,ACTT_BUTTON|ACTSC_INLINE|ACTSC_BOLD,txt,0,155, 30);
			txt = SetActParam("Importuj listê kontaktów", AP_ICO, inttostr(ICON_IMPORT));
			UIActionAdd(IMIG_GGCFG_USER, IMIA_LIST_GG_IMPORT ,ACTT_BUTTON|ACTSC_BOLD | ACTSC_FULLWIDTH,txt, 0, 180, 30);

			txt = SetActParam("Zmieñ has³o", AP_ICO, inttostr(ICON_CHANGEPASSWORD));
			UIActionAdd(IMIG_GGCFG_USER, IMIC_GG_NEWPASS ,ACTT_BUTTON|ACTSC_INLINE,txt,0,155, 30);
			txt = SetActParam("Przypomnij has³o", AP_ICO, inttostr(ICON_REMINDPASSWORD));
			UIActionAdd(IMIG_GGCFG_USER, IMIC_GG_REMINDPASS ,ACTT_BUTTON | ACTSC_FULLWIDTH,txt,0,180, 30);



		}UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_GROUPEND,"");
		UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_GROUP,"Ustawienia");{
			if (ShowBits::checkLevel(ShowBits::levelNormal)) {
				UIActionAdd(IMIG_GGCFG_USER, IMIB_CFG, ACTT_COMBO | ACTSCOMBO_LIST | ACTSC_INLINE
					, "Ostatni" CFGICO "#74" CFGVALUE "0\n"
						"Niedostêpny" CFGICO "0x40A00000" CFGVALUE "1\n"
						"Dostêpny" CFGICO "0x40A00400" CFGVALUE "2\n"
						"Zajêty" CFGICO "0x40A00410" CFGVALUE "3\n"
						"Ukryty" CFGICO "0x40A00420" CFGVALUE "20"
						AP_PARAMS AP_TIP "Status, który zostanie ustawiony po uruchomieniu programu"
					, CFG_GG_STARTSTATUS);

				UIActionAdd(IMIG_GGCFG_USER, 0, ACTT_COMMENT,"Status startowy");
				UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_CHECK,"U¿ywaj bezpiecznego po³¹czenia, jeœli to mo¿liwe",CFG_GG_USESSL);
			}

			UIActionAdd(IMIG_GGCFG_USER, IMIB_CFG, ACTT_CHECK,"Mój status widoczny tylko u znajomych z mojej listy",CFG_GG_FRIENDSONLY);

			if (ShowBits::checkLevel(ShowBits::levelAdvanced)) {
				UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_CHECK | ACTSC_NEEDRESTART,"Pe³ne menu w zasobniku systemowym" AP_TIP "Wszystkie statusy bêd¹ dostêpne bezpoœrednio w menu zasobnika (tray)",CFG_GG_TRAYMENU);
				UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_CHECK,"Nie ³¹cz ponownie je¿eli serwer zakoñczy po³¹czenie." AP_TIP "W³¹cz t¹ opcjê, je¿eli czêsto korzystasz z konta w ró¿nych miejscach. Zapobiega cyklicznemu \"prze³¹czaniu\" pomiêdzy w³¹czonymi programami.",CFG_GG_DONTRESUMEDISCONNECTED);
			}
		}UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_GROUPEND,"");
		
		if (ShowBits::checkLevel(ShowBits::levelAdvanced)) {
			UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_GROUP,"Serwery");
			UIActionAdd(IMIG_GGCFG_USER, IMIB_CFG ,ACTT_TEXT | ACTSC_INLINE,"" CFGTIP "Je¿eli zostawisz to pole puste - zostanie u¿yty serwer wskazany przez hub GG.",CFG_GG_SERVER, 150);
			UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_TIPBUTTON | ACTSC_INLINE, AP_TIPRICH  "W ka¿dej linijce jeden serwer. Pusta linijka oznacza HUB (system zwracaj¹cy najmniej obci¹¿ony serwer)."
				"<br/><b>Format</b> (zawartoœæ [...] jest opcjonalna):"
				"<br/><u>Adres</u>[:<u>port</u>]"
				"<br/><b>SSL</b>[ <u>Adres</u>[:<u>port</u>]] <i>(po³¹czenie szyfrowane)</i>"
				"<br/><br/>Aby wy³¹czyæ serwer dodaj <b>!</b> na pocz¹tku."
				AP_TIPRICH_WIDTH "300");
			txt = SetActParam("Domyœlne", AP_ICO, inttostr(ICON_DEFAULT));
			UIActionAdd(IMIG_GGCFG_USER, IMIC_GG_DEFSERVERS ,ACTT_BUTTON | ACTSC_INLINE,txt, 0, 0, 25);
			txt = SetActParam("Tylko SSL", AP_ICO, inttostr(ICON_SECURE));
			UIActionAdd(IMIG_GGCFG_USER, IMIC_GG_SERVERSSSLONLY ,ACTT_BUTTON,txt, 0, 0, 25);
			UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_GROUPEND,"");
		}
		if (ShowBits::checkLevel(ShowBits::levelNormal)) {
			UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_GROUP,"");{
				txt = SetActParam(AP_TIPRICH "<b>UWAGA!</b> Konto zostanie <u>bezpowrotnie</u> usuniête z serwera GG! Nie bêdziesz móg³ wiêcej korzystaæ z tego numeru!", AP_ICO, inttostr(ICON_WARNING));
				UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_TIPBUTTON | ACTSC_INLINE,txt,0,30, 30);
				txt = SetActParam("Usuñ konto z serwera", AP_ICO, inttostr(ICON_ACCOUNTREMOVE));
				UIActionAdd(IMIG_GGCFG_USER, IMIC_GG_REMOVEACCOUNT ,ACTT_BUTTON,txt,0,170, 30);
			}UIActionAdd(IMIG_GGCFG_USER, 0 ,ACTT_GROUPEND,"");
		}
	}
	return 1;
}

// status == -1  -> pobiera aktualny status
// setdesc == 1  -> automat
// setdesc == 2  -> auto-away
// setdesc == 3  -> reczny z CFG_GG_DESCR
// setdesc == -1 -> pobiera ten, ktory jest teraz ustawiony...

int IStart() {
  IMLOG("- LibGaduw32 v: %s", gg_libgadu_version());
  ggThread=0;
  PlugStatusChange(ST_OFFLINE, "");
  // Ustawiamy status startowy
  if (GETINT(CFG_GG_STARTSTATUS))
	  SETINT(CFG_GG_STATUS, GETINT(CFG_GG_STARTSTATUS));
  return 1;
}

int IEnd() {
  disconnect();
//  if (sess) gg_free_session(sess);
//  if (ggThread) TerminateThread(ggThread, 0);
//  Sleep(1000);
  int i = 0;
  IMLOG("# Czekam na koniec w¹tku GG");
  while (ggThread && i < 1000) {
//    IMLOG("#Czekam na koniec w¹tku GG");
    Ctrl->Sleep(10);
    i++;
  }
  IMLOG("# Zakoñczony po %d", i);
//  CloseHandle(timer);
  DeleteCriticalSection(&msgSent_CS);
  DeleteCriticalSection(&searchMap_CS);
  return 1;
}

int ActionProc(sUIActionNotify_base * anBase) {
  sUIActionNotify_2params * an = static_cast<sUIActionNotify_2params*>(anBase);
//  int uiPos=Cnt.find(act->cnt);
  string str, str1, str2;
  int i, a;
  sUIAction act2;
  sDIALOG_long sdl;
  switch (an->act.id) {
      case IMIG_GGSTATUS_SERVER:
		  if (an->code == ACTN_ACTION) {
			  GG::chooseServer();	
		  } else
          if (an->code == ACTN_CREATE) {
              sUIActionInfo ai(an->act);
              ai.mask = UIAIM_STATUS;
              ai.status = 0;//ggThread?0:ACTS_HIDDEN;
              ai.statusMask = ACTS_HIDDEN;
              CStdString server;
              ai.mask |= UIAIM_TXT;
              if (ggThread) {
                  if (currentServer=="" || currentServer=="SSL") {
                      server = "H:";
                      in_addr in;
					  if (sess) {
					      in.S_un.S_addr = sess->server_addr;
						  server += inet_ntoa(in);
					  }
					  if (currentServer=="SSL") {server+=" SSL";}
                  } else server = currentServer;
//				  if (sess && sess->ssl) {server+=" SSL";}
				  if (!sess) server = "³¹czê z " + server;
				  ai.txt = (char*)server.c_str();
			  } else {ai.txt = "wybierz serwer";}
              UIActionSet(ai);
          }
          break;
     case IMIA_GGSTATUS_ONLINE:
        ACTIONONLY(an);
        SETINT(CFG_GG_STATUS, GG_STATUS_AVAIL);
        if (!ggThread) connect();
          else setStatus(GG_STATUS_AVAIL,1);
        break;
     case IMIA_GGSTATUS_OFFLINE:
        ACTIONONLY(an);
        onRequest = true;
        disconnect();
        break;

     case IMIA_GGSTATUS_AWAY:
        ACTIONONLY(an);
        SETINT(CFG_GG_STATUS, GG_STATUS_BUSY);
        if (!ggThread) connect();
          else setStatus(GG_STATUS_BUSY,1);
        break;

     case IMIA_GGSTATUS_HIDDEN:
        ACTIONONLY(an);
        SETINT(CFG_GG_STATUS, GG_STATUS_INVISIBLE);
        if (!ggThread) connect();
          else setStatus(GG_STATUS_INVISIBLE,1);
        break;
     case IMIA_GGSTATUS_DESC:
        ACTIONONLY(an);
        setStatusDesc();
        break;
     case IMIC_GG_ACCOUNT:
        ACTIONONLY(an);
		CloseHandle((HANDLE)Ctrl->BeginThread("NewAccount", 0, 0, dlgAccount, 0, 0, 0));
        break;
     case IMIC_GG_REMOVEACCOUNT:
        ACTIONONLY(an);
		CloseHandle((HANDLE)Ctrl->BeginThread("RemoveAccount", 0, 0, dlgRemoveAccount, 0, 0, 0));
        break;
     case IMIC_GG_NEWPASS:
        ACTIONONLY(an);
		CloseHandle((HANDLE)Ctrl->BeginThread("NewPass", 0, 0, dlgNewPass, 0, 0, 0));
        break;
     case IMIC_GG_REMINDPASS:
        ACTIONONLY(an);
        CloseHandle((HANDLE)Ctrl->BeginThread("RemindPass", 0, 0, dlgRemindPass, 0, 0, 0));
        break;


     case IMIC_GG_DEFSERVERS:
         ACTIONONLY(an);
         UIActionCfgSetValue(sUIAction(an->act.parent,CFG_GG_SERVER | IMIB_CFG), GG_DEF_SERVERS );
         break;
	 case IMIC_GG_SERVERSSSLONLY:
         ACTIONONLY(an);
         UIActionCfgSetValue(sUIAction(an->act.parent,CFG_GG_SERVER | IMIB_CFG), "SSL");
		 UIActionCfgSetValue(sUIAction(an->act.parent,CFG_GG_USESSL | IMIB_CFG), "1");
         break;

     case IMIA_LIST_GG_CLEAR:
        ACTIONONLY(an);
        if (IMessage(IMI_CONFIRM, 0, 0, (int)"Wszystkie kontakty sieci G*duG*du zostan¹ usuniête!\nKontynuowaæ?",MB_TASKMODAL|MB_OKCANCEL)!=IDOK) return 0;
        a = ICMessage(IMC_CNT_COUNT);
        for (i = 1; i < a; i++) {
          if (GETCNTI(i, CNT_NET) == NET_GG){
            ICMessage(IMC_CNT_REMOVE, i);
            a--;
            i--;
          }
        }
        ICMessage(IMI_REFRESH_LST);
        break;

     case IMIA_LIST_GG_EXPORT:
        ACTIONONLY(an);
		dlgListExport();
        break;

     case IMIA_LIST_GG_IMPORT:
        ACTIONONLY(an);
        dlgListImport();
        break;
     case IMIA_LIST_GG_REFRESH:
        ACTIONONLY(an);
        CloseHandle((HANDLE)Ctrl->BeginThread("RefreshList", 0, 0, dlgListRefresh));
        break;

// ----------------------- CNT
     case IMIA_GGHIDESTATUS:
        if (an->code == ACTN_ACTION) {
          if ((signed int)an->act.cnt < 0) return 0;
          if (!GETINT(CFG_GG_FRIENDSONLY)) {
              int r = IMessage(&sIMessage_msgBox(IMI_CONFIRM 
                 , "Aby ukryæ swój status musisz mieæ w³¹czone\r\n[Tylko dla znajomych]. W³¹czyæ je za Ciebie?", 0, MB_YESNOCANCEL));
              if (r==IDYES) SETINT(CFG_GG_FRIENDSONLY, 1);
              else if (r==IDCANCEL) return 0;
          }
          bool hide = !(GETCNTI(an->act.cnt, CNT_STATUS) & ST_HIDEMYSTATUS);
		  
          SETCNTI(an->act.cnt, CNT_STATUS, hide?ST_HIDEMYSTATUS : 0, ST_HIDEMYSTATUS);
          ICMessage(IMI_REFRESH_CNT, an->act.cnt);
          if (sess && !(GETCNTI(an->act.cnt, CNT_STATUS) & ST_IGNORED)) {
              int Uid = atoi(GETCNTC(an->act.cnt, CNT_UID));
              if (hide)
                  gg_remove_notify_ex(sess, Uid, GG_USER_NORMAL);
              gg_add_notify_ex(sess, Uid, userType(an->act.cnt));
          }
        }
        else if (an->code == ACTN_CREATE) {
            if ((signed int)an->act.cnt < 0 || GETCNTI(an->act.cnt,CNT_NET)!=NET_GG) {
              UIActionSetStatus(an->act, ACTS_HIDDEN, ACTS_HIDDEN);
              return 0;
            }
            if (GETCNTI(an->act.cnt, CNT_STATUS)& ST_HIDEMYSTATUS) {
              UIActionSetStatus(an->act, ACTS_CHECKED, ACTS_CHECKED|ACTS_HIDDEN);
              //SETINT(CFG_GG_FRIENDSONLY, 1);
              return 0;
            }
            UIActionSetStatus(an->act, 0, ACTS_CHECKED|ACTS_HIDDEN);
        }
        break;
	 case IMIA_NFO_GGREFRESH:
		 if (anBase->code == ACTN_CREATE) {
			 UIActionSetStatus(anBase->act, (ggThread && (Ctrl->DTgetPos(DTCNT, anBase->act.cnt)==0 || !strcmp(UIActionCfgGetValue(sUIAction(IMIG_NFO_DETAILS, IMIB_CNT | CNT_NET, anBase->act.cnt),0,0), "10"))) ? 0 : -1, ACTS_HIDDEN );
		 } else if (anBase->code == ACTN_ACTION) {
			 IMessageDirect(IM_CNT_DOWNLOAD, Ctrl->ID(), anBase->act.cnt, 1);
			 ICMessage(IMI_CNT_DETAILS_SUMMARIZE, anBase->act.cnt);
		 }
		 break;
	 case IMIA_NFO_GGSAVE:
		 if (anBase->code == ACTN_CREATE) {
			 UIActionSetStatus(anBase->act, ggThread && Ctrl->DTgetPos(DTCNT,anBase->act.cnt)==0 ? 0 : -1, ACTS_HIDDEN );
		 } else if (anBase->code == ACTN_ACTION) {
			 IMessageDirect(IM_CNT_UPLOAD, Ctrl->ID(), anBase->act.cnt, 1);
		 }
		 break;

  }
  return 0;
}

//---------------------------------------------------------------------------

int __stdcall IMessageProc(sIMessage_base* msgBase) {
	sIMessage_2params* msg = static_cast<sIMessage_2params*>(msgBase);
	int r;
	sDIALOG sd;
	cMessage* m;
	switch (msg->id) {
		case IM_PLUG_NET:          return NET_GG;
		case IM_PLUG_TYPE:         return IMT_MESSAGE|IMT_PROTOCOL|IMT_CONTACT|IMT_CONFIG|IMT_UI|IMT_NET|IMT_NETSEARCH|IMT_MESSAGEACK|IMT_MSGUI|IMT_NETUID;
		case IM_PLUG_VERSION:      return (int)"";
		case IM_PLUG_SDKVERSION:   return KONNEKT_SDK_V;  // Ta linijka jest wymagana!
		case IM_PLUG_SIG:          return (int)"GG";
		case IM_PLUG_CORE_V:       return (int)"W98";
		case IM_PLUG_UI_V:         return 0;
		case IM_PLUG_NAME:         return (int)"GG";
		case IM_PLUG_NETNAME:      return (int)"Gadu-Gadu™";
		case IM_PLUG_NETSHORTNAME: return (int)"GG";
		case IM_PLUG_UIDNAME:      return (int)"#";
    case IM_PLUG_INIT:         Ctrl = (cCtrl*)msg->p1;Plug_Init(msg->p1,msg->p2); return Init();
    case IM_PLUG_DEINIT:       Plug_Deinit(msg->p1,msg->p2); return 1;
    case IM_PLUG_PRIORITY:     return PLUGP_LOW;
    case IM_SETCOLS:           return ISetCols();
    case IM_UI_PREPARE:        return IPrepare();
    case IM_START:             return IStart();
    case IM_END:               return IEnd();
    case IM_DISCONNECT:        return disconnect();
		case IM_UIACTION:          return ActionProc((sUIActionNotify_base*)msg->p1);
    
    case IM_GET_STATUS: return curStatus;
    case IM_GET_STATUSINFO: {
			char * status = (char*)Ctrl->GetTempBuffer(curStatusInfo.size() + 1);
			strcpy(status, curStatusInfo); 
			return (int)status;
		}
		case IM_GET_UID: return (int)GETSTR(CFG_GG_LOGIN);
		case IM_MSG_RCV: {
			m = (cMessage*)msg->p1;
			switch (m->type) {
				case MT_MESSAGE: {
					if (m->flag & MF_SEND) {
						return IM_MSG_ok;
					} /*else {
						m->action = sUIAction(IMIG_CNT, IMIA_CNT_MSGOPEN);
						m->notify = UIIcon(5,0,MT_MESSAGE,0);
					}
					return IM_MSG_RCV_ok;
				} case MT_SERVEREVENT: {
					IMLOG("SERVEREVENT");
					m->action = sUIAction(IMIG_EVENT, IMIA_EVENT_SERVER);
					m->notify = UIIcon(5,0,MT_SERVEREVENT,0);
					return IM_MSG_RCV_ok;
				} case MT_QUICKEVENT: {
					IMessage(IMI_MSG_OPEN, 0,0,msg->p1);
					return IM_MSG_RCV_delete;
				}*/
			}
			return 0;
		}
		case IM_MSG_CHARLIMIT: return 2000;
    case IM_MSG_SEND: {
			cMessage m2;
			char * errMsg = "Nie mog³em wys³aæ, spróbujê póŸniej";
			m = (cMessage*)msg->p1;
			if (m->type == MT_MESSAGE && atoi(m->toUid)) {
				if (!(m->flag & MF_SEND)) {
					IMDEBUG(DBG_ERROR, "- MSG_SEND_RECEIVED c=%s, t=%x, sndr=%x", m->toUid, time(0), msg->sender);
					char* errMsg = "Nie mogê wys³aæ odebranej wiadomoœci (b³¹d wtyczki!)";
					goto some_err;
				}

				if (m->flag & MF_AUTOMATED && curStatus == ST_HIDDEN) return 0;
				if (!sess) {
					r=-1;
					goto some_err;
				}

				IMDEBUG(DBG_LOG, "- MSG_SEND: c=%s, t=%x, sndr=%x", m->toUid, time(0), msg->sender);

				if (m->flag & MF_HTML) {
					CStdString body = m->body;
					int format_length = 20000;
					void * formats = malloc(format_length);
					body = GG::htmlToMsg(body, formats, format_length);
					r = gg_send_message_richtext(sess, GG_CLASS_CHAT, atoi(m->toUid), (unsigned char*)body.c_str(), (unsigned char*)formats, format_length);
					free(formats);
				} else {
					r = gg_send_message(sess, GG_CLASS_CHAT, atoi(m->toUid), (unsigned char*)m->body);
				}

				if (r != -1) {
					cMsgSent ms;
					ms.msgID = m->id;
					ms.digest = string(m->body).substr(0,30);
					ms.sentTime = time(0);
					ms.Uid = atoi(m->toUid);
					EnterCriticalSection(&msgSent_CS);
					msgSent[r]=ms;
					LeaveCriticalSection(&msgSent_CS);
				}

        some_err:
					if (r==-1) {
						if (m->flag & MF_NOEVENTS) return 0;
						m2.net = NET_GG;
						m2.type = MT_QUICKEVENT;
						m2.fromUid = m->toUid;
						m2.toUid = "";
						m2.body = errMsg;
						m2.ext = "";
						m2.flag = MF_HANDLEDBYUI;
						m2.action = NOACTION;
						m2.notify = 0;
						IMessage(IMC_NEWMESSAGE, 0,0,(int)&m2,0);
						m->flag |= MF_NOEVENTS;
						return 0;
					} else {
						return IM_MSG_delete;
					}
			}
		} case IM_CNT_DOWNLOAD: {
			if ((Ctrl->DTgetPos(DTCNT, msg->p1) && GETCNTI(msg->p1,CNT_NET) != NET_GG) || !GG::check(1, 1, 0, 1))
				break;
			Ctrl->BeginThreadAndWait("CntDownload", 0, 0, (cCtrl::fBeginThread) doCntDownload, (void*)msg);
			IMDEBUG(DBG_FUNC, "IM_CNT_DOWNLOAD finished");
			return 0;
    } case IM_CNT_UPLOAD: {
			if (Ctrl->DTgetPos(DTCNT, msg->p1) || !GG::check(0, 1, 0, 0)) {
				break;
			}
      Ctrl->BeginThreadAndWait("CntUpload", 0, 0, (cCtrl::fBeginThread) doCntUpload, (void*)msg);
      return 0;
    } case IM_CNT_CHANGED: {
			sIMessage_CntChanged icc(msg);
			if ((icc._changed.net || icc._changed.uid) && icc._oldNet == NET_GG) {
				gg_remove_notify_ex(sess, atoi(icc._oldUID), ICMessage(IMC_IGN_FIND, icc._oldNet, (int)icc._oldUID) ? GG_USER_BLOCKED : GG_USER_NORMAL);
			}
		} case IM_CNT_ADD: {  // kontynuacja !
			if (sess && GETCNTI(msg->p1,CNT_NET) == NET_GG && !(GETCNTI(msg->p1, CNT_STATUS) & (ST_IGNORED|ST_HIDEMYSTATUS|ST_NOTINLIST))
				/*&& !(GETCNTI(0,CNT_STATUS)&ST_HIDEMYSTATUS)*/) {
				gg_remove_notify(sess, atoi((char *)GETCNTC(msg->p1,CNT_UID)));
				gg_add_notify_ex(sess, atoi((char *)GETCNTC(msg->p1,CNT_UID)), GG::userType(msg->p1));
			}
			return 0;
    } case IM_CNT_REMOVE: {
			gg_remove_notify_ex(sess, atoi((char*)GETCNTC(msg->p1, CNT_UID)), GG::userType(msg->p1));
			return 0;
    } case IM_IGN_CHANGED: {
			if (abs(msg->p1) != NET_GG) return 0;
			if (msg->p1 > 0) {
				gg_add_notify_ex(sess, atoi((char *)msg->p2),  GG_USER_BLOCKED);
			} else {
				gg_remove_notify_ex(sess, atoi((char *)msg->p2),  GG_USER_BLOCKED);
				int cnt = ICMessage(IMC_CNT_FIND, msg->p1, msg->p2);
				gg_add_notify_ex(sess, atoi((char *)msg->p2),  (cnt>0)?GG::userType(cnt):GG_USER_NORMAL);
			}
			return 0;
    } case IM_AWAY: {
			if (!ggThread || (((GETINT(CFG_GG_STATUS)&0xFF)!=GG_STATUS_AVAIL_DESCR) && ((GETINT(CFG_GG_STATUS)&0xFF)!=GG_STATUS_AVAIL))) return 0;
			inAutoAway = true;
			setStatus(GG_STATUS_BUSY,2);
			return 0;
    } case IM_BACK: {
			if (!ggThread || !inAutoAway) return 0;
			inAutoAway = false;
			setStatus(GETINT(CFG_GG_STATUS),1);
			return 0;
    } case IM_CONNECT: if (!ggThread) connect(); return 1;
    case IM_ISCONNECTED: return (ggThread!=0 && sess != 0);
    case IM_CHANGESTATUS: {
			if (msg->p2) SETSTR(CFG_GG_DESCR, (char*)msg->p2);
			int status = msg->p1;
			switch (status) {
				case ST_OFFLINE: status = GG_STATUS_NOT_AVAIL; break;
				case ST_ONLINE: status = GG_STATUS_AVAIL; break;
				case ST_AWAY: status = GG_STATUS_BUSY; break;
				case ST_HIDDEN: status = GG_STATUS_INVISIBLE; break;
				default: status = -1; break;     
			}
			if (status != -1 && status != GG_STATUS_NOT_AVAIL) SETINT(CFG_GG_STATUS, status);
			if (status == GG_STATUS_NOT_AVAIL) {
				onRequest = true;
				disconnect();
			} else {
				if (!ggThread) connect();
				else setStatus(status, msg->p2?3:1);
			}
			break;
		} case IM_CNT_SEARCH:
			if (!GG::check(1, 0, 1, 1)) return 0;
			Ctrl->BeginThreadAndWait("CntSearch", 0, 0, (cCtrl::fBeginThread) doCntSearch, (void*)msg->p1);
			return 1;
		} case IM_GG_REGISTERHANDLER: {
			eventHandler[msg->sender] = msg->p1;
			return 1;
		} case IM_GG_GETSESSION: {
			if (sess) {
				sessionUsage++;
			}
			return (int)sess;
		} case IM_GG_RELEASESESSION: {
			if (sessionUsage) {
				sessionUsage--;
			}
			return 0;
		} case IM_GG_HTMLTOMSG: {
			sIMessage_GGHtmlFormat* hf = (sIMessage_GGHtmlFormat*) msgBase;
			std::string body = GG::htmlToMsg(hf->body, hf->format, hf->formatSize);
			hf->result = (char*) Ctrl->GetTempBuffer(body.length() + 1);
			strcpy((char*)hf->result, body.c_str());
			return 1;
		} case IM_GG_MSGTOHTML: {
			sIMessage_GGHtmlFormat* hf = (sIMessage_GGHtmlFormat*) msgBase;
			std::string body = GG::msgToHtml(hf->body, hf->format, hf->formatSize);
			hf->result = (char*) Ctrl->GetTempBuffer(body.length() + 1);
			strcpy((char*)hf->result, body.c_str());
			return 1;
		}
	}
	if (Ctrl) Ctrl->setError(IMERROR_NORESULT);
	return 0;
}