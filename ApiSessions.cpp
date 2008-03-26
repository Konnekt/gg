#include "stdafx.h"
#include "GG.h"
#include "Controller.h"

int GG::event(GGER_enum type, void* data) {
	sIMessage_GGEvent msg;
	msg.eventType = type;
	msg.data.pointer = data;
	msg.id = IM_GG_EVENT;
	int ret = 0;
	for (tEventHandler::iterator i = eventHandler.begin(); i != eventHandler.end(); i++) {
		if (i->second & type) {
			ret |= Ctrl->IMessageDirect(i->first, &msg);
		}
	}
	return ret;
}

void GG::waitOnSessions() {
	while (sessionUsage > 0) {
		Ctrl->Sleep(50);
	}
}
