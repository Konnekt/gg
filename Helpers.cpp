#include "StdAfx.h"
#include "Controller.h"
#include "Helpers.h"
#include "Dialogs.h"
#include "UserList.h"

namespace GG {	
	tServers getServers(string serversString, string selected) {
		tServers servers;
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
				servers.push_back(Server(server, ssl, server == selected));
			if (server == "HUB" && !hub)
				hub = true;
			if (b == string::npos)
				break;
			a = b + 2;
		}
		
		return servers;
	}
}