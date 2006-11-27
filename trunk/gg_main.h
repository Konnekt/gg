#pragma once


#define TIMER_INTERVAL 15 * 1000
#define PING_INTERVAL 120 * 1000
#define MSG_TIMEOUT    20
//#define MAX_STRING     2000
#define TIMEOUT        GETINT(CFG_TIMEOUT) //30000
#define HTTP_TIMEOUT TIMEOUT
#define PUBDIR_TIMEOUT TIMEOUT * 2

#define GG_DEF_SERVERS "SSL\r\n\r\n217.17.41.83\r\n217.17.41.84\r\n217.17.41.85\r\n217.17.41.86\r\n217.17.41.87\r\n217.17.41.88\r\n217.17.41.92"

namespace Konnekt { namespace GG {
	enum userListRequest {
		ulrNone , ulrPut , ulrGet , ulrClear , ulrDone
	};
	typedef map <int , int> tEventHandler;

	// funkcje ---------------------------------------


	// connection
	VOID CALLBACK timerProc(HWND hwnd,UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	gg_session * loginWithTimeout(gg_login_params * p);
	unsigned int __stdcall threadProc (void * lpParameter);
	int connect();
	int disconnect();
	// conn_tools
	void setProxy();
	void setStatus(int status , int setdesc = 1 , gg_login_params * lp=0);
	void setStatusDesc();
	void chooseServer();
	bool __stdcall disconnectDialogCB(sDIALOG_long*sd);
	bool __stdcall cancelDialogCB(sDIALOG_long*sd);
	bool __stdcall timeoutDialogCB(int type , sDIALOG_long*sd);
	bool __stdcall timeoutDialogSimpleCB(int type , sDIALOG_long*sd);

	// actions
	unsigned int __stdcall dlgAccount (LPVOID lParam);
	unsigned int __stdcall dlgRemoveAccount (LPVOID lParam);
	unsigned int __stdcall dlgNewPass(LPVOID lParam);
	unsigned int __stdcall dlgRemindPass(LPVOID lParam);
	// pubdir
	void onPubdirSearchReply(gg_event *e);
	unsigned int __stdcall doCntSearch(LPVOID lParam);
	CStdString nfoGet(bool noTable , int cnt , int id);
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
	int check(bool conn = 1 , bool sess = 1 , bool login = 1,  bool warn = 1);
	void getAccount(int & login , CStdString & pass);
	int event(GGER_enum type, void * data);
	void waitOnSessions();
	void gg_debug_lfunc(int level, const char *format, ...);
	int userType(int id);
	CStdString msgToHtml(CStdString msg , void * formats , int formats_length);
	CStdString htmlToMsg(CStdString msgIn , void * formats , int & length);
	bool getToken(const string & title , const string & info , string & tokenid , string & tokenval);
	void quickEvent(int Uid , const char * body , const char * ext="" , int flag=0);	// zmienne globalne

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
	typedef map <int , cMsgSent> tMsgSent;
	extern tMsgSent msgSent;

	// Wyszukiwanie
	struct cGGSearch {
		int cnt; // nr kontaktu lub -1 jesli search
		bool store; // zapisywane do wlasciwosci kontaktu
		bool finished;
	};
	extern CRITICAL_SECTION searchMap_CS;
	extern map <int , cGGSearch*> searchMap;




}; }; // namespace
