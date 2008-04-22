#include "stdafx.h"
#include "GG.h"
#include "Controller.h"
#include "Helpers.h"
#include "Dialogs.h"
#include "UserList.h"

namespace GG {
	/*void onUserlistReply(gg_event * e) {
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
					importListFromString((String)userListBuffer);
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
	}*/

	string getSoundSetting(tCntId cnt, const std::string& type) {
		string result;
		//todo: zgodnie z nowym API to b�dzie wygl�da�o nieco inaczej� nie wiem tylko jak ;)
		string sound = Ctrl->DTgetStr(Tables::tableContacts, cnt, ("SOUND_" + type).c_str());
		if (sound == "") {
			return "0;";
		} else if (sound[0] == '!') {
			return "1;";
		} else {
			return "2;" + sound;
		}
	}

	string exportListToString() {
		string result = "";
		int cntCount = ICMessage(IMC_CNT_COUNT);
		const string separator(";");
		for (int i = 1; i < cntCount; i++) {
			Contact cnt(i);

			if ((cnt.getNet() == GG::net || cnt.getNet() == Net::none) && (!cnt.getDisplay().empty() || (cnt.getNet() != Net::none && !cnt.getUid().empty()))) {
				result += cnt.getName() + separator;
				result += cnt.getSurname() + separator;
				result += cnt.getNick() + separator;
				result += cnt.getDisplay() + separator;
				result += cnt.getString(Contact::colCellPhone) + separator;
				result += cnt.getString(Contact::colGroup) + separator;
				result += (cnt.getNet() == net ? cnt.getUidString().c_str() : "") + separator;
				result += cnt.getEmail() + separator;
				result += getSoundSetting(i, "newUser") + separator;
				result += getSoundSetting(i, "newMsg") + separator;
				result += (cnt.getStatus() & ST_HIDEMYSTATUS) != 0 ? "1" : "0" + separator;
				result += cnt.getString(Contact::colPhone);
				result += "\r\n";
			}
		}
		return result;
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

	void importListFromString(String userList) {
		deque<String> ignoreGroups; // ignorowane grupy

		userList.replace("\r", "");
		tStringVector lines;
		split(userList, "\n", lines, false);

		for (tStringVector::iterator line = lines.begin(); line != lines.end(); ++line) {
			if (line->empty() || (*line).substr(0, 1) == '#' || line->find(';') == string::npos)
				continue;

			tStringVector res;
			split(*line, ";", res, true);
			enum Fields {
				fName, fSurname, fNick, fDisplay, fCell, fGroup, fUin, fEmail, fSndOnline, fSndOnlinePath, fSndMsg, fSndMsgPath, fHide, fPhone
			};
			res.resize(fPhone + 1);
			if (!atoi(res[fUin].c_str())) res[fUin] = "";

			if (res[fDisplay].empty() && res[fUin].empty()) {
				string msg = "Z�y format pliku z kontaktami!\r\n\r\nPoni�sza linijka nie jest prawid�owym kontaktem:\r\n";
				msg += "[" + inttostr(line - lines.begin()) + "] \"" + (*line).substr(0, 100) + "\"";
				msg +="\r\nPrzerwa� dodawanie?";
				if (Ctrl->IMessage(&sIMessage_msgBox(IMI_CONFIRM, msg.c_str())))
					return;
			}

			int pos = -1;
			if (res[fUin].empty()) {
				// omijamy bug w 0.6.22.137
				int c = ICMessage(IMC_CNT_COUNT);
				for (int i = 1; i < c; i++) {
					if (_stricoll(res[fDisplay].c_str(), Contact(i).getDisplay().c_str()) == 0) {
						pos = Ctrl->DTgetID(DTCNT, i);
						break;
					}
				}
			} else {
				pos = ICMessage(IMC_CNT_FIND, net, (int)(res[fUin].c_str()));
			}
			if (pos <= 0) pos = ICMessage(IMC_CNT_ADD, res[fUin].empty() ? Net::none : net, (int)res[fUin].c_str());
			Contact cnt = Contact(pos);

			if (!res[fName].empty()) cnt.setName(res[fName]);
			if (!res[fSurname].empty()) cnt.setSurname(res[fSurname]);
			if (!res[fNick].empty()) cnt.setNick(res[fNick]);
			if (!res[fDisplay].empty()) cnt.setDisplay(res[fDisplay]);
			if (!res[fCell].empty()) cnt.setString(Contact::colCellPhone, res[fCell]);
			if (res[fGroup].find(',') != string::npos) {
				res[fGroup].erase(res[fGroup].find(','));
			}
			if (!res[fGroup].empty()) {
				if (!ICMessage(IMC_GRP_FIND, (int)res[fGroup].c_str()) && find(ignoreGroups.begin(), ignoreGroups.end(), res[fGroup]) == ignoreGroups.end()) {
					if (ICMessage(IMI_CONFIRM, (int)stringf("Kontakt %s by� zapisany z grup� %s.\n Doda� j�?", res[fDisplay].c_str(), res[fGroup].c_str()).c_str())) {
						ICMessage(IMC_GRP_ADD, (int)res[fGroup].c_str());
					} else {
						ignoreGroups.push_back(res[fGroup]);
						res[fGroup] = "";
					}
				}
			}
			if (!res[fGroup].empty()) cnt.setString(Contact::colGroup, res[fGroup].c_str());
			if (!res[fEmail].empty()) cnt.setEmail(res[fEmail].c_str());

			if (!res[fSndOnline].empty()) {
				string sound;
				if (res[fSndOnline] == "1")
					sound = "!";
				else if (res[fSndOnline] == "2")
					sound = res[fSndOnlinePath];
				//todo: Z nowym API to powinno wygl�da� inaczej.
				Ctrl->DTsetStr(DTCNT, pos, "SOUND_newUser", sound.c_str());
			}

			if (!res[fSndMsg].empty()) {
				string sound;
				if (res[fSndMsg] == "1")
					sound = "!";
				else if (res[fSndMsg] == "2")
					sound = res[fSndMsgPath];
				//todo: To te��
				Ctrl->DTsetStr(DTCNT, pos, "SOUND_newMsg", sound.c_str());
			}

			if (!res[fUin].empty() && !res[fHide].empty() && res[fHide] != "0")
				cnt.setStatus(ST_HIDEMYSTATUS);

			if (!res[fPhone].empty()) cnt.setString(Contact::colPhone, res[fPhone]);

			cnt.changed();
		}
		ICMessage(IMI_REFRESH_LST);
		ICMessage(IMI_INFORM, (int)"Kontakty zosta�y wczytane.");
	}

	//todo: Wymagaj� dzia�aj�cych event�w i f-cji je obs�uguj�cej
	/*unsigned int __stdcall exportListToServer(LPVOID lParam) {
		if (onUserListRequest != ulrNone) {
			ICMessage(IMI_ERROR, (int)"Poprzednia operacja na li�cie kontakt�w nie zosta�a zako�czona!");
			return 0;
		}
		if (!GG::check(1, 1, 0, 1)) return 0;

		onUserListRequest = lParam ? ulrClear : ulrPut;

		sDIALOG_long sdl;
		sdl.info = "Wysy�am";
		sdl.title = "GG";
		sdl.flag = DLONG_ASEND | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogSimpleCB;
		sdl.timeout = GETINT(CFG_TIMEOUT) * 2;
		ICMessage(IMI_LONGSTART, (int)&sdl);
		if (gg_userlist_request(sess, GG_USERLIST_PUT, lParam ? "" : exportListToString().c_str())) {
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

	unsigned int __stdcall importListFromServer(LPVOID lParam) {
		if (onUserListRequest != ulrNone) {
			ICMessage(IMI_ERROR, (int)"Poprzednia operacja na li�cie kontakt�w nie zosta�a zako�czona!");
			return 0;
		}
		if (!GG::check(1, 1, 0, 1)) return 0;

		onUserListRequest = ulrGet;
		userListBuffer = "";
		sDIALOG_long sdl;
		sdl.info = "Wczytuj�";
		sdl.title = "GG";
		sdl.flag = DLONG_ARECV | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogSimpleCB;
		sdl.threadId = ggThreadId;
		sdl.timeout = GETINT(CFG_TIMEOUT) * 2;
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
		if (ICMessage(IMI_CONFIRM, (int)"Wszystkie kontakty sieci GG zostan� od�wie�one wg. KataloguPublicznego sieci\nKontynuowa�?",MB_TASKMODAL|MB_YESNO)==IDNO)
			return 0;
		sDIALOG_long sdl;
		sdl.info = "Wczytuj�";
		sdl.title = "GG";
		sdl.flag = DLONG_ARECV|DLONG_ONLY|DLONG_CANCEL;
		sdl.progress = 0;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogSimpleCB;
		sdl.timeout = GETINT(CFG_TIMEOUT) * 2;
		ICMessage(IMI_LONGSTART, (int)&sdl);
		int a = ICMessage(IMC_CNT_COUNT);
		for (int i = 1; i < a; i++) {
			if (sdl.cancel) break;
			if (Contact(i).getNet() == net) {
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
	}*/
};