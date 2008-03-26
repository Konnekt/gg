#include "stdafx.h"
#include "Controller.h"

namespace GG {
	Controller::Controller() {
		IMessageDispatcher& d = getIMessageDispatcher();
		ActionDispatcher& a = getActionDispatcher();
		Config& c = getConfig();

		//statyczne
		d.setStaticValue(IM_PLUG_NET, GG::Net);
		d.setStaticValue(IM_PLUG_SIG, (int) GG::Sig);
		d.setStaticValue(IM_PLUG_NAME, (int) GG::Name);
		d.setStaticValue(IM_PLUG_TYPE, imtMessage | imtProtocol | imtContact | imtConfig | imtUI | imtNet | imtNetSearch | imtMessageAck | imtMsgUI| imtNetUID);
		d.setStaticValue(IM_PLUG_PRIORITY, priorityLow);
		d.setStaticValue(IM_PLUG_NETNAME, (int) "Gadu-Gadu");
		d.setStaticValue(IM_PLUG_NETSHORTNAME, (int) "GG");
		d.setStaticValue(IM_PLUG_UIDNAME, (int) "#");
		d.setStaticValue(IM_MSG_CHARLIMIT, 2000);

		//IMessage
		d.connect(IM_UI_PREPARE, bind(&Controller::onPrepareUI, this, _1));
		d.connect(IM_START, bind(&Controller::onStart, this, _1));
		d.connect(IM_END, bind(&Controller::onEnd, this, _1));
		d.connect(IM_DISCONNECT, bind(&Controller::onDisconnect, this, _1));
		d.connect(IM_GET_STATUS, bind(&Controller::onGetStatus, this, _1));
		d.connect(IM_GET_STATUSINFO, bind(&Controller::onGetStatusInfo, this, _1));
		d.connect(IM_GET_UID, bind(&Controller::onGetUID, this, _1));
		d.connect(IM_MSG_RCV, bind(&Controller::onMsgRcv, this, _1));
		d.connect(IM_MSG_SEND, bind(&Controller::onMsgSend, this, _1));
		d.connect(IM_CNT_ADD, bind(&Controller::onCntAdd, this, _1));
		d.connect(IM_CNT_REMOVE, bind(&Controller::onCntRemove, this, _1));
		d.connect(IM_CNT_CHANGED, bind(&Controller::onCntChanged, this, _1));
		d.connect(IM_CNT_DOWNLOAD, bind(&Controller::onCntDownload, this, _1));
		d.connect(IM_CNT_UPLOAD, bind(&Controller::onCntUpload, this, _1));
		d.connect(IM_CNT_SEARCH, bind(&Controller::onCntSearch, this, _1));
		d.connect(IM_IGN_CHANGED, bind(&Controller::onIgnChanged, this, _1));
		d.connect(IM_ISCONNECTED, bind(&Controller::onIsConnected, this, _1));
		d.connect(IM_CHANGESTATUS, bind(&Controller::onChangeStatus, this, _1));
		d.connect(IM_ISCONNECTED, bind(&Controller::onIsConnected, this, _1));

		//API
		//d.connect(api::isEnabled, bind(&Controller::apiEnabled, this, _1));

		//Akcje
		//a.connect(ui::cntCfgGroup, bind(&Controller::_handleCntGroup, this, _1));

		//Konfiguracja
		c.setColumn(tableConfig, Cfg::login, ctypeString, "", "GG/login");
		c.setColumn(tableConfig, Cfg::password, ctypeString | cflagSecret | cflagXor, "", "GG/password");
		c.setColumn(tableConfig, Cfg::status, ctypeInt, 0, "GG/status");
		c.setColumn(tableConfig, Cfg::startStatus, ctypeInt, 0, "GG/startStatus");
		c.setColumn(tableConfig, Cfg::friendsOnly, ctypeInt, 0, "GG/friedsOnly");
		c.setColumn(tableConfig, Cfg::description, ctypeString, "", "GG/description");
		c.setColumn(tableConfig, Cfg::servers, ctypeString, "s1.gadu-gadu.pl\r\n\r\n217.17.41.83\r\n217.17.41.84\r\n217.17.41.85\r\n217.17.41.86\r\n217.17.41.87\r\n217.17.41.88\r\n217.17.41.92", "GG/servers");
		c.setColumn(tableConfig, Cfg::trayMenu, ctypeInt, 0, "GG/trayMenu");
		c.setColumn(tableConfig, Cfg::useSSL, ctypeInt, 0, "GG/useSSL");
		c.setColumn(tableConfig, Cfg::resumeDisconnected, ctypeInt, 0, "GG/resumeDisconnected");
	}
}