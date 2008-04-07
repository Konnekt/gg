#include "StdAfx.h"
#include "Controller.h"

namespace GG {
	int convertKStatus(tStatus status, string description) {
		switch (status) {
			case ST_ONLINE: {
				return description.empty() ? GG_STATUS_AVAIL : GG_STATUS_AVAIL_DESCR;
			} case ST_AWAY: {
				return description.empty() ? GG_STATUS_BUSY : GG_STATUS_BUSY_DESCR;
			} case ST_HIDDEN: {
				return description.empty() ? GG_STATUS_INVISIBLE : GG_STATUS_INVISIBLE_DESCR;
			} case ST_OFFLINE: {
				return description.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;
			} case ST_IGNORED: {
				return GG_STATUS_BLOCKED;
			} default: {
				return description.empty() ? GG_STATUS_NOT_AVAIL : GG_STATUS_NOT_AVAIL_DESCR;
			}
		}
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
}