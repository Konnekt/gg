#include "stdafx.h"
#include "../GG.h"
#include "Helpers.h"
#include "Session.h"
#include "Account.h"
#include "Directory.h"

Session::Session(string login, string password, ggHandler handler, bool friendsOnly) :
	_uid(atoi(login.c_str())), _password(password), _handler(handler), _friendsOnly(friendsOnly), _session(0), 
	_state(stDisconnected), _operation(opIdle), _status(ST_OFFLINE), _statusDescription("") { }

void Session::setProxy(bool enabled, bool httpOnly, string host, int port, string login, string password) {
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

bool Session::connect(tStatus status, string statusDescription) {
	if (!isConnected() && !isConnecting() && status != ST_OFFLINE) {
		_state = stConnecting;

		gg_login_params gglp;
		memset(&gglp, 0, sizeof(gglp));
		gglp.uin = _uid;
		gglp.password = (char*)_password.c_str();
		gglp.status = convertKStatus(status, statusDescription);
		gglp.status_descr = (char*)statusDescription.c_str();
		gglp.async = false;
		gglp.client_version = GG_DEFAULT_CLIENT_VERSION;
		gglp.protocol_version = GG_DEFAULT_PROTOCOL_VERSION;
		gglp.last_sysmsg = 0;
		gglp.image_size = 0;
		gglp.tls = false;

		hostent* host;

		_status = ST_CONNECTING;

		tServers::iterator default = find(_servers.begin(), _servers.end(), true);
		bool rotateServers = default == _servers.end();
		IMLOG("Rotacja serwerów: %i", rotateServers);
		if (!rotateServers) {
			host = gethostbyname(default->ip.c_str());
			if (host)
				memcpy(&gglp.server_addr, host->h_addr, 4);
			else
				gglp.server_addr = 0;
		}

		_stopConnecting = false;

		for (unsigned i = 0; !_session && isConnecting() && !_stopConnecting; ++i) {
			_status = ST_CONNECTING;
			_statusDescription = "";

			if (rotateServers) {
				host = gethostbyname(_servers[i % _servers.size()].ip.c_str());
				if (host)
					memcpy(&gglp.server_addr, host->h_addr, 4);
				else
					gglp.server_addr = 0;
			}
			IMLOG("£¹czê z \"%s\", rotacja: %i", _servers[i % _servers.size()].ip.c_str(), rotateServers);

			_session = gg_login(&gglp);

			if (_session) {
				gg_change_status_descr(_session, convertKStatus(status, statusDescription), statusDescription.c_str());

				_status = status;
				_statusDescription = statusDescription;
				_state = stConnected;
				return true;
			}
		}

		_status = ST_OFFLINE;
		_statusDescription = "";
		_state = stDisconnected;
		return 0;
	}
	return 0;
}

void Session::startListening() {
	IMLOG("Kolejka zdarzeñ GG…");
	if (isConnected()) {
		_state = stListening;

		for (bool first = true; isListening() && _session->state == GG_STATE_CONNECTED; first = false) {
			gg_event* event = gg_watch_fd(_session);
			if (!event) {
				break;
			}

			switch (event->type) {
				case GG_EVENT_CONN_FAILED: {
					_state = stDisconnected;
					break;
				} case GG_EVENT_DISCONNECT: {
					_state = stDisconnected;
					break;
				} case GG_EVENT_NOTIFY:
				case GG_EVENT_NOTIFY_DESCR: {
					//debug: Ponoæ serwer ju¿ nie wysy³a tego, ale jeœli dokumentacja libgadu siê myli to wolê siê jakoœ dowiedzieæ.
					MessageBox(0, "DOSTA£EŒ GG_EVENT_NOTIFY(_DESCR)!", 0, 0);
					break;
				} case GG_EVENT_USERLIST: {
					if (event->event.userlist.type == GG_USERLIST_PUT_REPLY && _operation == opExportContacts) {
						_operation = opIdle;
					} else if (event->event.userlist.type == GG_USERLIST_GET_REPLY && _operation == opImportContacts) {
						_operation = opIdle;
					} else {
						gg_event_free(event);
						continue;
					}
					break;
				} case GG_EVENT_NONE: {
					continue;
				} default: {
					
				}
			}
			(*_handler)(event);
			gg_event_free(event);
		}
		IMLOG("Kolejka siê zakoñczy³a.");

		_state = stDisconnected;
		_status = ST_OFFLINE;
		_statusDescription = "";
		gg_free_session(_session);
		_session = 0;
	}
}

void Session::stopConnecting() {
	_stopConnecting = true;
}

void Session::setStatus(tStatus status, const char* description) {
	if (!isConnected() && !isConnecting() && status != ST_OFFLINE) {
		return;
	} else if (isConnecting() && status == ST_OFFLINE) {
		_stopConnecting = true;
		stopConnecting();
		return;
	} else if (status == ST_OFFLINE) {
		return disconnect(description);
	}

	string setDescription = description ? description : _statusDescription.c_str();
	int setStatus = status == -1 ? convertKStatus(_status, setDescription) : convertKStatus(status, setDescription);
	PlugStatusChange(status == -1 ? _status : status, setDescription.c_str());
	gg_change_status_descr(_session, setStatus, setDescription.c_str());

	_status = status == -1 ? _status : status;
	_statusDescription = setDescription;
}

void Session::ping() {
	gg_ping(_session);
}

void Session::disconnect(const char* description) {
	if (!isConnected())
		return;

	string setDescription = description ? description : _statusDescription.c_str();
	int setStatus = setDescription.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;
	gg_change_status_descr(_session, setStatus, setDescription.c_str());

	gg_logoff(_session);
	_session = 0;
	_state = stDisconnected;
	_status = ST_OFFLINE;
	_statusDescription = setDescription.c_str();
}

void Session::sendContacts(tContacts& cnts) {
	//hack: Hao mia³ ograniczenie do 400 kontaktów. Czemu?
	uin_t* uins = new uin_t[cnts.size()];
	char* types = new char[cnts.size()];
	unsigned cntsCount = 0;
	for (tContacts::iterator cnt = cnts.begin(); cnt != cnts.end(); ++cnt) {
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

void Session::addContact(Contact& cnt) {
	if (isConnected())
		gg_add_notify_ex(_session, atoi(cnt.getUidString().a_str()), getCntType(cnt.getStatus()));
}

void Session::addContact(string uid, char type) {
	if (isConnected())
		gg_add_notify_ex(_session, atoi(uid.c_str()), type);
}

void Session::removeContact(Contact& cnt) {
	if (isConnected())
		gg_remove_notify_ex(_session, atoi(cnt.getUidString().a_str()), getCntType(cnt.getStatus()));
}

void Session::removeContact(string uid, char type) {
	if (isConnected())
		gg_remove_notify_ex(_session, atoi(uid.c_str()), type);
}

void Session::setFriendsOnly(bool friendsOnly) {
	_friendsOnly = friendsOnly;
	setStatus(-1, 0);
}

void Session::cancelOperation() {
	_operation = opIdle;
}
