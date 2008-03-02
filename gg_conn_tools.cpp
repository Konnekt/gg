#include "stdafx.h"
#include "gg_main.h"
using namespace Konnekt::GG;

void GG::setProxy() {
	static char p_host[100];
	static char p_user[100];
	static char p_pass[100];
	gg_proxy_http_only = GETINT(CFG_PROXY_HTTP_ONLY);
	gg_proxy_enabled = GETINT(CFG_PROXY);
	if (gg_proxy_enabled) {
		strcpy(p_host, GETSTR(CFG_PROXY_HOST));
		gg_proxy_host = p_host;
		gg_proxy_port = GETINT(CFG_PROXY_PORT);
	} else {
		gg_proxy_host = 0;
		gg_proxy_port = 0;
	}
	if (gg_proxy_enabled && GETINT(CFG_PROXY_AUTH)) {
		strcpy(p_user, (char*)GETSTR(CFG_PROXY_LOGIN));
		gg_proxy_username = p_user;
		strcpy(p_pass, (char*)GETSTR(CFG_PROXY_PASS));
		gg_proxy_password = p_pass;
	} else {
		gg_proxy_username = 0;
		gg_proxy_password = 0;
	}
}

void GG::setStatus(int status, int setdesc, gg_login_params* lp) {
	if (!lp && !ggThread) return;
	if (status == -1) status = GETINT(CFG_GG_STATUS);
	status &= 0xFF;
	IMLOG("setStatus st=%x d=%d lp=%x", status, setdesc, lp);
	if (!lp && (status) != GG_STATUS_NOT_AVAIL && ggThread && !sess) return;
	static CStdString desc = "";
	if (setdesc!=-1) {
		if (setdesc!=3 && status!=-1 && GETINT(CFG_DESCR_CLEARMANUAL) && status!=GG_STATUS_NOT_AVAIL) {
			SETSTR(CFG_GG_DESCR,"");
		}
		if (*GETSTR(CFG_GG_DESCR)) {
			if (GETINT(CFG_DESCR_USEMANUAL)) setdesc=3;
		} else if (setdesc==3) setdesc=1;
		if (setdesc == 1) {
			switch (status) {
				case GG_STATUS_AVAIL: desc = GETSTR(CFG_DESCR_ONLINE); break;
				case GG_STATUS_NOT_AVAIL: desc = GETSTR(CFG_DESCR_OFFLINE); break;
				case GG_STATUS_BUSY: desc = GETSTR(CFG_DESCR_AWAY); break;
				case GG_STATUS_INVISIBLE: desc = GETSTR(CFG_DESCR_HIDDEN); break;
			}
			if (desc.empty()) desc = GETSTR(CFG_GG_DESCR);
			//SETSTR(CFG_DESCR, desc);
		} else if (setdesc == 2) {
			if (status == GG_STATUS_BUSY) desc = GETSTR(CFG_DESCR_AAWAY);
		} else if (setdesc == 3) {
			desc = GETSTR(CFG_GG_DESCR);
		}
	}

	if (!lp || lp==(gg_login_params*)-1) {
		if (desc.empty()) desc = GETSTR(CFG_GG_DESCR);
		//if (!GETINT(CFG_DESCR_KEEP)) SETSTR(CFG_GG_DESCR, desc.c_str());
		CStdString shortDesc = CStdString(desc.substr(0,15));
		shortDesc.Replace("&", "&&");
		UIActionSetText(sUIAction(IMIG_TRAY, IMIA_GGSTATUS_DESC), ("Opis: " + shortDesc).c_str());
		UIActionSetText(sUIAction(IMIG_GGSTATUS, IMIA_GGSTATUS_DESC), ("Opis: " + shortDesc).c_str());
		curStatus = status==GG_STATUS_AVAIL? ST_ONLINE:
			status == GG_STATUS_BUSY ? ST_AWAY:
			status == GG_STATUS_INVISIBLE ? ST_HIDDEN:
			ST_OFFLINE;
		curStatusInfo = desc;
		PlugStatusChange(curStatus, curStatusInfo);
	}
	if (lp == (gg_login_params*)-1) return; // Ustawia TYLKO status dla rdzenia
	if (status!=GG_STATUS_NOT_AVAIL)
		status |= (GETINT(CFG_GG_FRIENDSONLY) ? GG_STATUS_FRIENDS_MASK : 0);
	if (!desc.empty()) {
		int mask = status & 0xFF00;
		status &=0xFF;
		if (status == GG_STATUS_NOT_AVAIL)
			status = GG_STATUS_NOT_AVAIL_DESCR;
		else
			status += 2;
		status |= mask;
	}

	if (lp) { // Nie ustawia statusu, tylko zapisuje go w login params
		lp->status=status;
		lp->status_descr=(char*)desc.c_str();
	} else {
		if (!desc.empty())
			gg_change_status_descr(sess, status, desc.c_str());
		else
			gg_change_status(sess, status);
	}
}

void GG::setStatusDesc() {
	//if (!ggThread) return;
	sDIALOG_enter sd;
	sd.title = "status opisowy";
	sd.id = "gg_status_desc";
	sd.value = (char*)GETSTR(CFG_GG_DESCR);
	sd.info = "Wpisz treœæ opisu (najpierw ustaw status!).";
	sd.maxLength = GG_STATUS_DESCR_MAXSIZE;
	if (!IMessage(IMI_DLGENTER, 0, 0, (int)&sd)) return;
	SETSTR(CFG_GG_DESCR, sd.value);
	setStatus(-1, 3);
}


void GG::chooseServer() {
	sDIALOG_choose sdc;
	CStdString servers = GETSTR(CFG_GG_SERVER);
	servers.Replace("\r", "");
	if (servers.empty())
		ICMessage(IMI_ERROR, (int)"Nie masz wybranych ¿adnych serwerów!");
	servers+="\n";
	CStdString items = "";
	sdc.title = "Wybierz serwer GG";
	sdc.info = "Od wybranego serwera rozpocznie siê nastêpna próba ³¹czenia.";
	size_t start = 0;
	size_t end;
	//bool hubUsed = false;
	vector <int> pos;
	while (start < servers.size() && (end = servers.find('\n', start)) != -1) {
		if (servers[start] != '!') {
			if (!items.empty()) items+="\n";
			if (servers[start]=='\n')
				items+="HUB";
			else
				items+=servers.substr(start, end - start);
			pos.push_back(start);
		}
		start = end + 1;
	}
	start = ("\n" + servers).find("\n" + lastServer + "\n");

	// Znajdujemy aktualny serwer...
	if (start != -1) {
		for (unsigned int i = 0; i<pos.size(); i++) {
			if (pos[i] == start) {
				sdc.def = i+1;
				break;
			}
		}
	}
	sdc.items = (char*)items.c_str();
	int r = ICMessage(IMI_DLGBUTTONS, (int)&sdc);
	if (!r) return;
	lastServer = servers.substr(pos[r - 1], servers.find('\n', pos[r - 1]) - pos[r - 1]);
}

bool __stdcall GG::disconnectDialogCB(sDIALOG_long*sd) {
	IMLOG("* Aktualny Socket w GG - %x @ %x", gg_thread_socket(sd->threadId,0), sd->threadId);
	if (gg_thread_socket(sd->threadId, 0)) {
		shutdown(gg_thread_socket(sd->threadId, 0), 0xFFFFFFFF);
		if (!gg_thread_socket(sd->threadId, -1))
			IMLOG("! closesocket.Error WSA = %d", WSAGetLastError());
	}
	return true;
}

bool __stdcall GG::cancelDialogCB(sDIALOG_long* sd) {
	sd->cancel = true;
	return true;
}

/*class timeoutDialogParam {
public:
	timeoutDialogParam(SOCKET s) {
		this->event = WSACreateEvent();
		this->socket = s;
		IMDEBUG(DBG_DEBUG, "New timeout param %x", this->socket);
				ioctlsocket(s, FIONBIO, (u_long*)1);
				WSAEventSelect(this->socket, event, FD_READ | FD_CLOSE);
				ioctlsocket(s, FIONBIO, 0);
	}
	~timeoutDialogParam() {
		WSACloseEvent(event);
	}
	SOCKET socket;
	WSAEVENT event;
};*/

bool __stdcall GG::timeoutDialogSimpleCB(int type, sDIALOG_long*sd) {
	switch (type) {
		case TIMEOUTT_START: break;
		case TIMEOUTT_END: break;
		case TIMEOUTT_TIMEOUT: {
			sd->cancel = true;
			ICMessage((sd->flag & DLONG_NODLG)?IMC_LOG:IMI_ERROR, (int)"Niestety, serwer nie odpowiada.",0);
			return true;
		} case TIMEOUTT_CHECK: return true;
	}
	return true;
}


bool __stdcall GG::timeoutDialogCB(int type, sDIALOG_long*sd) {
	switch (type) {
		case TIMEOUTT_START: break;
		case TIMEOUTT_END: {
			//if (sd->timeoutParam) { delete (timeoutDialogParam*) sd->timeoutParam; }
			//sd->timeoutParam=0; 
			//ioctlsocket(gg_thread_socket(sd->threadId,0), FIONBIO, 0);
			break;
		} case TIMEOUTT_TIMEOUT: {
			disconnectDialogCB(sd);
			ICMessage((sd->flag & DLONG_NODLG)?IMC_LOG:IMI_ERROR, (int)"Up³yn¹³ limit czasu po³¹czenia.",0);
			return true;
		} case TIMEOUTT_CHECK: {
			/*HACK: Czeka na dodanie locków do libgadu*/
			/*gg_thread_socket_lock();
			gg_thread* info = gg_thread_find(sd->threadId, 0, 0);
			if (!info) {
				gg_thread_socket_unlock();
				return 0;
			}
			int r=WSAWaitForMultipleEvents(1, &info->event, 0, 0, 0);

			gg_thread_socket_unlock();
			return r == WSA_WAIT_TIMEOUT;*/
		}
		return 0; /*TODO: Wymyœleæ coœ na timeout!*/
	}
	return true;
}