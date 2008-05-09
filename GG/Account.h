#pragma once

namespace GG {
	class Account {
		public:
			enum enAccountOperation {
				opIdle,
				opGetToken,
				opCreateAccount,
				opRemoveAccount,
				opChangePassword,
				opRemindPassword
			};

		public:
			Account(tUid uid = 0, string password = "");

		public:
			inline tUid getUid() {
				return _uid;
			}
			inline string getLogin() {
				return inttostr(_uid);
			}
			inline string getPassword() {
				return _password;
			}
			inline bool isIdle() {
				return _operation == opIdle;
			}
			
		protected:
			string contactsToString(tContacts& cnts);
			tContacts stringToContacts(string list);

		public:
			string getToken(string path);
			void createAccount(string newPassword, string email, string tokenVal);
			void removeAccount(string tokenVal);
			void changePassword(string newPassword, string email, string tokenVal);
			void remindPassword(string email, string tokenVal);

			void exportContactsToServer(Session& session, tContacts& cnts);
			void removeContactsFromServer(Session& session);
			void importContactsFromServer(Session& session);
			void exportContactsToFile(Session& session, tContacts& cnts, string file);
			tContacts importContactsFromFile(Session& session, string file);

			void cancelOperation();

		protected:
			tUid _uid;
			string _password;
			string _tokenID;
			enAccountOperation _operation;
	};
}