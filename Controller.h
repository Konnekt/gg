#pragma once

#include "GG.h"
#include "Config.h"

namespace GG {
	class Controller : public iController<Controller> {
	public:
		friend class iController<Controller>;

	public:
		/**
		 * Class version macro
		 */
		STAMINA_OBJECT_CLASS_VERSION(Controller, iController, Version(1,3,0,0));

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
		void onMsgRcv(IMEvent& ev);
		void onMsgSend(IMEvent& ev);
		void onCntAdd(IMEvent& ev);
		void onCntRemove(IMEvent& ev);
		void onCntChanged(IMEvent& ev);
		void onCntDownload(IMEvent& ev);
		void onCntUpload(IMEvent& ev);
		void onCntSearch(IMEvent& ev);
		void onIgnChanged(IMEvent& ev);
		void onIsConnected(IMEvent& ev);
		void onChangeStatus(IMEvent& ev);
		void onIsConnected(IMEvent& ev);

	public:
		//akcje
		//void _handleCntGroup(ActionEvent& ev);

	public:
		//API
		//void apiEnabled(IMEvent& ev);

	public:
		//f-cje

	protected:
		//zmienne wewnêtrzne

	public:
		//zmienne zewnêtrzne
	};
}