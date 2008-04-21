#pragma once

namespace GG {
	int convertKStatus(tStatus status, string description = "");
	tStatus convertGGStatus(int status);
	Controller::tServers getServers(string serversString, string selected = "");
	int getCntType(tStatus status);
}