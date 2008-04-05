#pragma once

#include "GG.h"
#include "Dialogs.h"
#include "UserList.h"

namespace GG {
	class Controller : public iController {

	public:
		//typy

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

	public:
		//API
		//void apiEnabled(IMEvent& ev);

	public:
		//f-cje
		void setProxy();

	protected:
		//zmienne wewnêtrzne

	public:
		//zmienne zewnêtrzne
	};
}