#include "stdafx.h"
#include "GG.h"
#include "Controller.h"

using Stamina::stringf;

void GG::onUserlistReply(gg_event * e) {
	switch(e->event.userlist.type) {
		case GG_USERLIST_GET_MORE_REPLY: {
			if (!e->event.userlist.reply || !*e->event.userlist.reply)
				break;
		} case GG_USERLIST_GET_REPLY: {
			if (onUserListRequest != ulrGet) return;
			if (e->event.userlist.reply)
				userListBuffer+= e->event.userlist.reply;
			if (e->event.userlist.type == GG_USERLIST_GET_MORE_REPLY)
				break;
			if (userListBuffer.empty()) {
				ICMessage(IMI_INFORM, (int)"Na serwerze nie ma kontakt�w!");
			} else {
				setUserList((char*)userListBuffer.c_str());
				userListBuffer = "";
				ICMessage(IMI_REFRESH_LST);
				ICMessage(IMI_INFORM, (int)"Kontakty zosta�y pobrane.");
			}
			onUserListRequest = ulrDone;
			break;
		} case GG_USERLIST_PUT_REPLY: {
			switch(onUserListRequest) {
				case ulrClear: ICMessage(IMI_INFORM, (int)"Kontakty zosta�y usuni�te z serwera."); break;
				case ulrPut: ICMessage(IMI_INFORM, (int)"Kontakty zosta�y wys�ane."); break;
				default: return;
			}
			onUserListRequest = ulrDone;
			break;
		}
	};
}

/*
imi�;nazwisko;pseudonim;wy�wietlane;telefon_kom�rkowy;grupa;uin;adres_email;dost�
pny;�cie�ka_dost�pny;wiadomo��;�cie�ka_wiadomo��;ukrywanie;telefon_domowy

Funkcje mniej oczywistych p�l to:

	* dost�pny okre�la d�wi�ki zwi�zane z pojawieniem si� danej osoby i przyjmuje warto�ci 0 (u�yte zostan� ustawienia globalne), 1 (d�wi�k powiadomienia zostanie wy��czony), 2 (zostanie odtworzony plik okre�lony w polu �cie�ka_dost�pny).
	* wiadomo�� dzia�a podobnie jak dost�pny, ale okre�la d�wi�k dla przychodz�cej wiadomo�ci.
	* ukrywanie okre�la czy b�dziemy dost�pni (0) czy niedost�pni (1) dla danej osoby w trybie tylko dla znajomych.

Pole niewype�nione mo�e zosta� puste, a w przypadku p�l liczbowych, przyj�� warto�� 0. 
*/

string getSoundSetting(tCntId cnt, const std::string& type) {
	string result;
	string sound = Ctrl->DTgetStr(DTCNT, cnt, ("SOUND_" + type).c_str());
	if (sound == "") {
		return "0;";
	} else if (sound[0] == '!') {
		return "1;";
	} else {
		return "2;" + sound;
	}
}

string GG::getUserList () {
	string str = "";
	int a = ICMessage(IMC_CNT_COUNT);
	for (int i = 1; i < a; i++) {
		int net = GETCNTI(i, CNT_NET);
		// Bierze kontakty GG, bez sieci i tylko takie, kt�re maj� warto�� display, albo UID
		if ((net == GG::Net || net == NET_NONE) && (GETCNTC(i, CNT_DISPLAY)[0] || (net && GETCNTC (i, CNT_UID)))) {
			string separator = ";";
			str += GETCNTC (i, CNT_NAME) + separator;
			str += GETCNTC (i, CNT_SURNAME) + separator;
			str += GETCNTC (i, CNT_NICK) + separator;
			str += GETCNTC (i, CNT_DISPLAY) + separator;
			str += GETCNTC (i, CNT_CELLPHONE) + separator;
			str += GETCNTC (i, CNT_GROUP) + separator;
			str += (net == GG::Net ? GETCNTC (i, CNT_UID) : "") + separator;
			str += GETCNTC (i, CNT_EMAIL) + separator;
			str += getSoundSetting(i, "newUser") + separator;
			str += getSoundSetting(i, "newMsg") + separator;
			str += ((GETCNTI(i, CNT_STATUS ) & ST_HIDEMYSTATUS) != 0 ? "1" : "0") + separator;
			str += GETCNTC (i, CNT_PHONE);
			str += "\r\n";
		}
	}
	return str;
}

/*
imi�;nazwisko;pseudonim;wy�wietlane;telefon_kom�rkowy;grupa;uin;adres_email;dost�
pny;�cie�ka_dost�pny;wiadomo��;�cie�ka_wiadomo��;ukrywanie;telefon_domowy

Funkcje mniej oczywistych p�l to:

	* dost�pny okre�la d�wi�ki zwi�zane z pojawieniem si� danej osoby i przyjmuje warto�ci 0 (u�yte zostan� ustawienia globalne), 1 (d�wi�k powiadomienia zostanie wy��czony), 2 (zostanie odtworzony plik okre�lony w polu �cie�ka_dost�pny).
	* wiadomo�� dzia�a podobnie jak dost�pny, ale okre�la d�wi�k dla przychodz�cej wiadomo�ci.
	* ukrywanie okre�la czy b�dziemy dost�pni (0) czy niedost�pni (1) dla danej osoby w trybie tylko dla znajomych.

Pole niewype�nione mo�e zosta� puste, a w przypadku p�l liczbowych, przyj�� warto�� 0. 
*/

int GG::setUserList(char* _userList) {
	CStdString userList = _userList;
	userList.Replace("\r", "");
	Stamina::tStringVector lines;
	Stamina::split(userList, "\n", lines, false);
	
	deque <CStdString> ignore_group; // ignorowane grupy
	for (Stamina::tStringVector::iterator line = lines.begin(); line != lines.end(); ++line) {
		if (line->empty() || (*line)[0] == '#')
			continue;
		IMDEBUG(DBG_DEBUG, "userList line = \"%s\"", line->c_str());
		Stamina::tStringVector res;
		if (line->find(';') == string::npos) {
			/*
			if (!strchr(buf, ';')) {
				if (!(res[3] = strchr(buf, ' ')))
				continue;
				res[6]=buf;
				res[3][0]=0;
				res[3]++;
			*/
			continue; // ooola� na razie...
		}
		Stamina::split(*line, ";", res, true);
		enum Fields {
			fName, fSurname, fNick, fDisplay, fCell, fGroup, fUin, fEmail, fSndOnline, fSndOnlinePath, fSndMsg, fSndMsgPath, fHide, fPhone
		};
		res.resize(fPhone + 1);
		/*
		0 - imi�; 1 - nazwisko; 2 - pseudonim; 3 - wy�wietlane; 4 - telefon_kom�rkowy;
		5 - grupa; 6 - uin; 7 - adres_email; 8 - dost�pny; 9 - �cie�ka_dost�pny;
		10 - wiadomo��; 11 - �cie�ka_wiadomo��; 12 - ukrywanie; 13 - telefon_domowy;
		*/
		if (!atoi(res[fUin].c_str())) res[fUin] = "";

		if (res[fDisplay].empty() && res[fUin].empty()) { // To jakis zly format!
			CStdString msg = "Z�y format pliku z kontaktami!\r\n\r\nPoni�sza linijka nie jest prawid�owym kontaktem:\r\n";
			msg += "[" + inttostr(line - lines.begin()) + "] \"" + (*line).substr(0,100) + "\"";
			msg+="\r\nPrzerwa� dodawanie?";
			if (Ctrl->IMessage(&sIMessage_msgBox(IMI_CONFIRM, msg)))
				return 0;
		}
		int pos = -1;
		if (res[fUin].empty()) {
			// omijamy bug w 0.6.22.137
			int c = ICMessage(IMC_CNT_COUNT);
			for (int i = 1; i < c; i++) {
				if (_stricoll(res[fDisplay].c_str(), GETCNTC(i, CNT_DISPLAY)) == 0) {
					pos = Ctrl->DTgetID(DTCNT, i);
					break;
				}
			}
		} else {
			pos = ICMessage(IMC_CNT_FIND, GG::Net, (int)(res[fUin].c_str()));
		}
		if (pos <= 0) pos = ICMessage(IMC_CNT_ADD, res[fUin].empty() ? NET_NONE : GG::Net, (int)res[fUin].c_str());

		if (res[fName].empty() == false) SETCNTC(pos, CNT_NAME, res[fName].c_str());
		if (res[fSurname].empty() == false) SETCNTC(pos, CNT_SURNAME, res[fSurname].c_str());
		if (res[fNick].empty() == false) SETCNTC(pos, CNT_NICK, res[fNick].c_str());
		if (res[fDisplay].empty() == false) SETCNTC(pos, CNT_DISPLAY, res[fDisplay].c_str());
		if (res[fCell].empty() == false) SETCNTC(pos, CNT_CELLPHONE, res[fCell].c_str());
		if (res[fGroup].find(',') != string::npos) {
			res[fGroup].erase(res[fGroup].find(','));
		}
		if (res[fGroup].empty() == false) {
			if (!ICMessage(IMC_GRP_FIND, (int)res[fGroup].c_str()) && std::find(ignore_group.begin(), ignore_group.end(), res[fGroup]) == ignore_group.end()) {
				if (ICMessage(IMI_CONFIRM, (int)stringf("Kontakt %s by� zapisany z grup� %s.\n Doda� j�?", res[fDisplay].c_str(), res[fGroup].c_str()).c_str())) {
					ICMessage(IMC_GRP_ADD, (int)res[fGroup].c_str());
				} else {
					ignore_group.push_back(res[fGroup]); // Dorzucamy grupe do listy ignorowanych...
					res[fGroup] = ""; // Skoro nie zgadzamy sie na utworzenie grupy, trzeba ja wywalic!
				}
			}
		}
		if (res[fGroup].empty() == false) SETCNTC(pos, CNT_GROUP, res[fGroup].c_str());
		if (res[fEmail].empty() == false) SETCNTC(pos, CNT_EMAIL, res[fEmail].c_str());

		if (res[fSndOnline].empty() == false) {
			string sound;
			if (res[fSndOnline] == "1")
				sound = "!";
			else if (res[fSndOnline] == "2")
				sound = res[fSndOnlinePath];
			Ctrl->DTsetStr(DTCNT, pos, "SOUND_newUser", sound.c_str());
		}

		if (res[fSndMsg].empty() == false) {
			string sound;
			if (res[fSndMsg] == "1")
				sound = "!";
			else if (res[fSndMsg] == "2")
				sound = res[fSndMsgPath];
			Ctrl->DTsetStr(DTCNT, pos, "SOUND_newMsg", sound.c_str());
		}

		if (res[fUin].empty() == false && res[fHide].empty() == false)
			SETCNTI(pos, CNT_STATUS, res[fHide] == "0" ? 0 : ST_HIDEMYSTATUS, ST_HIDEMYSTATUS);

		if (res[fPhone].empty() == false) SETCNTC(pos, CNT_PHONE, res[fPhone].c_str());

		ICMessage(IMC_CNT_CHANGED, pos);
	}
	return 0;
}

unsigned int __stdcall GG::doListExport(LPVOID lParam) {
	if (onUserListRequest != ulrNone) {
		ICMessage(IMI_ERROR, (int)"Poprzednia operacja na li�cie kontakt�w nie zosta�a zako�czona!");
		return 0;
	}

	if (!GG::check(1, 1, 0, 1)) return 0;
	//if (!IMessage(IMI_CONFIRM, 0, 0, (int)"Kontakty zostan� wys�ane na serwer.\nKontynuowa�?")) _endthread();
	onUserListRequest = lParam ? ulrClear : ulrPut;

	sDIALOG_long sdl;
	sdl.info = "Wysy�am";
	sdl.title = "GG";
	sdl.flag = DLONG_ASEND | DLONG_CANCEL;
	sdl.cancelProc = disconnectDialogCB;
	sdl.timeoutProc = timeoutDialogSimpleCB;
	sdl.timeout = PUBDIR_TIMEOUT;
	//sdl.threadId = ggThreadId;
	ICMessage(IMI_LONGSTART, (int)&sdl);
	string str = lParam?string(""):getUserList();
	//IMLOG("CNTS %s", str.c_str());
	if (gg_userlist_request(sess, GG_USERLIST_PUT, str.c_str())) {
		ICMessage(IMI_ERROR, (int)"Wysy�anie nie powiod�o si�!",0);
	} else {
		while (ggThread && onUserListRequest != ulrDone) {
			Sleep(100);
			if (sdl.cancel) break;
		}
	}
	onUserListRequest = ulrNone;
	ICMessage(IMI_LONGEND, (int)&sdl);
	return 0;
}

unsigned int __stdcall GG::doListImport(LPVOID lParam) {
	if (onUserListRequest != ulrNone) {
		ICMessage(IMI_ERROR, (int)"Poprzednia operacja na li�cie kontakt�w nie zosta�a zako�czona!");
		return 0;
	}
	if (!GG::check(1, 1, 0, 1)) return 0;
	//if (!IMessage(IMI_CONFIRM, 0, 0, (int)"Kontakty zostan� wczytane z serwera, zostan� dodane nowe kontakty i zaktualizowane istniej�ce (nic nie zostanie usuni�te!)\nKontynuowa�?")) _endthread();
	onUserListRequest = ulrGet;
	userListBuffer = "";
	sDIALOG_long sdl;
	sdl.info = "Wczytuj�";
	sdl.title = "GG";
	sdl.flag = DLONG_ARECV | DLONG_CANCEL;
	sdl.cancelProc = disconnectDialogCB;
	sdl.timeoutProc = timeoutDialogSimpleCB;
	sdl.threadId = ggThreadId;
	sdl.timeout = PUBDIR_TIMEOUT;
	ICMessage(IMI_LONGSTART, (int)&sdl);
	if (gg_userlist_request(sess, GG_USERLIST_GET, 0)) {
		ICMessage(IMI_ERROR, (int)"Pobieranie nie powiod�o si�!", 0);
	} else {
		while (ggThread && onUserListRequest != ulrDone) {
			Sleep(100);
			if (sdl.cancel) break;
		}
	}
	onUserListRequest = ulrNone;
	ICMessage(IMI_LONGEND, (int)&sdl);
	return 0;
}

unsigned int __stdcall GG::dlgListRefresh(LPVOID lParam) {
	if (!GG::check(1, 0, 1, 1)) 
		return 0;
	if (IMessage(IMI_CONFIRM, 0, 0, (int)"Wszystkie kontakty sieci GG zostan� od�wie�one wg. KataloguPublicznego sieci\nKontynuowa�?",MB_TASKMODAL|MB_YESNO)==IDNO)
		return 0;
	sDIALOG_long sdl;
	sdl.info = "Wczytuj�";
	sdl.title = "GG";
	sdl.flag = DLONG_ARECV|DLONG_ONLY|DLONG_CANCEL;
	sdl.progress = 0;
	sdl.cancelProc = disconnectDialogCB;
	sdl.timeoutProc = timeoutDialogSimpleCB;
	sdl.timeout = PUBDIR_TIMEOUT;
	ICMessage(IMI_LONGSTART, (int)&sdl);
	int a = ICMessage(IMC_CNT_COUNT);
	for (int i = 1; i < a; i++) {
		if (sdl.cancel) break;
		if (GETCNTI (i, CNT_NET) == GG::Net) {
			IMLOG("IM_CNT");
			IMessageDirect(IM_CNT_DOWNLOAD, 0, i);
			IMLOG("IM_CNT.done");
			sdl.progress = (i*100) / (a-1);
			ICMessage(IMI_LONGSET, (int)&sdl, DSET_PROGRESS);
		}
	}
	IMLOG("______________");
	ICMessage(IMI_LONGEND, (int)&sdl);
	return 0;
}
