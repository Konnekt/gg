#include "StdAfx.h"
#include "Controller.h"
#include "Helpers.h"
#include "Dialogs.h"
#include "UserList.h"

namespace GG {
	int convertKStatus(tStatus status, string description) {
		int result;
		switch (status) {
			case ST_ONLINE: {
				result = description.empty() ? GG_STATUS_AVAIL : GG_STATUS_AVAIL_DESCR;
				break;
			} case ST_AWAY: {
				result = description.empty() ? GG_STATUS_BUSY : GG_STATUS_BUSY_DESCR;
				break;
			} case ST_HIDDEN: {
				result = description.empty() ? GG_STATUS_INVISIBLE : GG_STATUS_INVISIBLE_DESCR;
				break;
			} case ST_OFFLINE: {
				result = description.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;
				break;
			} case ST_IGNORED: {
				result = GG_STATUS_BLOCKED;
				break;
			} default: {
				result = description.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;
				break;
			}
		}
		if (GETINT(CFG::friendsOnly))
			result |= GG_STATUS_FRIENDS_MASK;
		return result;
	}
	
	tStatus convertGGStatus(int status) {
		switch (status) {
			case GG_STATUS_AVAIL:
			case GG_STATUS_AVAIL_DESCR: {
				return ST_ONLINE;
			} case GG_STATUS_BUSY:
			case GG_STATUS_BUSY_DESCR: {
				return ST_AWAY;
			} case GG_STATUS_INVISIBLE:
			case GG_STATUS_INVISIBLE_DESCR: {
				return ST_HIDDEN;
			} case GG_STATUS_NOT_AVAIL:
			case GG_STATUS_NOT_AVAIL_DESCR: {
				return ST_OFFLINE;
			} case GG_STATUS_BLOCKED: {
				return ST_IGNORED;
			} default: {
				return ST_OFFLINE;
			}
		}
	}
	
	Controller::tServers getServers(string serversString, string selected) {
		Controller::tServers servers;
		size_t a = 0;
		size_t b = 0;
		string server;
		bool ssl;
		bool hub = false;

		for (;;) {
			b = serversString.find("\r\n", a);
			server = serversString.substr(a, b - a);
			if (server.substr(0, 4) == "SSL ") {
				ssl = true;
				server = server.substr(4);
			} else {
				ssl = false;
			}
			if (server.empty())
				server = "HUB";
			if (server != "HUB" || !hub)
				servers.push_back(Controller::Server(server, ssl, server == selected));
			if (server == "HUB" && !hub)
				hub = true;
			if (b == string::npos)
				break;
			a = b + 2;
		}
		
		return servers;
	}

	int getCntType(tStatus status) {
		return (status & ST_IGNORED) ? GG_USER_BLOCKED : (status & ST_HIDEMYSTATUS) ? GG_USER_OFFLINE : GG_USER_NORMAL;
	}
}