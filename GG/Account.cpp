#include "stdafx.h"
#include "Session.h"
#include "Account.h"

namespace GG {
	Account::Account(GG::tUid uid, string password) : 
		_uid(uid), _password(password), _operation(opIdle) {
	}

	string Account::contactsToString(tContacts& cnts) {
		string result = "";
		const string separator(";");

		for (tContacts::iterator i = cnts.begin(); i != cnts.end(); ++i) {
			result += i->getName() + separator;
			result += i->getSurname() + separator;
			result += i->getNick() + separator;
			result += i->getDisplay() + separator;
			result += i->getString(Contact::colCellPhone) + separator;
			result += i->getString(Contact::colGroup) + separator;
			result += (i->getNet() != Net::none ? i->getUidString().c_str() : "") + separator;
			result += i->getEmail() + separator;
			//todo: Ustawienia dŸwiêków.
			/*string getSoundSetting(tCntId cnt, const std::string& type) {
				string result;
				//todo: zgodnie z nowym API to bêdzie wygl¹da³o nieco inaczej… nie wiem tylko jak ;)
				string sound = Ctrl->DTgetStr(Tables::tableContacts, cnt, ("SOUND_" + type).c_str());
				if (sound == "") {
					return "0;";
				} else if (sound[0] == '!') {
					return "1;";
				} else {
					return "2;" + sound;
				}
			}
			result += getSoundSetting(i->getID(), "newUser") + separator;
			result += getSoundSetting(i->getID(), "newMsg") + separator;*/
			result += (i->getStatus() & ST_HIDEMYSTATUS) != 0 ? "1" : "0" + separator;
			result += i->getString(Contact::colPhone);
			result += "\r\n";
		}
		return result;
	}

	string Account::getToken(string path) {
		if (!isIdle())
			throw ExceptionString("Poprzednia operacja nie zosta³a zakoñczona!");

		_operation = opGetToken;

		gg_http* gghttp = gg_token(false);
		if (!gghttp || gghttp->state != GG_STATE_DONE)
			throw ExceptionString("B³¹d przy pobieraniu.");
		struct gg_token* token = (struct gg_token*)gghttp->data;
		_tokenID = token->tokenid;

		ofstream file(path.c_str(), ios_base::out | ios_base::trunc | ios_base::binary);
		file.write(gghttp->body, gghttp->body_size);
		if (file.fail()) {
			file.close();
			_operation = opIdle;
			throw ExceptionString("B³¹d przy zapisie pliku.");
		}
		file.close();

		gg_token_free(gghttp);
		_operation = opIdle;
		return path;
	}

	void Account::createAccount(string newPassword, string email, string tokenVal) {
		if (!isIdle())
			throw ExceptionString("Poprzednia operacja nie zosta³a zakoñczona!");

		_operation = opCreateAccount;

		gg_http* gghttp = gg_register3(email.c_str(), newPassword.c_str(), _tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp || gghttp->state != GG_STATE_DONE) {
			_operation = opIdle;
			throw ExceptionString("Rejestracja nie powiod³a siê!");
		}

		gg_register_watch_fd(gghttp);
		gg_pubdir* pd = (gg_pubdir*)gghttp->data;
		if (!pd->success) {
			gg_register_free(gghttp);
			_operation = opIdle;
			throw ExceptionString("Rejestracja nie powiod³a siê!");
		}

		_uid = pd->uin;
		_password = newPassword;

		gg_register_free(gghttp);
		_operation = opIdle;
	}

	void Account::removeAccount(string tokenVal) {
		if (!isIdle())
			throw ExceptionString("Poprzednia operacja nie zosta³a zakoñczona!");

		_operation = opRemoveAccount;

		gg_http* gghttp = gg_unregister3(_uid, _password.c_str(), _tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp || gghttp->state != GG_STATE_DONE) {
			_operation = opIdle;
			throw ExceptionString("Usuniêcie konta nie powiod³o siê!");
		}

		gg_pubdir* pd = (gg_pubdir*)gghttp->data;
		if (!pd->success) {
			gg_unregister_free(gghttp);
			_operation = opIdle;
			throw ExceptionString("Usuniêcie konta nie powiod³o siê!");
		}

		gg_unregister_free(gghttp);
		_operation = opIdle;
	}

	void Account::changePassword(string newPassword, string email, string tokenVal) {
		if (!isIdle())
			throw ExceptionString("Poprzednia operacja nie zosta³a zakoñczona!");

		_operation = opChangePassword;
		
		gg_http* gghttp = gg_change_passwd4(_uid, email.c_str(), _password.c_str(), newPassword.c_str(), _tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp || gghttp->state != GG_STATE_DONE) {
			_operation = opIdle;
			throw ExceptionString("Zmiana has³a nie powiod³a siê!");
		}

		gg_pubdir* pd = (gg_pubdir*)gghttp->data;
		if (!pd->success) {
			gg_free_change_passwd(gghttp);
			_operation = opIdle;
			throw ExceptionString("Zmiana has³a nie powiod³a siê!");
		}

		_password = newPassword;

		gg_free_change_passwd(gghttp);
		_operation = opIdle;
	}

	void Account::remindPassword(string email, string tokenVal) {
		if (!isIdle())
			throw ExceptionString("Poprzednia operacja nie zosta³a zakoñczona!");

		_operation = opRemindPassword;
		
		gg_http* gghttp = gg_remind_passwd3(_uid, email.c_str(), _tokenID.c_str(), tokenVal.c_str(), 0);
		if (!gghttp || gghttp->state != GG_STATE_DONE) {
			_operation = opIdle;
			throw ExceptionString("Przypomnienie has³a nie powiod³o siê!");
		}

		gg_pubdir* pd = (gg_pubdir*)gghttp->data;
		if (!pd->success) {
			gg_remind_passwd_free(gghttp);
			_operation = opIdle;
			throw ExceptionString("Przypomnienie has³a nie powiod³o siê!");
		}

		gg_remind_passwd_free(gghttp);
		_operation = opIdle;
	}

	void Account::exportContactsToServer(Session& session, tContacts& cnts) {
		if (!session.isListening()) {
			throw ExceptionString("Nie jesteœ po³¹czony z serwerem!");
		} else if (!session.isIdle() || !isIdle()) {
			throw ExceptionString("Poprzednia operacja nie zosta³a zakoñczona!");
		}

		session._operation = Session::opExportContacts;

		if (gg_userlist_request(session._session, GG_USERLIST_PUT, contactsToString(cnts).c_str())) {
			session._operation = Session::opIdle;
			throw ExceptionString("Wysy³anie nie powiod³o siê.");
		}
	}

	void Account::removeContactsFromServer(Session& session) {
		exportContactsToServer(session, tContacts());
	}

	void Account::importContactsFromServer(Session& session) {
		if (!session.isListening()) {
			throw ExceptionString("Musisz byæ po³¹czony z serwerem GG!");
		} else if (!session.isIdle() || !isIdle()) {
			throw ExceptionString("Poprzednia operacja nie zosta³a zakoñczona!");
		}

		session._operation = Session::opImportContacts;

		if (gg_userlist_request(session._session, GG_USERLIST_GET, 0)) {
			session._operation = Session::opIdle;
			throw ExceptionString("Wysy³anie nie powiod³o siê.");
		}
	}
	
	void Account::cancelOperation() {
		_operation = opIdle;
	}
};