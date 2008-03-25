#include "stdafx.h"
#include "GG.h"
using namespace Konnekt::GG;
using Stamina::inttostr;
using Stamina::stringf;

#define MAX_STRING 10000

unsigned int __stdcall GG::dlgAccount (LPVOID lParam) {
	if (IMessage(IMI_CONFIRM, 0, 0, (int)"Zostanie za³o¿one nowe konto w sieci Gadu-Gadu™.\nKontynuowaæ?", MB_TASKMODAL|MB_YESNO) == IDNO) 
		return 0;

	sDIALOG_access sda;
	sda.save = 1;
	sda.title = "GG - nowe konto [krok 1/3]";
	sda.info = "Podaj has³o do nowego konta.";
	if (!IMessage(IMI_DLGSETPASS, 0, 0, (int)&sda, 1) || !*sda.pass)
		return 0;
	string pass = sda.pass;
	bool save = sda.save;

	sDIALOG_enter sde;
	sde.title = "GG - nowe konto [krok 2/3]";
	sde.info = "Podaj swój adres email, na który bêdzie mo¿na kiedyœ przes³aæ has³o.";
	if (!IMessage(IMI_DLGENTER, 0, 0, (int)&sde) || !*sde.value)
		return 0;
	string email = sde.value;

	string tokenid;
	string tokenval;
	if (!GG::getToken("GG  - nowe konto [krok 3/3]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.", tokenid, tokenval))
		return 0;

	sDIALOG_long sdl;
	sdl.title = "GG - zak³adanie konta";
	sdl.info = "Komunikujê siê z serwerem…";
	sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
	sdl.cancelProc = disconnectDialogCB;
	sdl.timeoutProc = timeoutDialogCB;
	sdl.timeout = HTTP_TIMEOUT;
	ICMessage(IMI_LONGSTART, (int)&sdl);

	GG::setProxy();
	gg_http * gghttp;
	if (!(gghttp = gg_register3(email.c_str(), pass.c_str() ,tokenid.c_str(),  tokenval.c_str(), 0)) || gghttp->state != GG_STATE_DONE) {
		ICMessage(IMI_LONGEND, (int)&sdl);
		IMessage(IMI_ERROR, 0, 0, (int)stringf("Wyst¹pi³ b³¹d podczas zak³adania konta. SprawdŸ po³¹czenie i spróbuj ponownie.").c_str(),0);
		return 0;
	}
	ICMessage(IMI_LONGEND, (int)&sdl);

	gg_pubdir * pd = (gg_pubdir*)gghttp->data;
	if (pd->success) {
		UIActionCfgSetValue(sUIAction(IMIG_GGCFG_USER, IMIB_CFG | CFG_GG_PASS), save?pass.c_str():"");
		UIActionCfgSetValue(sUIAction(IMIG_GGCFG_USER,  IMIB_CFG | CFG_GG_LOGIN), inttostr(pd->uin).c_str());
		SETSTR(CFG_GG_PASS, save?pass.c_str():"");
		SETINT(CFG_GG_LOGIN, pd->uin);
		ICMessage(IMC_SAVE_CFG);
		IMessageDirect(IM_CNT_UPLOAD);
	}

	IMessage(IMI_INFORM, 0, 0,
		pd->success?
		(int)stringf("Konto zosta³o za³o¿one!\nTwój numerek to %d", pd->uin).c_str()
		:
		(int)stringf("Wyst¹pi³ b³¹d podczas zak³adania konta. SprawdŸ po³¹czenie i spróbuj ponownie.").c_str()
	);
	gg_register_free(gghttp);
	return 0;
}

unsigned int __stdcall GG::dlgRemoveAccount (LPVOID lParam) {
	if (IMessage(IMI_CONFIRM, 0, 0, (int)"Twoje konto na gadu-gadu zostanie usuniête!\nKontynuowaæ?", MB_TASKMODAL|MB_YESNO) == IDNO) 
		return 0;

	sDIALOG_enter sde;
	sde.title = "GG - usuwanie konta [krok 1/3]";
	sde.info = "PotwierdŸ numer konta";
	if (!IMessage(IMI_DLGENTER, 0, 0, (int)&sde, 1))
		return 0;
	string uin = sde.value;

	sDIALOG_access sda;
	sda.title = "GG - usuwanie konta [krok 2/3]";
	sda.info = "Podaj has³o do konta";
	if (!IMessage(IMI_DLGPASS, 0, 0, (int)&sda))
		return 0;
	string pass = sda.pass;

	string tokenid;
	string tokenval;
	if (!GG::getToken("GG  - usuwanie konta [krok 3/3]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.", tokenid, tokenval))
		return 0;

	sDIALOG_long sdl;
	sdl.title = "GG - usuwanie konta";
	sdl.info = "Komunikujê siê z serwerem…";
	sdl.flag = DLONG_MODAL | DLONG_AINET | DLONG_CANCEL;
	sdl.cancelProc = disconnectDialogCB;
	sdl.timeoutProc = timeoutDialogCB;
	sdl.timeout = HTTP_TIMEOUT;
	ICMessage(IMI_LONGSTART, (int)&sdl);
	GG::setProxy();
	gg_http* gghttp;
	if (!(gghttp = gg_unregister3(atoi(uin.c_str()), pass.c_str() ,tokenid.c_str(), tokenval.c_str(), 0))) {
		ICMessage(IMI_LONGEND, (int)&sdl);
		IMessage(IMI_ERROR, 0, 0, (int)stringf("Wyst¹pi³ b³¹d podczas usuwania konta. SprawdŸ po³¹czenie i spróbuj ponownie.").c_str(), 0);
		return 0;
	}

	ICMessage(IMI_LONGEND, (int)&sdl);
	gg_pubdir * pd = (gg_pubdir*)gghttp->data;
	if (pd->success) {
		UIActionCfgSetValue(sUIAction(IMIG_GGCFG_USER, IMIB_CFG | CFG_GG_PASS), "");
		UIActionCfgSetValue(sUIAction(IMIG_GGCFG_USER, IMIB_CFG | CFG_GG_LOGIN), "");
	}

	IMessage(IMI_INFORM, 0, 0 ,
		pd->success?
		(int)"Konto zosta³o usuniête…"
		:
		(int)"Wyst¹pi³ b³¹d podczas usuwania konta. SprawdŸ po³¹czenie i spróbuj ponownie."
	);
	gg_unregister_free(gghttp);
	return 0;
}

unsigned int __stdcall GG::dlgNewPass(LPVOID lParam) {
	if (!GG::check(1, 0, 1, 1) || IMessage(IMI_CONFIRM, 0, 0, (int)"Czy na pewno chcesz zmieniæ has³o?", MB_TASKMODAL|MB_YESNO) == IDNO)
		return 0;

	sDIALOG_access sda;
	CStdString oldPass;
	sda.save = 0;
	sda.flag = 0;
	sda.title = "GG - zmiana has³a [1/4]";
	sda.info = "Podaj aktualne has³o.";
	if (!IMessage(IMI_DLGPASS, 0, 0, (int)&sda, 0))  
		return 0;
	oldPass = sda.pass;

	sda.pass = "";
	sda.save = 1;
	sda.flag = DFLAG_SAVE;
	sda.title = "GG - zmiana has³a [2/4]";
	sda.info = "Podaj nowe has³o do twojego konta.";
	if (!IMessage(IMI_DLGSETPASS, 0, 0, (int)&sda, 1)) 
		return 0;
	string pass = sda.pass;
	bool save = sda.save;

	sDIALOG_enter sde;
	sde.title = "GG - zmiana has³a [3/4]";
	sde.info = "Podaj swój adres email, na który bêdzie mo¿na kiedyœ przes³aæ has³o.";
	if (!IMessage(IMI_DLGENTER, 0, 0, (int)&sde))  return 0;
	string email = sde.value;

	string tokenid;
	string tokenval;
	if (!GG::getToken("GG - zmiana has³a [krok 4/4]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.", tokenid, tokenval))
		return 0;

	sDIALOG_long sdl;
	sdl.title = "GG - zmiana has³a";
	sdl.info = "Komunikujê siê z serwerem…";
	sdl.flag = DLONG_MODAL|DLONG_AINET|DLONG_CANCEL;
	sdl.cancelProc = disconnectDialogCB;
	sdl.timeoutProc = timeoutDialogCB;
	sdl.timeout = HTTP_TIMEOUT;
	ICMessage(IMI_LONGSTART, (int)&sdl);
	GG::setProxy();
	gg_http * gghttp;
	int ggLogin;
	CStdString ggPass;
	getAccount(ggLogin, ggPass);
	if (!(gghttp = gg_change_passwd4(ggLogin, email.c_str(), oldPass.c_str(), pass.c_str(), tokenid.c_str(), tokenval.c_str(), 0))) {
		ICMessage(IMI_LONGEND, (int)&sdl);
		IMessage(IMI_ERROR, 0, 0, (int)stringf("Wyst¹pi³ b³¹d. SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy numer konta i has³o. Nastêpnie spróbuj ponownie.").c_str(),MB_TASKMODAL|MB_OK);
		return 0;
	}

	ICMessage(IMI_LONGEND, (int)&sdl);
	gg_pubdir* pd = (gg_pubdir*)gghttp->data;
	if (pd->success) {
		UIActionCfgSetValue(sUIAction(IMIG_GGCFG_USER, IMIB_CFG | CFG_GG_PASS), save?pass.c_str():"");
		SETSTR(CFG_GG_PASS, save?pass.c_str():"");
	}

	IMessage(IMI_INFORM, 0, 0 ,
		pd->success?
		(int)stringf("Has³o dla konta %d zosta³o zmienione!", ggLogin).c_str()
		:
		(int)stringf("Wyst¹pi³ b³¹d podczas zmiany has³a dla konta %d . SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy numer konta i has³o i spróbuj ponownie", ggLogin).c_str()
	);
	gg_free_change_passwd(gghttp);
	return 0;
}

unsigned int __stdcall GG::dlgRemindPass(LPVOID lParam) {
	if (!GG::check(1, 0, 1, 1))  return 0;

	sDIALOG_enter sde;
	sde.title = "GG - przypomnienie has³a [1/2]";
	sde.info = "Podaj adres email, który wpisa³eœ podczas zak³adania konta. Na ten email otrzymasz has³o.";
	if (!IMessage(IMI_DLGENTER, 0, 0, (int)&sde))  return 0;
	string email = sde.value;

	string tokenid;
	string tokenval;
	if (!GG::getToken("GG - przypomnienie has³a [2/2]", "Wpisz tekst znajduj¹cy siê na poni¿szym obrazku.", tokenid, tokenval))
		return 0;

	sDIALOG_long sdl;
	sdl.title = "GG - przypomnienie has³a";
	sdl.info = "Komunikujê siê z serwerem…";
	sdl.flag = DLONG_AINET | DLONG_CANCEL;
	sdl.cancelProc = disconnectDialogCB;
	sdl.timeoutProc = timeoutDialogCB;
	sdl.timeout = HTTP_TIMEOUT;
	ICMessage(IMI_LONGSTART, (int)&sdl);
	GG::setProxy();
	gg_http * gghttp;
	if (!(gghttp = gg_remind_passwd3(GETINT(CFG_GG_LOGIN), email.c_str(), tokenid.c_str(), tokenval.c_str(), 0))) {
		ICMessage(IMI_LONGEND, (int)&sdl);
		IMessage(IMI_ERROR, 0, 0, (int)stringf("Wyst¹pi³ b³¹d! SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy numer konta/adres email i spróbuj ponownie").c_str(),MB_TASKMODAL|MB_OK);
		return 0;
	}

	ICMessage(IMI_LONGEND, (int)&sdl);
	gg_pubdir* pd = (gg_pubdir*)gghttp->data;

	IMessage(IMI_INFORM, 0, 0 ,
		pd->success?
		(int)"Has³o zosta³o wys³ane!"
		:
		(int)"Wyst¹pi³ b³¹d! SprawdŸ po³¹czenie, oraz czy poda³eœ prawid³owy numer konta i spróbuj ponownie"
	);
	gg_remind_passwd_free(gghttp);
	return 0;
}

void GG::dlgListImport() {
	sDIALOG_choose sd;
	sd.title = "Import listy kontaktów";
	sd.info  = "Wybierz sk¹d importowaæ.\nZostan¹ dodane tylko brakuj¹ce kontakty.";
	sd.flag  = DFLAG_CANCEL;
	sd.items = "Z pliku\nZ Serwera";
	int r = ICMessage(IMI_DLGBUTTONS, (int)&sd);
	IMLOG("- Import type %d", r);

	switch (r) {
		case 1: {
			OPENFILENAME of;
			memset(&of, 0, sizeof(of));
			of.lStructSize = sizeof(of) - 12;
			of.lpstrFilter = "Txt\0*.txt\0*.*\0*.*\0";
			char * buff = new char [MAX_STRING];
			strcpy(buff, "");
			of.lpstrFile = buff;
			of.nMaxFile = MAX_STRING;
			of.Flags = OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
			of.lpstrDefExt="txt";
			int f;

			if (GetOpenFileName(&of)) {
				if ((f = open(of.lpstrFile, O_RDONLY|O_TEXT))) {
					unsigned length = filelength(f);
					char * fbuff = new char [length + 1];
					length = read(f, fbuff, length);
					if (length != -1) {
						fbuff[length] = 0;
						setUserList(fbuff);
						ICMessage(IMI_REFRESH_LST);
					} else {
						IMDEBUG(DBG_ERROR, "Nie mogê czytaæ pliku!");
					}
					delete[] fbuff;
					close(f);
					ICMessage(IMI_INFORM, (int)"Kontakty zosta³y wczytane.");
				} else {
					ICMessage(IMI_ERROR, (int)"Nie mog³em wczytaæ pliku!", MB_TASKMODAL|MB_OK);
				}
			}
			delete[] buff;
			break;
		} case 2: {
			CloseHandle((HANDLE)Ctrl->BeginThread("ListImport", 0, 0, doListImport, 0, 0, 0));
			break;
		}
	}
	ICMessage(IMC_SAVE_CNT);
}

void GG::dlgListExport() {
	sDIALOG_choose sd;
	sd.title = "Export listy kontaktów";
	sd.info  = "Wybierz dok¹d exportowaæ.";
	sd.flag  = DFLAG_CANCEL;
	sd.items = "Do pliku\nNa Serwer GG\nUsuñ listê z serwera";
	int r = ICMessage(IMI_DLGBUTTONS, (int)&sd);
	IMLOG("- Export type %d", r);

	switch (r) {
		case 1: {
			OPENFILENAME of;
			memset(&of, 0, sizeof(of));
			of.lStructSize = sizeof(of) - 12;
			of.lpstrFilter = "Txt\0*.txt\0*.*\0*.*\0";
			char * buff = new char[MAX_STRING];
			strcpy(buff, "kontakty");
			of.lpstrFile = buff;
			of.nMaxFile = MAX_STRING;
			of.nFilterIndex = 1;
			of.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
			of.lpstrDefExt = "txt";

			FILE* f;
			if (GetSaveFileName(&of)) {
				if ((f = fopen(of.lpstrFile, "wb"))) {
					string str=getUserList();
					fwrite((char*)str.c_str(), str.size(), 1, f);
					fclose(f);
					ICMessage(IMI_INFORM, (int)"Kontakty zosta³y zapisane.");
				} else {
					ICMessage(IMI_ERROR, (int)"Nie mog³em zapisaæ do pliku!", MB_TASKMODAL|MB_OK);
				}
			}
			delete [] buff;
			break;
		} case 2: {
			CloseHandle((HANDLE)Ctrl->BeginThread("ListExport", 0, 0, doListExport, 0, 0, 0));
			break;
		} case 3: {
			CloseHandle((HANDLE)Ctrl->BeginThread("ListExport", 0, 0, doListExport, (void*)1, 0, 0));
			break;
		}
	}
}
