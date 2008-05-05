#pragma once

namespace GG {
	class Session;
	class Contacts;
	class Account;
	class Directory;

	typedef unsigned long tUid;
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
		
		bool operator ==(bool default) {
			return this->default == default;
		}
	};
	typedef vector<Server> tServers;

	class Session {
		public:
			typedef void (*ggHandler)(gg_event*);
			
			enum SessionState {
				idle,
				connecting,
				connected,
				listening
			};

		public:
			Session(string login, string password, ggHandler handler, bool friendsOnly);

		public:
			inline bool isConnecting() {
				return _connecting;
			}
			inline bool isConnected() {
				return _connected;
			}
			inline bool isListening() {
				return _listening;
			}
			inline string getLogin() {
				return inttostr(_uid);
			}
			inline tUid getUid() {
				return _uid;
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
			inline void setServers(tServers servers) {
				_servers = servers;
			}

		public:
			void setProxy(bool enabled, bool httpOnly, string host = "", int port = 0, string login = "", string password = "");
			bool connect(tStatus status = ST_ONLINE, string statusDescription = "");
			void stopConnecting();
			void startListening();
			void setStatus(tStatus status, const char* statusDescription = "");
			void ping();
			void disconnect(const char* statusDescription = "");

			void sendContacts(tContacts& cnts);
			void addContact(Contact& cnt);
			void addContact(string uid, char type = GG_USER_NORMAL);
			void changeContact(Contact& cnt);
			void removeContact(Contact& cnt);
			void removeContact(string uid, char type = GG_USER_NORMAL);

			void setFriendsOnly(bool friendsOnly = true);

		protected:
			ggHandler _handler;
			tUid _uid;
			string _password;
			bool _friendsOnly;
			tServers _servers;

			gg_session* _session;

			bool _connecting;
			bool _connected;
			bool _listening;

			tStatus _status;
			string _statusDescription;

			bool _stopConnecting;
	};

	class Account {
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

		public:
			string getToken(string path);
			bool createAccount(string newPassword, string email, string tokenVal);
			bool removeAccount(string tokenVal);
			bool changePassword(string newPassword, string email, string tokenVal);
			bool remindPassword(string email, string tokenVal);

		protected:
			tUid _uid;
			string _password;
			string _tokenID;
	};

	class Directory {
		//na razie nie ma
	};
}

int convertKStatus(tStatus status, string description = "", bool friendsOnly = false);
tStatus convertGGStatus(int status);
inline int getCntType(tStatus status) {
	return (status & ST_IGNORED) ? GG_USER_BLOCKED : (status & ST_HIDEMYSTATUS) ? GG_USER_OFFLINE : GG_USER_NORMAL;
}