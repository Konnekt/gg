#pragma once

namespace GG {
	int convertKStatus(tStatus status, string description = "");
	tStatus convertGGStatus(int status);
}