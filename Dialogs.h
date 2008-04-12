#pragma once

namespace GG {
	unsigned __stdcall createGGAccount(LPVOID lParam);
	unsigned __stdcall removeGGAccount(LPVOID lParam);
	unsigned __stdcall changePassword(LPVOID lParam);
	unsigned __stdcall remindPassword(LPVOID lParam);
	void importList();
	void exportList();
};