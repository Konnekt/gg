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
				ICMessage((sd->flag & DLONG_NODLG) ? IMC_LOG : IMI_ERROR, (int)"Up³yn¹³ limit czasu po³¹czenia.", 0);
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
			return 0; /*todo: Wymyœleæ coœ na timeout!*/
		}
		return true;
	}

	bool getToken(const string& title, const string& info, string& tokenID, string& tokenVal) {
		gg_http* http = gg_token(0);
		if (!http) {
			ICMessage(IMI_ERROR, (int)"Wyst¹pi³ b³¹d podczas pobierania tokena. SprawdŸ po³¹czenie i spróbuj ponownie.");
			return false;
		}
		struct gg_token* token = (struct gg_token*)http->data;
		tokenID = token->tokenid;

		ICMessage(IMC_RESTORECURDIR);
		string filename = (string)(char*)ICMessage(IMC_TEMPDIR) + "\\gg_token.gif";
		ofstream file(filename.c_str(), ios_base::out | ios_base::trunc | ios_base::binary);
		file.write(http->body, http->body_size);
		if (file.fail()) {
			ICMessage(IMI_ERROR, (int)"Wyst¹pi³ b³¹d podczas zapisywania tokena.");
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

	//todo: Poni¿sze funkcje mog¹ informowaæ rodzajach b³êdów. Informacje wyci¹gamy przez f-cjê debugow¹ - inaczej siê chyba nie da (czyli czekamy na Winthuxa i naprawienie nowego libgadu…).
	unsigned __stdcall createGGAccount(LPVOID lParam) {
		if (ICMessage(IMI_CONFIRM, (int)"Zostanie za³o¿one nowe konto w sieci Gadu-Gadu™.\nKontynuowaæ?", MB_TASKMODAL | MB_YESNO) == IDNO) 
			return 0;

		sDIALOG_access sda;
		sda.save = 1;
		sda.title = "GG - nowe konto [krok 1/3]";
		sda.info = "Podaj has³o do nowego konta.";
		if (!ICMessage(IMI_DLGSETPASS, (int)&sda, 1) || !*sda.pass)
			return 0;
		string password = sda.pass;
		bool save = sda.save;

		sDIALOG_enter sde;
		sde.title = "GG - nowe konto [krok 2/3]";
		sde.info = "Podaj swój adres email, na który bêdzie mo¿na kiedyœ przes³aæ has³o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde) || !*sde.value)
			return 0;
		string email = sde.value;

		string tokenID;
		string tokenVal;
		if (!getToken("GG - nowe konto [krok 3/3]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.", tokenID, tokenVal))
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - zak³adanie konta";
		sdl.info = "Komunikujê siê z serwerem…";
		sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		Singleton<Controller>::getInstance()->setProxy();
		gg_http* gghttp = gg_register3(email.c_str(), password.c_str(), tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp || gghttp->state != GG_STATE_DONE) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)stringf("Wyst¹pi³ b³¹d podczas zak³adania konta. SprawdŸ po³¹czenie i spróbuj ponownie.").c_str(), 0);
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
			(int)stringf("Konto zosta³o za³o¿one!\nTwój numer UID to %d", pd->uin).c_str()
			:
			(int)stringf("Wyst¹pi³ b³¹d podczas zak³adania konta. SprawdŸ po³¹czenie i spróbuj ponownie.").c_str()
		);
		gg_register_free(gghttp);
		return 0;
	}

	unsigned __stdcall removeGGAccount (LPVOID lParam) {
		if (ICMessage(IMI_CONFIRM, (int)"Twoje konto na gadu-gadu zostanie usuniête!\nKontynuowaæ?", MB_TASKMODAL | MB_YESNO) == IDNO) 
			return 0;

		sDIALOG_enter sde;
		sde.title = "GG - usuwanie konta [krok 1/3]";
		sde.info = "PotwierdŸ numer konta";
		if (!ICMessage(IMI_DLGENTER, (int)&sde, 1))
			return 0;
		string login = sde.value;

		sDIALOG_access sda;
		sda.title = "GG - usuwanie konta [krok 2/3]";
		sda.info = "Podaj has³o do konta";
		if (!ICMessage(IMI_DLGPASS, (int)&sda))
			return 0;
		string password = sda.pass;

		string tokenID;
		string tokenVal;
		if (!getToken("GG - usuwanie konta [krok 3/3]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.", tokenID, tokenVal))
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - usuwanie konta";
		sdl.info = "Komunikujê siê z serwerem…";
		sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		Singleton<Controller>::getInstance()->setProxy();
		gg_http* gghttp = gg_unregister3(atoi(login.c_str()), password.c_str(), tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)stringf("Wyst¹pi³ b³¹d podczas usuwania konta. SprawdŸ po³¹czenie i spróbuj ponownie.").c_str(), 0);
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
			(int)"Konto zosta³o usuniête…"
			:
			(int)"Wyst¹pi³ b³¹d podczas usuwania konta. SprawdŸ po³¹czenie i spróbuj ponownie."
		);
		gg_unregister_free(gghttp);
		return 0;
	}

	unsigned __stdcall changePassword(LPVOID lParam) {
		if (ICMessage(IMI_CONFIRM, (int)"Czy na pewno chcesz zmieniæ has³o?", MB_TASKMODAL | MB_YESNO) == IDNO)
			return 0;

		sDIALOG_access sda;
		string oldPassword;
		sda.save = 0;
		sda.flag = 0;
		sda.title = "GG - zmiana has³a [1/4]";
		sda.info = "Podaj aktualne has³o.";
		if (!ICMessage(IMI_DLGPASS, (int)&sda, 0))
			return 0;
		oldPassword = sda.pass;

		sda.pass = "";
		sda.save = 1;
		sda.flag = DFLAG_SAVE;
		sda.title = "GG - zmiana has³a [2/4]";
		sda.info = "Podaj nowe has³o do twojego konta.";
		if (!ICMessage(IMI_DLGSETPASS, (int)&sda, 1)) 
			return 0;
		string password = sda.pass;
		bool save = sda.save;

		sDIALOG_enter sde;
		sde.title = "GG - zmiana has³a [3/4]";
		sde.info = "Podaj swój adres email, na który bêdzie mo¿na kiedyœ przes³aæ has³o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde)) return 0;
		string email = sde.value;

		string tokenID;
		string tokenVal;
		if (!GG::getToken("GG - zmiana has³a [krok 4/4]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.", tokenID, tokenVal))
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - zmiana has³a";
		sdl.info = "Komunikujê siê z serwerem…";
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
			ICMessage(IMI_ERROR, (int)stringf("Wyst¹pi³ b³¹d. SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy numer konta i has³o. Nastêpnie spróbuj ponownie.").c_str(),MB_TASKMODAL|MB_OK);
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
			(int)stringf("Has³o dla konta %d zosta³o zmienione!", login).c_str()
			:
			(int)stringf("Wyst¹pi³ b³¹d podczas zmiany has³a dla konta %d . SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy numer konta i has³o i spróbuj ponownie.", login).c_str()
		);
		gg_free_change_passwd(gghttp);
		return 0;
	}

	unsigned __stdcall remindPassword(LPVOID lParam) {
		sDIALOG_enter sde;
		sde.title = "GG - przypomnienie has³a [1/2]";
		sde.info = "Podaj adres email, który wpisa³eœ podczas zak³adania konta. Na ten email otrzymasz has³o.";
		if (!ICMessage(IMI_DLGENTER, (int)&sde))
			return 0;
		string email = sde.value;

		string tokenID;
		string tokenVal;
		if (!GG::getToken("GG - przypomnienie has³a [2/2]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.", tokenID, tokenVal))
			return 0;

		sDIALOG_long sdl;
		sdl.title = "GG - przypomnienie has³a";
		sdl.info = "Komunikujê siê z serwerem…";
		sdl.flag = DLONG_AINET | DLONG_CANCEL;
		sdl.cancelProc = disconnectDialogCB;
		sdl.timeoutProc = timeoutDialogCB;
		sdl.timeout = GETINT(CFG_TIMEOUT);
		ICMessage(IMI_LONGSTART, (int)&sdl);

		Singleton<Controller>::getInstance()->setProxy();
		gg_http* gghttp = gg_remind_passwd3(GETINT(CFG::login), email.c_str(), tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp) {
			ICMessage(IMI_LONGEND, (int)&sdl);
			ICMessage(IMI_ERROR, (int)"Wyst¹pi³ b³¹d! SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy numer konta/adres email i spróbuj ponownie.", MB_TASKMODAL | MB_OK);
			return 0;
		}
		ICMessage(IMI_LONGEND, (int)&sdl);

		gg_pubdir* pd = (gg_pubdir*)gghttp->data;

		ICMessage(IMI_INFORM,
			pd->success ?
			(int)"Has³o zosta³o wys³ane!"
			:
			(int)"Wyst¹pi³ b³¹d! SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy numer konta i spróbuj ponownie."
		);
		gg_remind_passwd_free(gghttp);
		return 0;
	}

	void importList() {
		sDIALOG_choose sd;
		sd.title = "Import listy kontaktów";
		sd.info = "Wybierz sk¹d importowaæ.\nZostan¹ dodane tylko brakuj¹ce kontakty.";
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
						//todo: Nie wiem czy to dzia³a.
						file >> fileContents;
						if (file.good()) {
							importListFromString((String)fileContents);
						} else {
							ICMessage(IMI_ERROR, (int)"Nie mog³em wczytaæ pliku!", MB_TASKMODAL | MB_OK);
						}
						file.close();
					} else {
						ICMessage(IMI_ERROR, (int)"Nie mog³em wczytaæ pliku!", MB_TASKMODAL | MB_OK);
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
		sd.title = "Export listy kontaktów";
		sd.info = "Wybierz dok¹d exportowaæ.";
		sd.flag = DFLAG_CANCEL;
		sd.items = "Do pliku\nNa Serwer GG\nUsuñ listê z serwera";
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
							ICMessage(IMI_INFORM, (int)"Kontakty zosta³y zapisane.");
						} else {
							ICMessage(IMI_ERROR, (int)"Nie mog³em zapisaæ do pliku!", MB_TASKMODAL | MB_OK);
						}
						file.close();
					} else {
						ICMessage(IMI_ERROR, (int)"Nie mog³em zapisaæ do pliku!", MB_TASKMODAL | MB_OK);
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