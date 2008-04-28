#pragma once

#include "GG.h"
#include "GGSession.h"

namespace GG {
	class Controller : public iController {

	public:
		//typy
		enum checkCriterion {
			ccInternet = 2,
			ccServer = 4,
			ccData = 8
		};
		
		struct Server {
			string ip;
			bool selected;
			bool ssl;
			
			Server (string ip = "", bool ssl = false, bool selected = false) {
				this->ip = ip;
				this->ssl = ssl;
				this->selected = selected;
			}
		};
		
		typedef vector<Server> tServers;

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
		void handleImportCntList(ActionEvent& ev);
		void handleExportCntList(ActionEvent& ev);
		void handleStatusDescription(ActionEvent& ev);
		void handleStatusServer(ActionEvent& ev);
		void handleStatusOnline(ActionEvent& ev);
		void handleStatusAway(ActionEvent& ev);
		void handleStatusInvisible(ActionEvent& ev);
		void handleStatusOffline(ActionEvent& ev);

	public:
		//API
		//void apiEnabled(IMEvent& ev);

	public:
		//f-cje
		void refreshServers(string serversString);
		void setProxy();
		string getPassword();
		bool checkConnection(unsigned short criterion = ccInternet | ccServer, bool warnUser = true);
		void sendMessage();

		void setCntStatus(Contact& cnt, tStatus status, string description = "", long ip = 0, int port = 0);
		void resetCnts();

	public:
		static void ggEventHandler(gg_event* event);

	protected:
		//zmienne wewnêtrzne
		ThreadRunner threads;
		GGSession gg;
		vector<Server> servers;

	public:
		//zmienne zewnêtrzne
	};
}