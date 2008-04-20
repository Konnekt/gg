#include "StdAfx.h"
#include "Controller.h"
#include "Helpers.h"
#include "Dialogs.h"
#include "UserList.h"

namespace GG {
	bool __stdcall GG::disconnectDialogCB(sDIALOG_long*sd) {
		IMLOG("Aktualny Socket w GG - %x @ %x", gg_thread_socket(sd->threadId, 0), sd->threadId);
		if (gg_thread_socket(sd->threadId, 0)) {
			shutdown(gg_thread_socket(sd->threadId, 0), -1);
			if (!gg_thread_socket(sd->threadId, -1))
				IMLOG("closesocket.Error WSA = %d", WSAGetLastError());
		}
		return true;
	}

	bool __stdcall GG::timeoutDialogCB(int type, sDIALOG_long*sd) {
		switch (type) {
			case TIMEOUTT_START: break;
			case TIMEOUTT_END: {
				/*if (sd->timeoutParam) {
					delete (timeoutDialogParam*)sd->timeoutParam;
				}
				sd->timeoutParam=0; 
				ioctlsocket(gg_thread_socket(sd->threadId, 0), FIONBIO, 0);*/
				break;
			} case TIMEOUTT_TIMEOUT: {
				disconnectDialogCB(sd);
				ICMessage((sd->flag & DLONG_NODLG) ? IMC_LOG : IMI_ERROR, (int)"Up�yn�� limit czasu po��czenia.", 0);
				return true;
			} case TIMEOUTT_CHECK: {
				/*gg_thread_socket_lock();
				gg_thread* info = gg_thread_find(sd->threadId, 0, 0);
				if (!info) {
					gg_thread_socket_unlock();
					return 0;
				}
				int r = WSAWaitForMultipleEvents(1, &info->event, 0, 0, 0);

				gg_thread_socket_unlock();
				return r == WSA_WAIT_TIMEOUT;*/
			}
			return 0; /*todo: Wymy�le� co� na timeout!*/
		}
		return true;
	}

	bool getToken(const string& title, const string& info, string& tokenID, string& tokenVal) {
		gg_http* http = gg_token(0);
		if (!http) {
			ICMessage(IMI_ERROR, (int)"Wyst�pi� b��d podczas pobierania tokena. Sprawd� po��czenie i spr�buj ponownie.");
			return false;
		}
		struct gg_token* token = (struct gg_token*)http->data;
		tokenID = token->tokenid;

		ICMessage(IMC_RESTORECURDIR);
		string filename = (string)(char*)ICMessage(IMC_TEMPDIR) + "\\gg_token.gif";
		ofstream file(filename.c_str(), ios_base::out | ios_base::trunc | ios_base::binary);
		file.write(http->body, http->body_size);
		if (file.fail()) {
			ICMessage(IMI_ERROR, (int)"Wyst�pi� b��d podczas zapisywania tokena.");
			file.close();
			return false;
		}
		file.close();

		sDIALOG_token dt;
		dt.title = title.c_str();
		dt.info = info.c_str();
		string URL = "file://" + filename;
		dt.imageURL = URL.c_str();
		ICMessage(IMI_DLGTOKEN, (int)&dt);
		tokenVal = dt.token;
		gg_token_free(http);
		return !tokenVal.empty();
	}

	//todo: Poni�sze funkcje mog� informowa� rodzajach b��d�w. Informacje wyci�gamy przez f-cj� debugow� - inaczej si� chyba nie da (czyli czekamy na Winthuxa i naprawienie nowego libgadu�).
	unsigned __stdcall createGGAccount(LPVOID lParam) {
		if (ICMessage(IMI_CONFIRM, (int)"Zostanie za�o�one nowe konto w sieci Gadu-Gadu�.\nKontynuowa�?", MB_TASKMODAL | MB_YESNO) == IDNO) 
			return 0;

		sDIALOG_access sda;
		sda.save = 1;
		sda.title = "GG - nowe konto [krok 1/3]";
		sda.info = "Podaj has�o do nowego konta.";
		if (!ICMessage(IMI_DLGSETPASS, (int)&sda, 1) || !*sda.pass)
			return 0;
		string password = sda.pass;
		bool save = sda.save;

		sDIALOG_enter sde;
		sde.title = "GG - nowe konto [krok 2/3]";
		sde.info = "Podaj sw�j adres email, na kt�ry b�dzie mo�na kiedy� przes�a� has�o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde) || !*sde.value)
			return 0;
		string email = sde.value;

		string tokenID;
		string tokenVal;
		if (!getToken("GG - nowe konto [krok 3/3]", "Wpisz tekst znajduj�cy si� na poni�szym obrazku.", tokenID, tokenVal))
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - zak�adanie konta";
		sdl.info = "Komunikuj� si� z serwerem�";
		sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		Singleton<Controller>::getInstance()->setProxy();
		gg_http* gghttp = gg_register3(email.c_str(), password.c_str(), tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp || gghttp->state != GG_STATE_DONE) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)stringf("Wyst�pi� b��d podczas zak�adania konta. Sprawd� po��czenie i spr�buj ponownie.").c_str(), 0);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		gg_register_watch_fd(gghttp);
		gg_pubdir* pd = (gg_pubdir*)gghttp->data;
		if (pd->success) {
			UIActionCfgSetValue(sUIAction(CFG::group, CFG::login), inttostr(pd->uin).c_str());
			SETINT(CFG::login, pd->uin);
			UIActionCfgSetValue(sUIAction(CFG::group, CFG::password), save ? password.c_str() : "");
			SETSTR(CFG::password, save ? password.c_str() : "");
			ICMessage(IMC_SAVE_CFG);
		}

		ICMessage(IMI_INFORM,
			pd->success ?
			(int)stringf("Konto zosta�o za�o�one!\nTw�j numer UID to %d", pd->uin).c_str()
			:
			(int)stringf("Wyst�pi� b��d podczas zak�adania konta. Sprawd� po��czenie i spr�buj ponownie.").c_str()
		);
		gg_register_free(gghttp);
		return 0;
	}

	unsigned __stdcall removeGGAccount (LPVOID lParam) {
		if (ICMessage(IMI_CONFIRM, (int)"Twoje konto na gadu-gadu zostanie usuni�te!\nKontynuowa�?", MB_TASKMODAL | MB_YESNO) == IDNO) 
			return 0;

		sDIALOG_enter sde;
		sde.title = "GG - usuwanie konta [krok 1/3]";
		sde.info = "Potwierd� numer konta";
		if (!ICMessage(IMI_DLGENTER, (int)&sde, 1))
			return 0;
		string login = sde.value;

		sDIALOG_access sda;
		sda.title = "GG - usuwanie konta [krok 2/3]";
		sda.info = "Podaj has�o do konta";
		if (!ICMessage(IMI_DLGPASS, (int)&sda))
			return 0;
		string password = sda.pass;

		string tokenID;
		string tokenVal;
		if (!getToken("GG - usuwanie konta [krok 3/3]", "Wpisz tekst znajduj�cy si� na poni�szym obrazku.", tokenID, tokenVal))
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - usuwanie konta";
		sdl.info = "Komunikuj� si� z serwerem�";
		sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		Singleton<Controller>::getInstance()->setProxy();
		gg_http* gghttp = gg_unregister3(atoi(login.c_str()), password.c_str(), tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)stringf("Wyst�pi� b��d podczas usuwania konta. Sprawd� po��czenie i spr�buj ponownie.").c_str(), 0);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		gg_pubdir* pd = (gg_pubdir*)gghttp->data;
		if (pd->success) {
			UIActionCfgSetValue(sUIAction(CFG::group, CFG::login), "");
			SETSTR(CFG::login, "");
			UIActionCfgSetValue(sUIAction(CFG::group, CFG::password), "");
			SETSTR(CFG::password, "");
		}

		ICMessage(IMI_INFORM,
			pd->success ?
			(int)"Konto zosta�o usuni�te�"
			:
			(int)"Wyst�pi� b��d podczas usuwania konta. Sprawd� po��czenie i spr�buj ponownie."
		);
		gg_unregister_free(gghttp);
		return 0;
	}

	unsigned __stdcall changePassword(LPVOID lParam) {
		if (ICMessage(IMI_CONFIRM, (int)"Czy na pewno chcesz zmieni� has�o?", MB_TASKMODAL | MB_YESNO) == IDNO)
			return 0;

		sDIALOG_access sda;
		string oldPassword;
		sda.save = 0;
		sda.flag = 0;
		sda.title = "GG - zmiana has�a [1/4]";
		sda.info = "Podaj aktualne has�o.";
		if (!ICMessage(IMI_DLGPASS, (int)&sda, 0))
			return 0;
		oldPassword = sda.pass;

		sda.pass = "";
		sda.save = 1;
		sda.flag = DFLAG_SAVE;
		sda.title = "GG - zmiana has�a [2/4]";
		sda.info = "Podaj nowe has�o do twojego konta.";
		if (!ICMessage(IMI_DLGSETPASS, (int)&sda, 1)) 
			return 0;
		string password = sda.pass;
		bool save = sda.save;

		sDIALOG_enter sde;
		sde.title = "GG - zmiana has�a [3/4]";
		sde.info = "Podaj sw�j adres email, na kt�ry b�dzie mo�na kiedy� przes�a� has�o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde)) return 0;
		string email = sde.value;

		string tokenID;
		string tokenVal;
		if (!GG::getToken("GG - zmiana has�a [krok 4/4]", "Wpisz tekst znajduj�cy si� na poni�szym obrazku.", tokenID, tokenVal))
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - zmiana has�a";
		sdl.info = "Komunikuj� si� z serwerem�";
		sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		Singleton<Controller>::getInstance()->setProxy();
		int login = GETINT(CFG::login);
		gg_http* gghttp = gghttp = gg_change_passwd4(login, email.c_str(), oldPassword.c_str(), password.c_str(), tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)stringf("Wyst�pi� b��d. Sprawd� po��czenie, oraz czy poda�e� prawid�owy numer konta i has�o. Nast�pnie spr�buj ponownie.").c_str(),MB_TASKMODAL|MB_OK);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		gg_pubdir* pd = (gg_pubdir*)gghttp->data;
		if (pd->success) {
			UIActionCfgSetValue(sUIAction(CFG::group, CFG::password), save ? password.c_str():"");
			SETSTR(CFG::password, save ? password.c_str() : "");
		}

		ICMessage(IMI_INFORM,
			pd->success ?
			(int)stringf("Has�o dla konta %d zosta�o zmienione!", login).c_str()
			:
			(int)stringf("Wyst�pi� b��d podczas zmiany has�a dla konta %d . Sprawd� po��czenie, oraz czy poda�e� prawid�owy numer konta i has�o i spr�buj ponownie.", login).c_str()
		);
		gg_free_change_passwd(gghttp);
		return 0;
	}

	unsigned __stdcall remindPassword(LPVOID lParam) {
		sDIALOG_enter sde;
		sde.title = "GG - przypomnienie has�a [1/2]";
		sde.info = "Podaj adres email, kt�ry wpisa�e� podczas zak�adania konta. Na ten email otrzymasz has�o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde))
			return 0;
		string email = sde.value;

		string tokenID;
		string tokenVal;
		if (!GG::getToken("GG - przypomnienie has�a [2/2]", "Wpisz tekst znajduj�cy si� na poni�szym obrazku.", tokenID, tokenVal))
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - przypomnienie has�a";
		sdl.info = "Komunikuj� si� z serwerem�";
		sdl.flag = DLONG_AINET | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		Singleton<Controller>::getInstance()->setProxy();
		gg_http* gghttp = gg_remind_passwd3(GETINT(CFG::login), email.c_str(), tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)"Wyst�pi� b��d! Sprawd� po��czenie, oraz czy poda�e� prawid�owy numer konta/adres email i spr�buj ponownie.", MB_TASKMODAL | MB_OK);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		gg_pubdir* pd = (gg_pubdir*)gghttp->data;

		ICMessage(IMI_INFORM,
			pd->success ?
			(int)"Has�o zosta�o wys�ane!"
			:
			(int)"Wyst�pi� b��d! Sprawd� po��czenie, oraz czy poda�e� prawid�owy numer konta i spr�buj ponownie."
		);
		gg_remind_passwd_free(gghttp);
		return 0;
	}

	void importList() {
		sDIALOG_choose sd;
		sd.title = "Import listy kontakt�w";
		sd.info = "Wybierz sk�d importowa�.\nZostan� dodane tylko brakuj�ce kontakty.";
		sd.flag = DFLAG_CANCEL;
		sd.items = "Z pliku\nZ Serwera";
		int r = ICMessage(IMI_DLGBUTTONS, (int)&sd);
		IMLOG("- Import type %d", r);

		switch (r) {
			case 1: {
				OPENFILENAME of;
				memset(&of, 0, sizeof(of));
				of.lStructSize = sizeof(of) - 12;
				of.lpstrFilter = "Txt\0*.txt\0*.*\0*.*\0";
				of.lpstrFile = new char[10000];
				strcpy(of.lpstrFile, "");
				of.nMaxFile = 10000;
				of.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
				of.lpstrDefExt = "txt";

				if (GetOpenFileName(&of)) {
					ifstream file(of.lpstrFile, ios_base::out);
					if (file.is_open()) {
						string fileContents;
						//todo: Nie wiem czy to dzia�a.
						file >> fileContents;
						if (file.good()) {
							importListFromString((String)fileContents);
						} else {
							ICMessage(IMI_ERROR, (int)"Nie mog�em wczyta� pliku!", MB_TASKMODAL | MB_OK);
						}
						file.close();
					} else {
						ICMessage(IMI_ERROR, (int)"Nie mog�em wczyta� pliku!", MB_TASKMODAL | MB_OK);
					}
				}
				delete[] of.lpstrFile;
				break;
			} case 2: {
				//CloseHandle(Ctrl->BeginThread("importListFromServer", 0, 0, importListFromServer, 0, 0, 0));
				break;
			}
		}
		ICMessage(IMC_SAVE_CNT);
	}

	void exportList() {
		sDIALOG_choose sd;
		sd.title = "Export listy kontakt�w";
		sd.info = "Wybierz dok�d exportowa�.";
		sd.flag = DFLAG_CANCEL;
		sd.items = "Do pliku\nNa Serwer GG\nUsu� list� z serwera";
		int r = ICMessage(IMI_DLGBUTTONS, (int)&sd);
		IMLOG("- Export type %d", r);

		switch (r) {
			case 1: {
				OPENFILENAME of;
				memset(&of, 0, sizeof(of));
				of.lStructSize = sizeof(of) - 12;
				of.lpstrFilter = "Txt\0*.txt\0*.*\0*.*\0";
				char* buff = new char[10000];
				strcpy(buff, "kontakty");
				of.lpstrFile = buff;
				of.nMaxFile = 10000;
				of.nFilterIndex = 1;
				of.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
				of.lpstrDefExt = "txt";

				if (GetSaveFileName(&of)) {
					ofstream file(of.lpstrFile, ios_base::out);
					if (file.is_open()) {
						file << exportListToString();
						if (file.good()) {
							ICMessage(IMI_INFORM, (int)"Kontakty zosta�y zapisane.");
						} else {
							ICMessage(IMI_ERROR, (int)"Nie mog�em zapisa� do pliku!", MB_TASKMODAL | MB_OK);
						}
						file.close();
					} else {
						ICMessage(IMI_ERROR, (int)"Nie mog�em zapisa� do pliku!", MB_TASKMODAL | MB_OK);
					}
				}
				delete[] buff;
				break;
			} case 2: {
				//CloseHandle(Ctrl->BeginThread("exportListToServer", 0, 0, exportListToServer, 0, 0, 0));
				break;
			} case 3: {
				//CloseHandle(Ctrl->BeginThread("exportListToServer", 0, 0, exportListToServer, (void*)1, 0, 0));
				break;
			}
		}
	}
}