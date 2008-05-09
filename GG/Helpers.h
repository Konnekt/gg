#pragma once

namespace GG {
	int convertKStatus(tStatus status, string description = "", bool friendsOnly = false);
	tStatus convertGGStatus(int status);
	inline int getCntType(tStatus status) {
		return (status & ST_IGNORED) ? GG_USER_BLOCKED : (status & ST_HIDEMYSTATUS) ? GG_USER_OFFLINE : GG_USER_NORMAL;
	}
}