#pragma once

#include "GG.h"
#include "GG/Session.h"
#include "GG/Helpers.h"
#include "GG/Account.h"

namespace GG {
	class Controller : public iController {

	public:
		//typy
		enum checkCriterion {
			ccInternet = 2,
			ccSession = 4,
			ccAccount = 8,
			ccSessionIdle = 16,
			ccAccountIdle = 32,
		};

		typedef pair<tStatus, string> tStatusInfo;

	protected:
		Controller();

	protected:
		//IMessage
		void onPrepareUI(IMEvent& ev);
		void onStart(IMEvent& ev);
		void onEnd(IMEvent& ev);
		void onDisconnect(IMEvent& ev);
		void onGetStatus(IMEvent& ev);
		void onGetStatusInfo(IMEvent& ev);
		void onGetUID(IMEvent& ev);
		void onIsConnected(IMEvent& ev);
		void onCntAdd(IMEvent& ev);
		void onCntChanged(IMEvent& ev);
		void onCntRemove(IMEvent& ev);
		void onIgnChanged(IMEvent& ev);
		void onCntDownload(IMEvent& ev);
		void onCntUpload(IMEvent& ev);
		void onCntSearch(IMEvent& ev);
		void onChangeStatus(IMEvent& ev);

	public:
		//akcje
		void handleConfig(ActionEvent& ev);
		void handleSetDefaultServers(ActionEvent& ev);
		void handleCreateGGAccount(ActionEvent& ev);
		void handleRemoveGGAccount(ActionEvent& ev);
		void handleChangePassword(ActionEvent& ev);
		void handleRemindPassword(ActionEvent& ev);
		void handleImportContactsFromServer(ActionEvent& ev);
		void handleImportContactsFromFile(ActionEvent& ev);
		void handleExportContactsToServer(ActionEvent& ev);
		void handleExportContactsToFile(ActionEvent& ev);
		void handleRemoveContactsFromServer(ActionEvent& ev);
		void handleStatusDescription(ActionEvent& ev);
		void handleStatusServer(ActionEvent& ev);
		void handleStatusOnline(ActionEvent& ev);
		void handleStatusAway(ActionEvent& ev);
		void handleStatusInvisible(ActionEvent& ev);
		void handleStatusOffline(ActionEvent& ev);
		
	protected:
		static unsigned __stdcall ggWatchThread(LPVOID lParam);
		static void _stdcall ggPingTimer(LPVOID lParam, DWORD dwTimerLowValue, DWORD dwTimerHighValue);

		static unsigned __stdcall createAccount(LPVOID lParam);
		static unsigned __stdcall removeAccount(LPVOID lParam);
		static unsigned __stdcall changePassword(LPVOID lParam);
		static unsigned __stdcall remindPassword(LPVOID lParam);

	public:
		//API
		//void apiEnabled(IMEvent& ev);

	public:
		//f-cje
		void refreshServers(string serversString);
		void setProxy();
		string getPassword();
		bool checkConnection(unsigned short criterion = ccInternet | ccSession, bool warnUser = true);
		void sendMessage();

		void setCntStatus(Contact& cnt, tStatus status, string description = "", long ip = 0, int port = 0);
		void resetCnts();
		
		string getToken(Account& account, string title, string info);

	public:
		static void ggEventHandler(gg_event* event);

		static bool __stdcall operationAccountCancel(sDIALOG_long* dlg);
		static bool __stdcall operationAccountTimeout(int type, sDIALOG_long* dlg);
		static bool __stdcall operationPubDirCancel(sDIALOG_long* dlg);
		static bool __stdcall operationPubDirTimeout(int type, sDIALOG_long* dlg);

	protected:
		//zmienne wewnêtrzne
		ThreadRunner threads;
		HANDLE sessionThread;
		HANDLE accountThread;
		Session* gg;
		Account* account;
		tServers servers;
		TimerBasic timer;
		sDIALOG_long operationDlg;

	public:
		//zmienne zewnêtrzne
	};
}