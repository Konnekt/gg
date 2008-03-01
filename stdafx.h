#pragma once

#define _WIN32_WINNT 0x0500
#define ASSIGN_SOCKETS_TO_THREADS
#include <Winsock2.h>
#include <windows.h>
#include <deque>
#include <stdstring.h>
#include <map>
#include <vector>
#include <stack>
#include <commctrl.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#include "libgadu\compat_w32.h"
#include "libgadu\libgadu.h"

#include "konnekt/plug.h"
#include "konnekt/ui.h"
#include "konnekt/plug_export.h"
#include "konnekt/plug_func.h"
#include "konnekt/gg.h"
#include "konnekt/ui_message_controls.h"

#include <Stamina\Helpers.h>
#include <Stamina\time64.h>
#include <Stamina\RegEx.h>
#include <Stamina\SimXML.h>
#include <Stamina\TLS.h>

using namespace std;
