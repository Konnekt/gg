#pragma once

#include "GG.h"
#include "Helpers.h"
#include "Dialogs.h"
#include "UserList.h"

namespace GG {
	class Controller : public iController {

	public:
		//typy
		enum checkCriterion {
			ccInternet = 2,
			ccServer = 4,
			ccData = 8
		};
		
		struct statusInfo {
			tStatus status;
			const char* description;
			
			statusInfo(tStatus status, const char* description = "") {
				this->status = status;
				this->description = description;
			}
		};

	protected:
		Controller();

	protected:
		//IMessage
		void onPrepareUI(IMEvent& ev);
		void onStart(IMEvent& ev);
		void onEnd(IMEvent& ev);
		void onBeforeEnd(IMEvent& ev);
		void onDisconnect(IMEvent& ev);
		void onGetStatus(IMEvent& ev);
		void onGetStatusInfo(IMEvent& ev);
		void onGetUID(IMEvent& ev);
		void onCntAdd(IMEvent& ev);
		void onCntRemove(IMEvent& ev);
		void onCntChanged(IMEvent& ev);
		void onCntDownload(IMEvent& ev);
		void onCntUpload(IMEvent& ev);
		void onCntSearch(IMEvent& ev);
		void onIgnChanged(IMEvent& ev);
		void onIsConnected(IMEvent& ev);
		void onChangeStatus(IMEvent& ev);

	public:
		//akcje
		void handleSetDefaultServers(ActionEvent& ev);
		void handleCreateGGAccount(ActionEvent& ev);
		void handleRemoveGGAccount(ActionEvent& ev);
		void handleChangePassword(ActionEvent& ev);
		void handleRemindPassword(ActionEvent& ev);
		void handleImportCntList(ActionEvent& ev);
		void handleExportCntList(ActionEvent& ev);
		void handleStatusDescription(ActionEvent& ev);
		void handleStatusOnline(ActionEvent& ev);
		void handleStatusAway(ActionEvent& ev);
		void handleStatusInvisible(ActionEvent& ev);
		void handleStatusOffline(ActionEvent& ev);

	public:
		//API
		//void apiEnabled(IMEvent& ev);

	public:
		//proste inline'y
		inline bool isConnected() {
			return connected;
		}

		inline tStatus getStatus() {
			return status;
		}
		
		inline string getStatusDescription() {
			return statusDescription;
		}

	public:
		//f-cje
		void setProxy();
		string getPassword();
		bool checkConnection(unsigned short criterion = ccInternet | ccServer, bool warnUser = true);
		void connect(tStatus status, const char* description = "");
		void setStatus(tStatus status, const char* description = "");
		void sendMessage();
		void disconnect(const char* description = "");

	public:
		static unsigned __stdcall connectProc(LPVOID param);

	protected:
		//zmienne wewnêtrzne
		bool connected;
		bool connecting;
		tStatus status;
		string statusDescription;
		gg_session* session;
		HANDLE connectThread;

	public:
		//zmienne zewnêtrzne
	};
}