#pragma once

namespace GG {
	class Session;
	class Account;
	class Directory;
	
	typedef unsigned long tUid;

	class Session {
		public:
			typedef void (*ggHandler)(gg_event*);
			typedef list<Contact> tContacts;
			
			struct Server {
				string ip;
				bool default;
				bool ssl;

				Server (string ip = "", bool ssl = false, bool default = false) {
					this->ip = ip;
					this->ssl = ssl;
					this->default = default;
				}
			};
			typedef list<Server> Servers;
			
		public:
			Session(string login, string password, ggHandler handler, bool friendsOnly);

		public:
			inline bool isConnecting() {
				return _connecting;
			}
			inline bool isConnected() {
				return _connected;
			}
			inline bool isWatching() {
				return _watching;
			}
			inline string getLogin() {
				return inttostr(_login);
			}
			inline string getPassword() {
				return _password;
			}
			inline tStatus getStatus() {
				return _status;
			}
			inline string getStatusDescription() {
				return _statusDescription;
			}

		public:
			void setProxy(bool enabled, bool httpOnly, string host = "", int port = 0, string login = "", string password = "");
			bool connect(tStatus status = ST_ONLINE, string statusDescription = "");
			void stopConnecting(bool quick = false, unsigned timeout = 1000, bool terminate = false);
			void startWatching();
			void setStatus(tStatus status, const char* statusDescription = "");
			void disconnect(const char* statusDescription = "");

			void setFriendsOnly(bool friendsOnly = true);

			void sendCnts(tContacts& cnts);
			void addCnt(Contact& cnt);
			void addCnt(string uid, char type = GG_USER_NORMAL);
			void changeCnt(Contact& cnt);
			void removeCnt(Contact& cnt);
			void removeCnt(string uid, char type = GG_USER_NORMAL);

		public:
			static int convertKStatus(tStatus status, string description = "", bool friendsOnly = false);
			static tStatus convertGGStatus(int status);
			static inline int getCntType(tStatus status) {
				return (status & ST_IGNORED) ? GG_USER_BLOCKED : (status & ST_HIDEMYSTATUS) ? GG_USER_OFFLINE : GG_USER_NORMAL;
			}

		protected:
			ggHandler _handler;
			tUid _login;
			string _password;
			bool _friendsOnly;

			gg_session* _session;
			HANDLE _thread;

			bool _connecting;
			bool _connected;
			bool _watching;
			
			tStatus _status;
			string _statusDescription;

			bool _stopConnecting;
	};

	class Account {
		public:
			Account(tUid login = 0, string password = "");

		public:
			string getToken(string file); //zwraca file
			bool createAccount();
			bool changePassword(string newPassword, string token);
			bool remindPassword(string email, string token);
			bool removeAccount();

		protected:
			tUid _login;
			string _password;
			string _tokenID;
	};
	
	class Directory {
		//na razie nie ma
	};
}