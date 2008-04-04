#pragma once

#define TIMER_INTERVAL 15 * 1000
#define PING_INTERVAL 120 * 1000
#define MSG_TIMEOUT 20

namespace GG {
	//TODO: Net w jakiœ sposób bêdzie dynamiczny.
	const unsigned net = 10; // wartoœæ NET
	const int sig = (int)"GG"; //sygnatura
	const int name = (int)"Gadu-Gadu"; //nazwa
	
	const char defaultServers[] = "s1.gadu-gadu.pl\r\n\r\n217.17.41.83\r\n217.17.41.84\r\n217.17.41.85\r\n217.17.41.86\r\n217.17.41.87\r\n217.17.41.88\r\n217.17.41.92";

	namespace CFG {
		const unsigned group = net * 1000;
		const unsigned login = group + 1;
		const unsigned password = group + 2;
		const unsigned status = group + 3;
		const unsigned startStatus = group + 4;
		const unsigned description = group + 5;
		const unsigned friendsOnly = group + 6;
		const unsigned servers = group + 7;
		const unsigned choosenServer = group + 8;
		const unsigned useSSL = group + 9;
		const unsigned resumeDisconnected = group + 10;
		
		/* TODO: Stare id kolumn; zapewne przydadz¹ siê przy pisaniu f-cji updatuj¹cej plugin.
		const unsigned login = 1053;
		const unsigned password = 1054;
		const unsigned status = 1055;
		const unsigned startStatus = 1056;
		const unsigned description = 1057;
		const unsigned friendsOnly = 1058;
		const unsigned servers = 1059;
		const unsigned choosenServer = 1060;
		const unsigned trayMenu = 1061;
		const unsigned useSSL = 1063;
		const unsigned resumeDisconnected = 1064;*/
	};
	
	namespace ACT {
		const unsigned setDefaultServers = net * 1000 + 200;
		const unsigned createGGAccount = net * 1000 + 201;
		const unsigned removeGGAccount = net * 1000 + 202;
		const unsigned changePassword = net * 1000 + 203;
		const unsigned remindPassword = net * 1000 + 204;
		const unsigned importCntList = net * 1000 + 205;
	};
	
	namespace ICO {
		const unsigned logo = net * 1000 + 300;
		const unsigned server = net * 1000 + 301;
		const unsigned overlay = net * 1000 + 302;
		const unsigned online = net * 1000 + 303;
		const unsigned away = net * 1000 + 304;
		const unsigned invisible = net * 1000 + 305;
		const unsigned offline = net * 1000 + 306;
		const unsigned blocking = net * 1000 + 307;
		const unsigned connecting = net * 1000 + 308;
	};

	enum userListRequest {
		ulrNone, ulrPut, ulrGet, ulrClear, ulrDone
	};
	typedef map <int, int> tEventHandler;

	// funkcje ---------------------------------------

	// connection
	VOID CALLBACK timerProc(HWND hwnd,UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	gg_session * loginWithTimeout(gg_login_params * p);
	unsigned int __stdcall threadProc (void * lpParameter);
	int connect();
	int disconnect();

	// conn_tools
	void setStatus(int status, int setdesc = 1, gg_login_params * lp=0);
	void setStatusDesc();
	void chooseServer();
	bool __stdcall disconnectDialogCB(sDIALOG_long*sd);
	bool __stdcall cancelDialogCB(sDIALOG_long*sd);
	bool __stdcall timeoutDialogCB(int type, sDIALOG_long*sd);
	bool __stdcall timeoutDialogSimpleCB(int type, sDIALOG_long*sd);

	// pubdir
	void onPubdirSearchReply(gg_event *e);
	unsigned int __stdcall doCntSearch(LPVOID lParam);
	CStdString nfoGet(bool noTable, int cnt, int id);
	unsigned int __stdcall doCntDownload(int pMsg);
	unsigned int __stdcall doCntUpload(sIMessage_2params * msg);

	// api_sessions
	int event(GGER_enum type, void * data);
	void waitOnSessions();
	
	// userlist
	void dlgListImport();
	void dlgListExport();
	unsigned int __stdcall doListExport(LPVOID lParam);
	unsigned int __stdcall doListImport(LPVOID lParam);
	unsigned int __stdcall dlgListRefresh(LPVOID lParam);
	void onUserlistReply(gg_event * e);
	string getUserList ();
	int setUserList(char * list);

	// tools
	int check(bool conn = 1, bool sess = 1, bool login = 1, bool warn = 1);
	void getAccount(int & login, CStdString & pass);
	int event(GGER_enum type, void * data);
	void waitOnSessions();
	int userType(int id);
	CStdString msgToHtml(CStdString msg, void * formats, int formats_length);
	CStdString htmlToMsg(CStdString msgIn, void * formats, int & length);
	bool getToken(const string & title, const string & info, string & tokenid, string & tokenval);
	void quickEvent(int Uid, const char * body, const char * ext="", int flag=0);	// zmienne globalne

	extern bool onRequest;
	extern HANDLE ggThread;
	extern int ggThreadId;
	extern UINT_PTR timer;
	extern struct gg_session *sess;
	extern int loop;
	extern bool inAutoAway;
	extern CStdString lastServer;
	extern CStdString currentServer;
	extern userListRequest onUserListRequest;
	extern CStdString userListBuffer;

	extern CStdString curStatusInfo;
	extern int curStatus;

	// Eventy, 
	extern tEventHandler eventHandler;

	extern int sessionUsage;

	// Sprawdzanie dostarczenia wiadomosci
	struct cMsgSent {
		int msgID;
		time_t sentTime;
		CStdString digest;
		int Uid;
	};
	extern CRITICAL_SECTION msgSent_CS;
	typedef map <int, cMsgSent> tMsgSent;
	extern tMsgSent msgSent;

	// Wyszukiwanie
	struct cGGSearch {
		int cnt; // nr kontaktu lub -1 jesli search
		bool store; // zapisywane do wlasciwosci kontaktu
		bool finished;
	};
	extern CRITICAL_SECTION searchMap_CS;
	extern map <int, cGGSearch*> searchMap;
};

using namespace GG;