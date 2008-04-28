#pragma once

#define TIMER_INTERVAL 15 * 1000
#define PING_INTERVAL 120 * 1000
#define MSG_TIMEOUT 20

namespace GG {
	//todo: Net w jakiœ sposób bêdzie dynamiczny.
	const unsigned net = 10; // wartoœæ NET
	const int sig = (int)"GG"; //sygnatura
	const int name = (int)"Gadu-Gadu"; //nazwa
	
	const char defaultServers[] = "\r\n91.197.13.17\r\n91.197.13.2\r\n91.197.13.28\r\n91.197.13.26\r\n91.197.13.18\r\n91.197.13.12\r\n91.197.13.27\r\n91.197.13.24\r\n91.197.13.13\r\n91.197.13.29\r\n91.197.13.21\r\n91.197.13.25\r\n91.197.13.20\r\n91.197.13.32\r\n91.197.13.31\r\n91.197.13.33\r\n91.197.13.19\r\n91.197.13.49";
	const unsigned serversCount = 25;

	namespace CFG {
		const unsigned group = net * 1000;
		const unsigned login = group + 1;
		const unsigned password = group + 2;
		const unsigned status = group + 3;
		const unsigned startStatus = group + 4;
		const unsigned description = group + 5;
		const unsigned friendsOnly = group + 6;
		const unsigned servers = group + 7;
		const unsigned selectedServer = group + 8;
		const unsigned useSSL = group + 9;
		const unsigned resumeDisconnected = group + 10;

		/* todo: Stare id kolumn; zapewne przydadz¹ siê przy pisaniu f-cji updatuj¹cej plugin.
		const unsigned login = 1053;
		const unsigned password = 1054;
		const unsigned status = 1055;
		const unsigned startStatus = 1056;
		const unsigned description = 1057;
		const unsigned friendsOnly = 1058;
		const unsigned servers = 1059;
		const unsigned selectedServer = 1060;
		const unsigned trayMenu = 1061;
		const unsigned useSSL = 1063;
		const unsigned resumeDisconnected = 1064;*/
	};

	namespace ACT {
		const unsigned setDefaultServers = net * 1000 + 200;
		const unsigned createGGAccount = net * 1000 + 201;
		const unsigned removeGGAccount = net * 1000 + 202;
		const unsigned changePassword = net * 1000 + 203;
		const unsigned remindPassword = net * 1000 + 204;
		const unsigned importCntList = net * 1000 + 205;
		const unsigned exportCntList = net * 1000 + 206;
		const unsigned deleteCnts = net * 1000 + 207;
		const unsigned refreshCnts = net * 1000 + 208;

		const unsigned status = net * 1000 + 209;
		const unsigned statusServers = net * 1000 + 210;
		const unsigned statusDescripton = net * 1000 + 211;
		const unsigned statusOnline = net * 1000 + 212;
		const unsigned statusAway = net * 1000 + 213;
		const unsigned statusInvisible = net * 1000 + 214;
		const unsigned statusOffline = net * 1000 + 215;
		
		//tu rezerwujemy serversCount identyfikatorów, pocz¹wszy od poni¿szego;
		const unsigned statusServer = net * 1000 + 250;
	};

	namespace ICO {
		const unsigned server = net * 1000 + 300;
		const unsigned logo = UIIcon(IT_LOGO, net, 0, 0);
		const unsigned overlay = UIIcon(IT_OVERLAY, net, 0, 0);
		const unsigned online = UIIcon(IT_STATUS, net, ST_ONLINE, 0);
		const unsigned away = UIIcon(IT_STATUS, net, ST_AWAY, 0);
		const unsigned invisible = UIIcon(IT_STATUS, net, ST_HIDDEN, 0);
		const unsigned offline = UIIcon(IT_STATUS, net, ST_OFFLINE, 0);
		const unsigned blocking = UIIcon(IT_STATUS, net, ST_BLOCKING, 0);
		const unsigned connecting = UIIcon(IT_STATUS, net, ST_CONNECTING, 0);
	};

	enum userListRequest {
		ulrNone, ulrPut, ulrGet, ulrClear, ulrDone
	};
	typedef map <int, int> tEventHandler;

	extern bool onRequest;
	extern HANDLE ggThread;
	extern int ggThreadId;
	extern UINT_PTR timer;
	extern struct gg_session *sess;
	extern int loop;
	extern bool inAutoAway;
	extern CStdString lastServer;
	extern CStdString currentServer;
	extern userListRequest onUserListRequest;
	extern CStdString userListBuffer;

	extern CStdString curStatusInfo;
	extern int curStatus;

	// Eventy, 
	extern tEventHandler eventHandler;

	extern int sessionUsage;

	// Sprawdzanie dostarczenia wiadomosci
	struct cMsgSent {
		int msgID;
		time_t sentTime;
		CStdString digest;
		int Uid;
	};
	extern CRITICAL_SECTION msgSent_CS;
	typedef map <int, cMsgSent> tMsgSent;
	extern tMsgSent msgSent;

	// Wyszukiwanie
	struct cGGSearch {
		int cnt; // nr kontaktu lub -1 jesli search
		bool store; // zapisywane do wlasciwosci kontaktu
		bool finished;
	};
	extern CRITICAL_SECTION searchMap_CS;
	extern map <int, cGGSearch*> searchMap;
};

using namespace GG;