#pragma once

namespace GG {
	unsigned int __stdcall createGGAccount(LPVOID lParam);
	unsigned int __stdcall removeGGAccount(LPVOID lParam);
	unsigned int __stdcall changePassword(LPVOID lParam);
	unsigned int __stdcall remindPassword(LPVOID lParam);
};