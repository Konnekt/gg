#include "stdafx.h"

namespace GG {
	int convertKStatus(tStatus status, string description, bool friendsOnly) {
		int result;
		if (status == ST_ONLINE)
			result = description.empty() ? GG_STATUS_AVAIL : GG_STATUS_AVAIL_DESCR;
		else if (status == ST_AWAY)
			result = description.empty() ? GG_STATUS_BUSY : GG_STATUS_BUSY_DESCR;
		else if (status == ST_HIDDEN)
			result = description.empty() ? GG_STATUS_INVISIBLE : GG_STATUS_INVISIBLE_DESCR;
		else if (status == ST_OFFLINE)
			result = description.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;
		else if (status == ST_IGNORED)
			result = GG_STATUS_BLOCKED;
		else
			result = description.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;

		if (friendsOnly)
			result |= GG_STATUS_FRIENDS_MASK;
		return result;
	}

	tStatus convertGGStatus(int status) {
		if (status & GG_STATUS_AVAIL | GG_STATUS_AVAIL_DESCR)
			return ST_ONLINE;
		else if (status & GG_STATUS_BUSY | GG_STATUS_BUSY_DESCR)
			return ST_AWAY;
		else if (status & GG_STATUS_INVISIBLE | GG_STATUS_INVISIBLE_DESCR)
			return ST_HIDDEN;
		else if (status & GG_STATUS_NOT_AVAIL | GG_STATUS_NOT_AVAIL_DESCR)
			return ST_OFFLINE;
		else if (status & GG_STATUS_BLOCKED)
			return ST_BLOCKING;
		else
			return ST_OFFLINE;
	}
}