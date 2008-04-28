#pragma once

class GGSession {
	public:
		typedef void (*ggHandler)(gg_event*);
		typedef list<Contact> Contacts;
		
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

	protected:
		struct ConnectInfo {
			string login;
			string password;
			ggHandler handler;
			tStatus status;
			string statusDescription;
			bool friendsOnly;
			GGSession* gg;
			
			ConnectInfo(string login, string password, ggHandler handler, tStatus status, string statusDescription, bool friendsOnly, GGSession* gg) {
				this->login = login;
				this->password = password;
				this->handler = handler;
				this->status = status;
				this->statusDescription = statusDescription;
				this->friendsOnly = friendsOnly;
				this->gg = gg;
			}
		};
		
	public:
		GGSession(ThreadRunner& threads);
		//~GGSession();
		
	public:
		inline bool isConnected() {
			return _connected;
		}
		inline bool isConnecting() {
			return _connecting;
		}
		inline tStatus getStatus() {
			return _status;
		}
		inline String getStatusDescription() {
			return _statusDescription;
		}

	public:
		void setProxy(bool enabled, bool httpOnly, string host = "", int port = 0, string login = "", string password = "");
		void connect(string login, string password, ggHandler handler, tStatus status = ST_ONLINE, string statusDescription = "", bool friendsOnly = false);
		void stopConnecting(bool quick = false, unsigned timeout = 1000, bool terminate = false);
		void setStatus(tStatus status, const char* statusDescription);
		void disconnect(const char* statusDescription);
		
		void sendCnts(Contacts& cnts);
		void addCnt(Contact& cnt);
		void addCnt(string uin, char type = GG_USER_NORMAL);
		void changeCnt(Contact& cnt);
		void removeCnt(Contact& cnt);
		void removeCnt(string uin, char type = GG_USER_NORMAL);

	public:
		static int convertKStatus(tStatus status, string description = "", bool friendsOnly = false);
		static tStatus convertGGStatus(int status);
		static inline int getCntType(tStatus status) {
			return (status & ST_IGNORED) ? GG_USER_BLOCKED : (status & ST_HIDEMYSTATUS) ? GG_USER_OFFLINE : GG_USER_NORMAL;
		}
		
	protected:
		static unsigned _stdcall connectProc(LPVOID lParam);

	protected:
		ThreadRunner _threads;
		gg_session* _session;
		HANDLE _thread;
		bool _connected;
		bool _connecting;
		tStatus _status;
		String _statusDescription;
		
		bool _stopConnecting;
};