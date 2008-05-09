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
		friend class Account;
		friend class Directory;

		public:
			typedef void (*ggHandler)(gg_event*);
			
			enum enSessionState {
				stDisconnected,
				stConnecting,
				stConnected,
				stListening
			};
			
			enum enSessionOperation {
				opIdle,
				opImportContacts,
				opExportContacts,
				opGetContact,
				opSetContact,
				opSearchContact
			};

		public:
			Session(string login, string password, ggHandler handler, bool friendsOnly);

		public:
			inline bool isConnecting() {
				return _state == stConnecting;
			}
			inline bool isConnected() {
				return _state == stConnected || _state == stListening;
			}
			inline bool isListening() {
				return _state == stListening;
			}
			inline bool isIdle() {
				return _operation == opIdle;
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
			void cancelOperation();

		protected:
			ggHandler _handler;
			tUid _uid;
			string _password;
			bool _friendsOnly;
			tServers _servers;

			gg_session* _session;
			enSessionState _state;
			enSessionOperation _operation;
			tStatus _status;
			string _statusDescription;

			bool _stopConnecting;
	};
}