#pragma once

#include "GG.h"

#include "Config.h"


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
  /* IMessage callback methods */
  void _prepare(IMEvent& ev);
  void _prepareUI(IMEvent& ev);

  void onEnd(IMEvent& ev);
  void onPluginsLoaded(IMEvent& ev);
  void onExtAutoAway();
  void onAutoAway(IMEvent& ev);
  void onBack(IMEvent& ev);

public:
  // actions
  void _handleCntGroup(ActionEvent& ev);
  void _handleMsgTb(ActionEvent& ev);
  void _handlePowerBtns(ActionEvent& ev);
  void _handleIgnoreBtn(ActionEvent& ev);
  void _clearMRU(ActionEvent& ev);
  void _resetGlobalSettings(ActionEvent& ev);
  void _resetContactSettings(ActionEvent& ev);

public:
  /* API callback methods */
  void apiEnabled(IMEvent& ev);

public:

protected:
	//zmienne wewnêtrzne

public:
	//zmienne wewnêtrzne
};