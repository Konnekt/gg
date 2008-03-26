#include "stdafx.h"
#include "GG.h"
using namespace Konnekt::GG;
using Stamina::inttostr;
using Stamina::longToIp;
using Stamina::ipToLong;
using Stamina::str_tr;

VOID CALLBACK GG::timerProc(HWND hwnd,UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
	//Info o dostarczeniu wiadomoœci
	if (!GG::check(true, true, false, false))
		return;
	time_t curTime = time(0);
	EnterCriticalSection(&msgSent_CS);
	//if (msgSent.size()) IMLOG("msgSent Check n=%d", msgSent.size());
	tMsgSent::iterator it=msgSent.begin();
	while (it != msgSent.end()) {
		if (curTime - it->second.sentTime > MSG_TIMEOUT) {
			if (GETINT(CFG_ACK_SHOWFAILED)) 
				quickEvent(it->second.Uid, CStdString("Wiadomoœæ \""+it->second.digest+"\" prawdopodobnie nie zosta³a dostarczona."));
			it = msgSent.erase(it);
		} else {
			it++;
		}
	}
	LeaveCriticalSection(&msgSent_CS);
	// PING
	static int ping_time =- TIMER_INTERVAL;
	ping_time += TIMER_INTERVAL;
	if (ping_time >= PING_INTERVAL && sess) {
		ping_time = 0;
		gg_ping(sess);
	}
}

gg_session * GG::loginWithTimeout(gg_login_params * p) {
	sDIALOG_long sd;
	sd.flag = DLONG_NODLG;
	sd.timeoutProc = timeoutDialogCB;
	sd.timeout = TIMEOUT;
	if (GG::event(GGER_LOGIN, p) & GGERF_ABORT)
		return 0;
	ICMessage(IMI_LONGSTART, (int)&sd);
	gg_session * sess = gg_login(p);
	ICMessage(IMI_LONGEND, (int)&sd);
	return sess;
}

unsigned int __stdcall GG::threadProc (void * lpParameter) {
	ggThreadId = GetCurrentThreadId();
	if (GG::check(0, 0, 1, 1)) {
		gg_event *e;
		GG::onRequest = false;
		GG::loop=1;
		GG::sess = 0;
		int a,b,c;
		curStatus = ST_CONNECTING;
		PlugStatusChange(ST_CONNECTING, "");
		GG::setProxy();
		IMLOG("PROXY %d, %s : %d", gg_proxy_enabled, gg_proxy_host, gg_proxy_port);
		gg_login_params gglp;
		memset(&gglp, 0, sizeof(gglp));
		CStdString ggPass;
		getAccount((int &)gglp.uin, ggPass);
		gglp.password = (char*)ggPass.c_str();
		setStatus(GETINT(CFG_GG_STATUS),1,&gglp);
		gglp.async = 0;
		//gglp.client_addr
		//gglp.client_port
		//gglp.external_addr;
		//gglp.external_port;
		gglp.client_version = GG_DEFAULT_CLIENT_VERSION;
		gglp.last_sysmsg = 0;
		gglp.protocol_version = GG_DEFAULT_PROTOCOL_VERSION;
		gglp.image_size = 0;
		gglp.era_omnix = Ctrl->DTgetInt(DTCFG, 0, "GG/imOmnix");
		gglp.tls = GETINT(CFG_GG_USESSL);
		CStdString servers = GETSTR(CFG_GG_SERVER);
		size_t pos = 0;
		while ((pos = servers.find("\r", pos)) != -1) {
			servers.erase(pos, 1);
		}
		if (gglp.tls && servers.find("SSL") == -1) {
			servers = "SSL\n" + servers;
		}
		size_t start = ("\n" + servers + "\n").find("\n" + lastServer + "\n"); // Znajdujemy ostatnio uzyty serwer
		if (start == -1) start = 0;
		pos = start;
		bool success = false;
		CStdString serv; // Aktualnie wybrany serwer
		serv = servers.substr(pos, servers.find("\n", pos) - pos);
		//Gdy SSL, nie u¿ywamy listy serwerów!
		//if (gglp.tls) serv = "";
		GG::event(GGER_BEFORELOGIN, &gglp);
		//DCC
		gg_dcc_ip = 0;
		gg_dcc_port = 0;
		do {
			gglp.server_addr = 0; 
			gglp.server_port=0;
			Stamina::RegEx hostPreg;
			// 1 - Wykluczenie 2 - SSL 3 - Host 4 - Port
			hostPreg.match("/^\\s*(\\!?)\\s*(SSL *)?(?:([a-z0-9_\\-\\.]+\\.[a-z0-9]+)(?:\\:(\\d+))?)?\\s*$/i", serv);
			gglp.tls = hostPreg.hasSub(2);
			if ((hostPreg.hasSub(1) == false /*Jest wlaczony*/) && (!gglp.tls || GETINT(CFG_GG_USESSL))) {
				// Odczytujemy to, co znajdziemy
				if (hostPreg.hasSub(3)) {
					hostent * he=gethostbyname(hostPreg[3].c_str());
					if (he) {
						//gglp.server_addr = (int)he->h_addr;
						memcpy(&gglp.server_addr, he->h_addr, 4);
					}
					if (hostPreg.hasSub(4))
						gglp.server_port = atoi(hostPreg[4].c_str());
					else
						gglp.server_port = GG_DEFAULT_PORT;
				}
				//Laczymy sie z naszym znaleziskiem
				IMLOG("Connecting to \"%s\" (SSL=%d Host=\"%s\" Port=%d) from list", serv.c_str(), gglp.tls, hostPreg[3].c_str(), gglp.server_port);
				GG::currentServer = serv;
				GG::sess = 0;
				GG::sessionUsage = 0;
				if ((sess = GG::loginWithTimeout(&gglp))) {
					success = true;
					break;
				} else {
					//if (gglp.failure == GG_FAILURE_PASSWORD) {
						ICMessage(IMI_CONFIG, IMIG_GGCFG_USER);
						ICMessage(IMI_ERROR, (int)"Poda³eœ z³y numer konta lub has³o dla GG.\r\nSprawdŸ w konfiguracji i spróbuj po³¹czyæ siê ponownie.");
						onRequest = true;
					//}
					success = false;
				}
				// Nie uda³o siê... usuwamy TLS
				gglp.tls = 0;
			} // !
			pos = servers.find("\n", pos) + 1;
			serv = servers.substr(pos, servers.find("\n", pos) - pos);
		} while (!success && pos != start && !onRequest);
		GG::lastServer = serv;
		GG::loop = success;

		if (GG::loop) {
			GG::event(GGER_LOGGEDIN, 0);
			uin_t * uins = new uin_t[500];
			char * types = new char[500];
			int c = IMessage(IMC_CNT_COUNT);
			int count=0;
			int j = 0;
			//if (!(GETCNTI(0,CNT_STATUS)&(ST_HIDEMYSTATUS))) {
			for (int i=1 ; i<c && j < 400;i++) {
				int cntStatus = GETCNTI(i, CNT_STATUS);
				if (GETCNTI(i,CNT_NET) == NET_GG && !(cntStatus & (ST_NOTINLIST))) {
					j++;
					uins[count] = atoi((char *)GETCNTC(i,CNT_UID));
					types[count] = GG::userType(i);
					if (uins[count]) count++;
				}
			}
		//}
		//sess->initial_status = GETINT(CFG_GG_STATUS);
		gg_notify_ex(sess, uins, types, count);
		delete [] uins;
		delete [] types;
		ICMessage(IMC_MESSAGEQUEUE, (int)&sMESSAGESELECT(NET_GG, 0, MT_MESSAGE, MF_SEND));

		/*if (gglp.status_descr) // Dziwny sposob ale dziala
			setStatus(GETINT(CFG_GG_STATUS),1);
		else*/
		setStatus(-1,-1,(gg_login_params*)-1);
		/*LARGE_INTEGER lDueTime;
		lDueTime.QuadPart = 0;
		SetWaitableTimer(timer, &lDueTime, TIMER_INTERVAL ,TimerAPCProc, 0, 0);
		timer = SetTimer(0,0,TIMER_INTERVAL,(TIMERPROC)TimerProc);*/
		
		if (GG::loop)
			ICMessage(IMC_SETCONNECT, 0);
		GG::event(GGER_FIRSTLOOP, 0);
		while (GG::loop) {
			if (!(e = gg_watch_fd(sess))) {
				IMLOG("! GG - przerwanie po³¹czenia");
				//disconnect();
				break;
			}
			int er = GG::event(GGER_EVENT, e);
			if (!(er & GGERF_ABORT))
				switch (e->type) {
					case GG_EVENT_ACK: {
						IMLOG("ACK rec = %d, seq = %d, stat = %d", e->event.ack.recipient, e->event.ack.seq, e->event.ack.status);
						switch (e->event.ack.status) {
							case GG_ACK_QUEUED: {
								if (GETINT(CFG_ACK_SHOWQUEUED))
									quickEvent(e->event.ack.recipient, "Odbiorca jest niedostêpny. Wiadomoœæ zosta³a dodana do kolejki.");
							} case GG_ACK_DELIVERED: {
								EnterCriticalSection(&msgSent_CS);
								tMsgSent::iterator fnd = msgSent.find(e->event.ack.seq);
								if (fnd != msgSent.end())
									msgSent.erase(fnd);
								LeaveCriticalSection(&msgSent_CS);
								break;
							}
						}
						break;
					} case GG_EVENT_CONN_FAILED: {
						if (e->event.failure == GG_FAILURE_PASSWORD)
							ICMessage(IMI_ERROR, (int)"Poda³eœ z³y numer konta lub has³o dla GG.\r\nSprawdŸ w konfiguracji i spróbuj po³¹czyæ siê ponownie.", (int)"Konnekt - GG");
						GG::loop = 0;
						onRequest = true;
						break;
					} case GG_EVENT_DISCONNECT: {
						GG::loop=0;
						onRequest = GETINT(CFG_GG_DONTRESUMEDISCONNECTED) ? true : false;
						break;
					} case GG_EVENT_NOTIFY_DESCR:
					case GG_EVENT_NOTIFY: {
						gg_notify_reply *n;
						c = sizeof(gg_notify_reply);
						n = (e->type == GG_EVENT_NOTIFY) ? e->event.notify : e->event.notify_descr.notify;
						c = 0;
						while (n->uin) {
							c++;
							a = IMessage(IMC_FINDCONTACT, 0, 0, NET_GG, (int)Stamina::inttostr(n->uin).c_str());
							if (a > 0 && GETCNTI(a, CNT_STATUS) & ST_IGNORED)
								a = -1;
							b = ST_OFFLINE;
							switch (n->status) {
								case GG_STATUS_AVAIL:
								case GG_STATUS_AVAIL_DESCR: {
									b = ST_ONLINE;
									break;
								} case GG_STATUS_BUSY:
								case GG_STATUS_BUSY_DESCR: { 
									b=ST_AWAY;
									break;
								} case GG_STATUS_BLOCKED: {
									b = ST_BLOCKING; break;
								}
							}
							IMLOG("__GG Notify c=%d st=%x [%x] D=%d", DT_UNMASKID(a), n->status, b, e->type == GG_EVENT_NOTIFY_DESCR);
							if (a > 0) {
								ICMessage(IMI_CNT_ACTIVITY, a);
								CntSetStatus(a, b, (e->type == GG_EVENT_NOTIFY_DESCR) ? str_tr(e->event.notify_descr.descr, "\r\n", "  ") : "");
								if (n->remote_ip || n->remote_port) {
									SETCNTC(a, CNT_HOST, n->remote_ip ? (char*)longToIp(n->remote_ip).c_str() : "");
									SETCNTI(a, CNT_PORT, n->remote_port & 0xFFFF);
								}
							}
							n++;
						}
						if (c == 1) 
							ICMessage(IMI_REFRESH_CNT, a);
						else 
							ICMessage(IMI_REFRESH_LST);
						break;
					} case GG_EVENT_STATUS: {
						a = IMessage(IMC_FINDCONTACT, 0,0, NET_GG, (int)inttostr(e->event.status.uin).c_str());
						if (a <= 0) break;
						if (ICMessage(GETCNTI(a, CNT_STATUS) & ST_IGNORED)) break;
							b = ST_OFFLINE;
						char* descr = "";
						switch (e->event.status.status) {
							case GG_STATUS_NOT_AVAIL_DESCR: {
								descr = e->event.status.descr;
								break;
							} case GG_STATUS_AVAIL_DESCR: {
								descr = e->event.status.descr;
							}	case GG_STATUS_AVAIL: {
								b = ST_ONLINE; break;
							} case GG_STATUS_BUSY_DESCR: {
								descr = e->event.status.descr;
							}	case GG_STATUS_BUSY: {
								b = ST_AWAY;
								break;
							}	case GG_STATUS_BLOCKED: {
								b = ST_BLOCKING;
								break;
							}
						}
						IMLOG("__GG Status c=%d st=%x [%x] \"%s\"", DT_UNMASKID(a), e->event.status.status, b, descr);
						ICMessage(IMI_CNT_ACTIVITY, a);
						CntSetStatus(a, b, str_tr(descr, "\r\n", "  "));
						ICMessage(IMI_REFRESH_CNT, a);

						break;
					} case GG_EVENT_NOTIFY60: {
						c = 0;
						int cnt = -1;
						while (e->event.notify60[c].uin) {
							int cnt = IMessage(IMC_FINDCONTACT, 0, 0, NET_GG, (int)inttostr(e->event.notify60[c].uin).c_str());
							if (cnt > 0 && GETCNTI(cnt, CNT_STATUS) & ST_IGNORED) cnt = -1;
							int status = ST_OFFLINE;
							switch (e->event.notify60[c].status) {
								case GG_STATUS_AVAIL:
								case GG_STATUS_AVAIL_DESCR: {
									status = ST_ONLINE;
									break;
								} case GG_STATUS_BUSY:
								case GG_STATUS_BUSY_DESCR: {
									status = ST_AWAY;
									break;
								} case GG_STATUS_BLOCKED: {
									status = ST_BLOCKING;
									break;
								}
							}
							IMLOG("__GG Notify c=%d st=%x [%x]", DT_UNMASKID(cnt), e->event.notify60[c].status, status);
							if (cnt > 0) {
								ICMessage(IMI_CNT_ACTIVITY, cnt);
								CntSetStatus(cnt, status, e->event.notify60[c].descr);
								if (e->event.notify60[c].remote_ip || e->event.notify60[c].remote_port) {
									SETCNTC(cnt, CNT_HOST, e->event.notify60[c].remote_ip?(char*)longToIp(e->event.notify60[c].remote_ip).c_str():"");
									SETCNTI(cnt, CNT_PORT, e->event.notify60[c].remote_port & 0xFFFF);
								}
							}
							c++;
						}
						if (c == 1)
							ICMessage(IMI_REFRESH_CNT, cnt);
						else
							ICMessage(IMI_REFRESH_LST);
						break;
					} case GG_EVENT_STATUS60: {
						int cnt = IMessage(IMC_FINDCONTACT, 0, 0, NET_GG, (int)inttostr(e->event.status60.uin).c_str());
						if (cnt <= 0) break;
						if (ICMessage(GETCNTI(cnt, CNT_STATUS) & ST_IGNORED)) break;
						b = ST_OFFLINE;
						char* descr="";
						switch (e->event.status60.status) {
							case GG_STATUS_NOT_AVAIL_DESCR: { 
								descr = e->event.status60.descr;
								break;
							} case GG_STATUS_AVAIL_DESCR: {
								descr = e->event.status60.descr;
							} case GG_STATUS_AVAIL: {
								b = ST_ONLINE;
								break;
							} case GG_STATUS_BUSY_DESCR: {
								descr = e->event.status60.descr;
							} case GG_STATUS_BUSY: {
								b = ST_AWAY;
								break;
							} case GG_STATUS_BLOCKED: {
								b = ST_BLOCKING;
								break;
							}
						}
						if (e->event.status60.remote_ip || e->event.status60.remote_port) {
							SETCNTC(cnt, CNT_HOST, e->event.status60.remote_ip ? (char*)longToIp(e->event.status60.remote_ip).c_str():"");
							SETCNTI(cnt, CNT_PORT, e->event.status60.remote_port & 0xFFFF);
						}
						IMLOG("__GG Status c=%d st=%x [%x] \"%s\"", DT_UNMASKID(cnt), e->event.status60.status, b, descr);
						ICMessage(IMI_CNT_ACTIVITY, cnt);
						CntSetStatus(cnt, b, descr);
						ICMessage(IMI_REFRESH_CNT, cnt);
						break;
					} case GG_EVENT_MSG: {
						if (e->event.msg.msgclass == GG_CLASS_CTCP || !e->event.msg.message || !*e->event.msg.message)
							break;
						cMessage m;
						//ZeroMemory(&m, sizeof(m));
						//e->event.msg.sender=0;
						m.net = NET_GG;
						m.type = e->event.msg.sender?MT_MESSAGE:MT_SERVEREVENT;
						m.toUid = "";
						CStdString body;
						if (e->event.msg.formats_length && e->event.msg.formats) {
							body = GG::msgToHtml((char*)e->event.msg.message, e->event.msg.formats, e->event.msg.formats_length);
							m.flag = MF_HTML;
						} else {
							 body = (char*)e->event.msg.message;
						}
						m.body = (char*)body.c_str();
						CStdString from;
						if (e->event.msg.sender) {
							from = inttostr(e->event.msg.sender);
						}
						m.fromUid = (char*)from.c_str();
						m.ext = "";
						m.flag |= MF_HANDLEDBYUI;
						m.time = min(_time64(0), e->event.msg.time);
						ICMessage(IMI_CNT_ACTIVITY, IMessage(IMC_FINDCONTACT, 0,0, NET_GG, (int)m.fromUid));
						if (ICMessage(IMC_CNT_IGNORED, NET_GG, (int)m.fromUid)) {
							sHISTORYADD ha;
							ha.m = &m;
							ha.dir = HISTORY_IGNORED_DIR;
							ha.name = HISTORY_IGNORED_NAME;
							ha.cnt = 0;
							ha.session = 0;
							ICMessage(IMI_HISTORY_ADD, (int)&ha);
							break;
						}
						IMDEBUG(DBG_LOG, "- MSG_NEW: c=%s, t=%x", m.fromUid, time(0));
						sMESSAGESELECT ms;
						ms.id = IMessage(IMC_NEWMESSAGE, 0, 0, (int)&m, 0);
						if (ms.id)
							ICMessage(IMC_MESSAGEQUEUE, (int)&ms);
						break;
					}	case GG_EVENT_PUBDIR50_SEARCH_REPLY: {
						GG::onPubdirSearchReply(e);
						break;
					} case GG_EVENT_USERLIST: {
						onUserlistReply(e);
					}
				}
				gg_free_event(e);
			}

			GG::event(GGER_LOGOUT, 0);
			GG::waitOnSessions();
			gg_free_session(sess);
			sess = 0;
			PlugStatusChange(ST_OFFLINE, "");
			
			// Wyzerowanie statusow na liscie
			c = IMessage(IMC_CNT_COUNT);
			for (int i=1 ; i<c;i++) {
				if (GETCNTI(i,CNT_NET) == NET_GG) {
					CntSetStatus(i, ST_OFFLINE, "");
					ICMessage(IMI_CNT_DEACTIVATE, i);
				}
			}
			IMessage(IMI_REFRESH_LST);
			ICMessage(IMC_SETCONNECT, !onRequest);
		}
	}
	CloseHandle(ggThread);
	ggThread = 0;
	IMLOG("- GGThread finished");
	return 0;
}


int GG::connect() {
	if (!timer)
		timer = SetTimer(0, 0, TIMER_INTERVAL, (TIMERPROC)GG::timerProc);
	//ggThread=CreateThread(0,0, GGThreadProc, 0,0,&i);
	ggThread = (HANDLE)Ctrl->BeginThread("Connect", 0, 0, GG::threadProc, 0, 0, 0);
	return 1;
}

int GG::disconnect() {
	if (curStatus == ST_OFFLINE) return 0;
	GG::loop = 0;
	onRequest = true;
	if (timer) KillTimer(0, timer);
		timer = 0;
	//CancelWaitableTimer(timer);
	if (sess) {
		setStatus(GG_STATUS_NOT_AVAIL,1);
		curStatus = ST_OFFLINE; // dla pewnoœci :)
		GG::event(GGER_BEFORELOGOUT, 0);
		gg_logoff(sess);
		IMLOG("GG - disconnected");
	} else if (ggThread && gg_thread_socket(ggThreadId,0)) {
		setStatus(GG_STATUS_NOT_AVAIL,1,(gg_login_params*)-1);
		IMLOG("___CLOSED WHILE CONNECTING!____");
		gg_thread_socket(ggThreadId,-1);
	} else { // roz³¹czony
		Ctrl->ICMessage(IMC_SETCONNECT, 0);
	}
	// ICMessage(IMC_SETCONNECT, !onRequest);
	return 1;
}