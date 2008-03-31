#include "stdafx.h"
#include "GG.h"
#include "Controller.h"

void GG::onPubdirSearchReply(gg_event *e) {
	gg_pubdir50_t src = e->event.pubdir50;

	EnterCriticalSection(&GG::searchMap_CS);
	const char * ch;
	if (GG::searchMap.find(src->seq) == GG::searchMap.end() || GG::searchMap[src->seq] == 0) goto end;
	if (GG::searchMap[src->seq]->cnt != -1) { 
		// KONTAKTY
		if (GG::searchMap[src->seq]->store) {
			#define SEARCH_STORE(fld,col,val) {ch = gg_pubdir50_get(src, 0, fld);if (ch) SETCNTC(GG::searchMap[src->seq]->cnt, col, val);}
			SEARCH_STORE(GG_PUBDIR50_FIRSTNAME, CNT_NAME, ch);
			SEARCH_STORE(GG_PUBDIR50_LASTNAME, CNT_SURNAME, ch);
			SEARCH_STORE(GG_PUBDIR50_NICKNAME, CNT_NICK, ch);
			SEARCH_STORE(GG_PUBDIR50_CITY, CNT_CITY, ch);
			SEARCH_STORE(GG_PUBDIR50_GENDER, CNT_GENDER, inttostr((*ch == *GG_PUBDIR50_GENDER_MALE)?GENDER_MALE : ((*ch == *GG_PUBDIR50_GENDER_FEMALE)?GENDER_FEMALE : GENDER_UNKNOWN)).c_str());
			SEARCH_STORE(GG_PUBDIR50_BIRTHYEAR, CNT_BORN, (inttostr(*ch?(atoi(ch) << 16)|0x0101:0).c_str()));
		} else {
			#define SEARCH_NOSTORE(fld,grp,col,val) {const char * ch = gg_pubdir50_get(src, 0, fld);if (ch) UIActionCfgSetValue(sUIAction(grp,col|IMIB_CNT, GG::searchMap[src->seq]->cnt), val, false);}
			SEARCH_NOSTORE(GG_PUBDIR50_FIRSTNAME, IMIG_NFO_DETAILS, CNT_NAME, ch);
			SEARCH_NOSTORE(GG_PUBDIR50_LASTNAME, IMIG_NFO_DETAILS, CNT_SURNAME, ch);
			SEARCH_NOSTORE(GG_PUBDIR50_NICKNAME, IMIG_NFO_DETAILS, CNT_NICK, ch);
			SEARCH_NOSTORE(GG_PUBDIR50_CITY, IMIG_NFO_CONTACT, CNT_CITY, ch);
			SEARCH_NOSTORE(GG_PUBDIR50_GENDER, IMIG_NFO_INFO, CNT_GENDER, inttostr((*ch == *GG_PUBDIR50_GENDER_MALE)?GENDER_MALE : ((*ch == *GG_PUBDIR50_GENDER_FEMALE)?GENDER_FEMALE : GENDER_UNKNOWN)).c_str());
			SEARCH_NOSTORE(GG_PUBDIR50_BIRTHYEAR, IMIG_NFO_INFO, CNT_BORN, (string("Y")+ch).c_str());

		}
		#undef SEARCH_STORE
		#undef SEARCH_NOSTORE
	} else {
		// WYSZUKIWANIE
		if (src->count) {
			for (int i = 0; i < src->count; i++) {
				sCNTSEARCH fnd;
				fnd.net = GG::net;
				switch (atoi(SAFECHAR(gg_pubdir50_get(src, i, GG_PUBDIR50_STATUS)))) {
					case 1: fnd.status = ST_OFFLINE; break;
					case 2: fnd.status = ST_ONLINE; break;
					case 3: fnd.status = ST_AWAY; break;
				}
				strncpy(fnd.uid, SAFECHAR(gg_pubdir50_get(src, i, GG_PUBDIR50_UIN)), sizeof(fnd.uid));
				strncpy(fnd.name, SAFECHAR(gg_pubdir50_get(src, i, GG_PUBDIR50_FIRSTNAME)), sizeof(fnd.name));
				strncpy(fnd.surname, SAFECHAR(gg_pubdir50_get(src, i, GG_PUBDIR50_LASTNAME)), sizeof(fnd.surname));
				strncpy(fnd.nick, SAFECHAR(gg_pubdir50_get(src, i, GG_PUBDIR50_NICKNAME)), sizeof(fnd.nick));
				strncpy(fnd.city, SAFECHAR(gg_pubdir50_get(src, i, GG_PUBDIR50_CITY)), sizeof(fnd.city));
				fnd.other[0] = 0;
				fnd.born_min = fnd.born_max = atoi(SAFECHAR(gg_pubdir50_get(src, i, GG_PUBDIR50_BIRTHYEAR)));
				fnd.gender = atoi(SAFECHAR(gg_pubdir50_get(src, i, GG_PUBDIR50_GENDER)));
				switch (fnd.gender) {
					case 1 /*GG_PUBDIR50_GENDER_FEMALE*/: fnd.gender = GENDER_FEMALE; break;
					case 2 /* GG_PUBDIR50_GENDER_MALE*/: fnd.gender = GENDER_MALE; break;
					default: fnd.gender = GENDER_UNKNOWN; break;
				}
				if (i == src->count - 1 && src->next && src->count>5) 
					fnd.start = src->next;
				ICMessage(IMI_CNT_SEARCH_FOUND, (int)&fnd);
			}
		} else {
			Ctrl->IMessage(&sIMessage_msgBox(IMI_INFORM, "Nic nie znalaz³em.", 0, 0, 0));
		}
	}
	GG::searchMap[src->seq]->finished = true;
	end:
		LeaveCriticalSection(&GG::searchMap_CS);
}

unsigned int __stdcall GG::doCntSearch(LPVOID lParam) {
	if (!GG::check(0, 1, 0, 1)) return 0;

	sCNTSEARCH* src = (sCNTSEARCH*)lParam;
	sDIALOG_long sd;
	sd.title = "GG - katalog publiczny";
	sd.info = "Wysy³am zapytanie i odbieram wyniki…";
	sd.flag = DLONG_AINET | DLONG_CANCEL;
	sd.cancelProc = cancelDialogCB;
	sd.timeoutProc = timeoutDialogSimpleCB;
	sd.timeout = PUBDIR_TIMEOUT;
	sd.threadId = ggThreadId;
	sd.handle = src->handle;
	ICMessage(IMI_LONGSTART, (int)&sd);
	gg_pubdir50_t req = gg_pubdir50_new(GG_PUBDIR50_SEARCH);
	req->next = src->start;
	if (src->status) gg_pubdir50_add(req, GG_PUBDIR50_ACTIVE, GG_PUBDIR50_ACTIVE_TRUE);

	if (src->start) gg_pubdir50_add(req, GG_PUBDIR50_START, inttostr(src->start).c_str());
	if (*src->uid) gg_pubdir50_add(req, GG_PUBDIR50_UIN, src->uid);
	else {
		if (*src->nick) gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, src->nick);
		if (*src->name) gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, src->name);
		if (*src->surname) gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, src->surname);
		if (*src->city) gg_pubdir50_add(req, GG_PUBDIR50_CITY, src->city);
		if (src->gender != GENDER_UNKNOWN) gg_pubdir50_add(req, GG_PUBDIR50_GENDER, src->gender==GENDER_MALE?GG_PUBDIR50_GENDER_MALE:GG_PUBDIR50_GENDER_FEMALE);

		if (src->born_min) 
			gg_pubdir50_add(req, GG_PUBDIR50_BIRTHYEAR, inttostr(src->born_min) + CStdString(" ") + inttostr(src->born_max) );
	}

	int res = gg_pubdir50(sess, req);
	gg_pubdir50_free(req);
	if (res) {
		cGGSearch srch;
		srch.cnt = -1;
		srch.store = false;
		srch.finished = false;
		EnterCriticalSection(&GG::searchMap_CS);
		GG::searchMap[res] = &srch;
		LeaveCriticalSection(&GG::searchMap_CS);

		while (ggThread && !srch.finished && !sd.cancel) 
			Sleep(100);
		EnterCriticalSection(&GG::searchMap_CS);
		GG::searchMap.erase(res);
		LeaveCriticalSection(&GG::searchMap_CS);
	} else {
		Ctrl->IMessage(&sIMessage_msgBox(IMI_ERROR, "Wyst¹pi³ b³¹d! \nJe¿eli jesteœ po³¹czony z internetem, to znaczy ¿e problem le¿y po stronie serwera GG. Spróbuj póŸniej.", 0, 0, src->handle));
	}
	ICMessage(IMI_LONGEND, (int)&sd);
	return 0;
}

CStdString GG::nfoGet(bool noTable, int cnt, int id) {
	return CntGetInfoValue(noTable, cnt, id);
}

unsigned int __stdcall GG::doCntDownload(int pMsg) {
	sIMessage * msg = (sIMessage*)pMsg;
	int cnt = Ctrl->DTgetPos(DTCNT, msg->p1);
	IMLOG("CNT download");
	GG::setProxy();

	sDIALOG_long sdl;
	sdl.title = "GG - katalog publiczny";
	sdl.info = "Pobieram dane";
	sdl.flag = DLONG_ARECV | DLONG_CANCEL;
	sdl.cancelProc = cancelDialogCB;
	sdl.timeoutProc = timeoutDialogSimpleCB;
	sdl.timeout = PUBDIR_TIMEOUT;
	sdl.threadId = ggThreadId;
	sdl.handle = (HWND)ICMessage(IMI_GROUP_GETHANDLE, (int)&sUIAction(0,IMIG_NFOWND,msg->p1));

	ICMessage(IMI_LONGSTART, (int)&sdl);
	gg_pubdir50_t req = gg_pubdir50_new(GG_PUBDIR50_SEARCH);
	gg_pubdir50_add(req, GG_PUBDIR50_UIN, cnt?nfoGet(msg->p2 != 0, msg->p1,CNT_UID):GETSTR(CFG_GG_LOGIN));
	//EnableWindow((HWND)sdl.handle, true); /* TODO: to powinno byc jakos inaczej nie? */
	int res = gg_pubdir50(sess, req);
	gg_pubdir50_free(req);
	if (!res) {
		Ctrl->IMessage(&sIMessage_msgBox(IMI_ERROR, "Dane nie zosta³y pobrane z serwera.", 0, 0, (HWND)ICMessage(IMI_GROUP_GETHANDLE, (int)&sUIAction(0,IMIG_NFOWND,msg->p1)))); 
		ICMessage(IMI_LONGEND, (int)&sdl);
		return 0;
	}
	cGGSearch srch;
	srch.cnt = msg->p1;
	srch.store = !msg->p2;
	srch.finished = false;
	EnterCriticalSection(&GG::searchMap_CS);
	GG::searchMap[res] = &srch;
	LeaveCriticalSection(&GG::searchMap_CS);

	while (ggThread && !srch.finished && !sdl.cancel) 
		Sleep(100);
	EnterCriticalSection(&GG::searchMap_CS);
	GG::searchMap.erase(res);
	LeaveCriticalSection(&GG::searchMap_CS);
	ICMessage(IMI_LONGEND, (int)&sdl);
	return 0;
}

unsigned int __stdcall GG::doCntUpload(sIMessage_2params* msg) {
	int cnt = Ctrl->DTgetPos(DTCNT, msg->p1);
	if (!cnt) {
		sDIALOG_long sdl;
		sdl.title = "GG - katalog publiczny";
		sdl.info = "Wysy³am zmiany";
		sdl.flag = DLONG_ASEND|DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogSimpleCB;
		sdl.timeout = PUBDIR_TIMEOUT;
		sdl.threadId = ggThreadId;
		ICMessage(IMI_LONGSTART, (int)&sdl);
		gg_pubdir50_t req = gg_pubdir50_new(GG_PUBDIR50_WRITE);

		gg_pubdir50_add(req, GG_PUBDIR50_FIRSTNAME, nfoGet(msg->p2 != 0, cnt, CNT_NAME));
		gg_pubdir50_add(req, GG_PUBDIR50_LASTNAME, nfoGet(msg->p2 != 0, cnt, CNT_SURNAME));
		gg_pubdir50_add(req, GG_PUBDIR50_NICKNAME, nfoGet(msg->p2 != 0, cnt, CNT_NICK));
		gg_pubdir50_add(req, GG_PUBDIR50_CITY, nfoGet(msg->p2 != 0, cnt, CNT_CITY));
		gg_pubdir50_add(req, GG_PUBDIR50_BIRTHYEAR, inttostr((atoi(nfoGet(msg->p2 != 0, cnt, CNT_BORN)) & 0xFFFF0000) >> 16).c_str());
		int gender = atoi(nfoGet(msg->p2 != 0, cnt, CNT_GENDER));
		if (gender != GENDER_UNKNOWN) gg_pubdir50_add(req, GG_PUBDIR50_GENDER, gender == GENDER_MALE ? GG_PUBDIR50_GENDER_SET_MALE : GG_PUBDIR50_GENDER_SET_FEMALE);
		int res = gg_pubdir50(sess, req);
		gg_pubdir50_free(req);
		ICMessage(IMI_LONGEND, (int)&sdl);
		if (!res) {
			ICMessage(IMI_ERROR, (int)"Dane nie zosta³y zapisane na serwerze", 0);
			return 0;
		}
	}
	return 0;
}