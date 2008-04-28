#include "stdafx.h"
#include "GG.h"
#include "GGSession.h"

GGSession::GGSession(ThreadRunner& threads) :
	_threads(threads), _session(0), _thread(0), _connected(false), _connecting(false), _status(ST_OFFLINE), _statusDescription("") { }

void GGSession::setProxy(bool enabled, bool httpOnly, string host, int port, string login, string password) {
	static char hostBuff[100];
	static char loginBuff[100];
	static char passwordBuff[100];

	gg_proxy_http_only = httpOnly;
	gg_proxy_enabled = enabled;

	if (gg_proxy_enabled) {
		gg_proxy_host = strcpy(hostBuff, host.c_str());
		gg_proxy_port = port;
		if (!login.empty() || !password.empty()) {
			gg_proxy_username = strcpy(loginBuff, login.c_str());
			gg_proxy_password = strcpy(passwordBuff, password.c_str());
		} else {
			gg_proxy_username = 0;
			gg_proxy_password = 0;
		}
	} else {
		gg_proxy_host = 0;
		gg_proxy_port = 0;
	}
}

void GGSession::connect(string login, string password, ggHandler handler, tStatus status, string statusDescription, bool friendsOnly) {
	if (!isConnected() && !isConnecting() && status != ST_OFFLINE) {
		_connecting = true;
		_thread = _threads.runEx(connectProc, (void*)(new ConnectInfo(login, password, handler, status, statusDescription, friendsOnly, this)), "Connect");
	}
}

void GGSession::stopConnecting(bool quick, unsigned timeout, bool terminate) {
	_stopConnecting = true;
	if (!quick && WaitForSingleObject(_thread, 500) == WAIT_OBJECT_0) {
		CloseHandle(_thread);
		return;
	}
	TerminateThread(_thread, 0);
	CloseHandle(_thread);
}

void GGSession::setStatus(tStatus status, const char* description) {
	if (!_connected && !_connecting && status != ST_OFFLINE) {
		return;
	} else if (_connecting && status == ST_OFFLINE) {
		_stopConnecting = true;
		stopConnecting();
		return;
	} else if (status == ST_OFFLINE) {
		return disconnect(description);
	}

	string setDescription = description ? description : _statusDescription.a_str();
	int setStatus = status == -1 ? convertKStatus(_status, setDescription) : convertKStatus(status, setDescription);
	PlugStatusChange(status == -1 ? _status : status, setDescription.c_str());
	gg_change_status_descr(_session, setStatus, setDescription.c_str());

	_status = status == -1 ? _status : status;
	_statusDescription = setDescription;
}

void GGSession::disconnect(const char* description) {
	if (!isConnected())
		return;

	string setDescription = description ? description : _statusDescription.c_str();
	int setStatus = setDescription.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;
	PlugStatusChange(ST_OFFLINE, setDescription.c_str());
	gg_change_status_descr(_session, setStatus, setDescription.c_str());

	gg_logoff(_session);
	_session = 0;
	_connected = false;
	_status = ST_OFFLINE;
	_statusDescription = setDescription.c_str();
}

unsigned __stdcall GGSession::connectProc(LPVOID lParam) {
	ConnectInfo ci(*(ConnectInfo*)lParam);
	delete lParam;
	GGSession* gg = ci.gg;

	gg_login_params gglp;
	memset(&gglp, 0, sizeof(gglp));
	gglp.uin = atoi(ci.login.c_str());
	gglp.password = (char*)ci.password.c_str();
	gglp.status = convertKStatus(ci.status, ci.statusDescription);
	gglp.status_descr = (char*)ci.statusDescription.c_str();
	gglp.async = false;
	gglp.client_version = GG_DEFAULT_CLIENT_VERSION;
	gglp.protocol_version = GG_DEFAULT_PROTOCOL_VERSION;
	gglp.last_sysmsg = 0;
	gglp.image_size = 0;
	gglp.tls = false;

	hostent* host;

	PlugStatusChange(ST_CONNECTING);
	gg->_status = ST_CONNECTING;

	/*bool rotateServers = (string)GETSTR(CFG::selectedServer) == "";
	if (!rotateServers) {
		host = gethostbyname(GETSTR(CFG::selectedServer));
		if (host)
			memcpy(&gglp.server_addr, host->h_addr, 4);
		else
			gglp.server_addr = 0;
	}*/

	gg->_stopConnecting = false;

	for (unsigned i = 0; !gg->_session && gg->_connecting && !gg->_stopConnecting; ++i) {
		PlugStatusChange(ST_CONNECTING);
		gg->_status = ST_CONNECTING;
		gg->_statusDescription = "";

		/*if (rotateServers) {
			host = gethostbyname(c->servers[i % c->servers.size()].ip.c_str());
			if (host)
				memcpy(&gglp.server_addr, host->h_addr, 4);
			else
				gglp.server_addr = 0;
		}*/

		gg->_session = gg_login(&gglp);
		
		if (gg->_session) {
			PlugStatusChange(ci.status, ci.statusDescription.c_str());
			gg_change_status_descr(gg->_session, convertKStatus(ci.status, ci.statusDescription), ci.statusDescription.c_str());

			gg->_status = ci.status;
			gg->_statusDescription = ci.statusDescription;
			gg->_connecting = false;
			gg->_connected = true;

			IMLOG("Kolejka zdarzeñ GG…");
			//debug: Do usuniêcia
			uin_t kontakty[] = {1169042};
			gg_notify(gg->_session, kontakty, 1);

			bool loop = true;

			for (bool first = true; loop && gg->_session->state == GG_STATE_CONNECTED; first = false) {
				gg_event* event = gg_watch_fd(gg->_session);
				if (!event && first) {
					IMLOG("Fa³szywe po³¹czenie, próbujê znowu…");
					//todo: To z³y sposób. Mam nadziejê, ¿e nowe libgadu tak nie robi.
					//c->connecting = true;
					break;
				} else if (!event) {
					break;
				}

				switch (event->type) {
					case GG_EVENT_CONN_FAILED: {
						loop = false;
						break;
					} case GG_EVENT_DISCONNECT: {
						loop = false;
						break;
					} case GG_EVENT_NOTIFY:
					case GG_EVENT_NOTIFY_DESCR: {
						//debug: Ponoæ serwer ju¿ nie wysy³a tego, ale jeœli dokumentacja libgadu siê myli to wolê siê jakoœ dowiedzieæ.
						MessageBox(0, "DOSTA£EŒ GG_EVENT_NOTIFY(_DESCR)!", 0, 0);
						break;
					} case GG_EVENT_NONE: {
						continue;
					} default: {
						
					}
				}
				(*ci.handler)(event);
				gg_event_free(event);
			}
			gg_free_session(gg->_session);
			gg->_session = 0;
			gg->_connected = false;
			gg->_statusDescription = "";
			IMLOG("Kolejka siê zakoñczy³a.");
		}
	}

	IMLOG("Koniec w¹tku.");
	PlugStatusChange(ST_OFFLINE);
	gg->_status = ST_OFFLINE;
	gg->_statusDescription = "";
	gg->_connecting = false;
	gg->_connected = false;
	return 0;
}

void GGSession::sendCnts(Contacts& cnts) {
	//hack: Hao mia³ ograniczenie do 400 kontaktów. Czemu?
	uin_t* uins = new uin_t[cnts.size()];
	char* types = new char[cnts.size()];
	unsigned cntsCount = 0;
	for (Contacts::iterator cnt = cnts.begin(); cnt != cnts.end(); ++cnt) {
		if (!(cnt->getStatus() & ST_NOTINLIST)) {
			uins[cntsCount] = atoi((char*)cnt->getUidString().a_str());
			types[cntsCount] = getCntType(cnt->getStatus());
			++cntsCount;
		}
	}
	IMLOG("Wysy³am listê, iloœæ: %i", cntsCount);
	gg_notify_ex(_session, uins, types, cntsCount);
	delete[] uins;
	delete[] types;
}

void GGSession::addCnt(Contact &cnt) {
	gg_add_notify_ex(_session, atoi(cnt.getUidString().a_str()), getCntType(cnt));
}

void GGSession::addCnt(string uin, char type) {
	gg_add_notify_ex(_session, atoi(uin.c_str()), type);
}

void GGSession::changeCnt(Contact &cnt) {
	gg_remove_notify_ex(_session, atoi(cnt.getUidString().a_str()), getCntType(cnt));
	gg_add_notify_ex(_session, atoi(cnt.getUidString().a_str()), getCntType(cnt));
}

void GGSession::removeCnt(Contact &cnt) {
	gg_remove_notify_ex(_session, atoi(cnt.getUidString().a_str()), getCntType(cnt));
}

void GGSession::removeCnt(string uin, char type) {
	gg_add_notify_ex(_session, atoi(uin.c_str()), type);
}

int GGSession::convertKStatus(tStatus status, string description, bool friendsOnly) {
	int result;
	if (status == ST_ONLINE)
		result = description.empty() ? GG_STATUS_AVAIL : GG_STATUS_AVAIL_DESCR;
	else if (status == ST_AWAY)
		result = description.empty() ? GG_STATUS_BUSY : GG_STATUS_BUSY_DESCR;
	else if (status == ST_HIDDEN)
		result = description.empty() ? GG_STATUS_INVISIBLE : GG_STATUS_INVISIBLE_DESCR;
	else if (status == ST_OFFLINE)
		result = description.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;
	else if (status == ST_IGNORED)
		result = GG_STATUS_BLOCKED;
	else
		result = description.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;

	if (friendsOnly)
		result |= GG_STATUS_FRIENDS_MASK;
	return result;
}

tStatus GGSession::convertGGStatus(int status) {
	if (status & GG_STATUS_AVAIL | GG_STATUS_AVAIL_DESCR)
		return ST_ONLINE;
	else if (status & GG_STATUS_BUSY | GG_STATUS_BUSY_DESCR)
		return ST_AWAY;
	else if (status & GG_STATUS_INVISIBLE | GG_STATUS_INVISIBLE_DESCR)
		return ST_HIDDEN;
	else if (status & GG_STATUS_NOT_AVAIL | GG_STATUS_NOT_AVAIL_DESCR)
		return ST_OFFLINE;
	else if (status & GG_STATUS_BLOCKED)
		return ST_BLOCKING;
	else
		return ST_OFFLINE;
}