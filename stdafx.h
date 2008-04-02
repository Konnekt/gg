//TODO: Dziwne, ale konieczna jest taka kolejnoœæ include'ów. Proponujê dalsze œledztwo, bo problem jest ciekawy.

// include just once
#pragma once

// allow use of features specific to Windows XP or later.
#ifndef _WIN32_WINNT
  #define _WIN32_WINNT 0x0501
#endif

// allow use of features specific to IE 6.0 or later.
#ifndef _WIN32_IE
  #define _WIN32_IE 0x0600
#endif

// exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

// wci¹gamy biblioteke libgadu
#pragma comment (lib, "libgadu.lib")
// i reszte
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "ws2_32.lib")

// Windows Header Files:
#include <windows.h>
#include <windowsx.h>
#include <process.h>
#include <commctrl.h>
#include <io.h>
#include <hash_map>
#include <list>
#include <deque>
#include <stdstring.h>
#include <string>
#include <sstream>

#pragma comment(lib, "comctl32.lib")

// hapsamy Stamina::Lib
#ifdef _DEBUG
  #pragma comment(lib, "stamina_d.lib")
#else
  #pragma comment(lib, "stamina.lib")
#endif

//Libgadu
#include <libgadu\compat_w32.h>
#include <libgadu\libgadu\include\libgadu.h>

// nag³ówki boosta
#include <boost/signal.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>

// nag³ówki S.L
/*
#include <stamina/threadrun.h>
#include <stamina/thread.h>
#include <stamina/threadinvoke.h>
*/

#include <stamina/helpers.h>
#include <stamina/object.h>
#include <stamina/objectimpl.h>
#include <stamina/exception.h>
#include <stamina/string.h>
#include <stamina/time64.h>
#include <stamina/timer.h>

using namespace Stamina;
using namespace std;

// nag³ówki Konnekta
#include <konnekt/plug_export.h>
#include <konnekt/ui.h>
#include <konnekt/plug_func.h>
#include <konnekt/knotify.h>
#include <konnekt/ksound.h>
#include <konnekt/gg.h>
#include <konnekt/ui_message_controls.h>
#include <konnekt/lib.h>
#include <konnekt/tabletka.h>
#include <konnekt/plugsNET.h>
#include <konnekt/contrib/iController.h>

using namespace Konnekt;
using namespace boost;

// plik z definicjami zasobów
#include "Resources/resource.h"

/**
 * Konnekt::ShowBits helpers
 */
#define ifPRO         if (Konnekt::ShowBits::checkLevel(Konnekt::ShowBits::levelPro))
#define ifADV         if (Konnekt::ShowBits::checkLevel(Konnekt::ShowBits::levelAdvanced))
#define ifNORM        if (Konnekt::ShowBits::checkLevel(Konnekt::ShowBits::levelNormal))
#define ifINT         if (Konnekt::ShowBits::checkLevel(Konnekt::ShowBits::levelIntermediate))

#define ifToolTipADV  if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showTooltipsAdvanced))
#define ifToolTipNORM if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showTooltipsNormal))
#define ifToolTipBEG  if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showTooltipsBeginner))

#define ifInfoADV     if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showInfoAdvanced))
#define ifInfoNORM    if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showInfoNormal))
#define ifInfoBEG     if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showInfoBeginner))

#define ifWizardsADV  if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showWizardsAdvanced))
#define ifWizardsNORM if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showWizardsNormal))
#define ifWizardsBEG  if (Konnekt::ShowBits::checkBits(Konnekt::ShowBits::showWizardsBeginner))