#include "stdafx.h"
#include "GG.h"
#include "Controller.h"

using Stamina::str_tr;
using Stamina::RegEx;
using Stamina::SXML;
using Stamina::chtoint;

int GG::check(bool conn, bool session, bool login, bool warn) {
	bool err = 0;
	/* TODO : Rozsadne anulowanie polaczen... */
	/*if (conn && !session && gg_connect(0,0,0) && !sess) {
		err = 1;
			if (warn) ICMessage(IMI_ERROR, (int)"Poczekaj .. Po≥πczenie juø jest zajÍte!");
	} else*/
	if (conn && !ICMessage(IMC_CONNECTED)) {
		err = 1;
			if (warn) ICMessage(IMI_ERROR, (int)"Aby dzia≥aÊ na serwerze GG musisz byÊ po≥πczony z internetem!\nJeøeli jesteú po≥πczony sprawdü czy prawid≥owo skonfigurowa≥eú po≥πczenie w konfiguracji!", MB_TASKMODAL|MB_OK);
	} else if (session && !sess) {
		err = 1;
		if (warn) ICMessage(IMI_ERROR, (int)"Musisz byÊ po≥πczony z serwerem GG!", MB_TASKMODAL);
	} else if (login && !GETINT(CFG_GG_LOGIN)) {
		err = 1;
		if (warn) ICMessage(IMI_ERROR, (int)"Musisz ustawiÊ login i has≥o konta GG!\nJeøeli nie masz konta, za≥Ûø je przyciskiem w konfiguracji.", MB_TASKMODAL|MB_OK);
	}

	if (err) {
		Ctrl->setError(IMERROR_NORESULT);
		return 0;
	} else {
		return 1;
	}
}

void GG::getAccount(int& login, CStdString& pass) {
	login = GETINT(CFG_GG_LOGIN);
	pass = GETSTR(CFG_GG_PASS);
	if (!login || pass.empty()) {
		sDIALOG_access sda;
		sda.flag = DFLAG_SAVE;
		sda.title = "Has≥o do konta GG";
		sda.info = "Aby wykonaÊ wybranπ operacjÍ, musisz podaÊ has≥o do swojego konta GG.";
		sda.save = false;
		sda.login = (char*)GETSTR(CFG_GG_LOGIN);
		sda.pass = "";

		if (!ICMessage(IMI_DLGLOGIN, (int)&sda)) return;
		pass = sda.pass;
		login = atoi(sda.login);
		if (sda.save) {
			SETINT(CFG_GG_LOGIN, login);
			SETSTR(CFG_GG_PASS, pass);
		}
	}
}

void GG::quickEvent(int Uid, const char* body, const char* ext, int flag) {
	cMessage m;
	m.net = GG::net;
	m.type = MT_QUICKEVENT;
	std::string uid = inttostr(Uid);
	m.fromUid = (char*)uid.c_str();
	m.toUid = "";
	m.body = (char*)body;
	m.ext = (char*)ext;
	m.flag = MF_HANDLEDBYUI;
	m.action = NOACTION;
	m.notify = 0;
	ICMessage(IMC_NEWMESSAGE, (int)&m, 0);
}

int GG::userType(int id) {
	int cntStatus = GETCNTI(id, CNT_STATUS);
	return (cntStatus & ST_IGNORED) ? GG_USER_BLOCKED : (cntStatus & ST_HIDEMYSTATUS) ? GG_USER_OFFLINE : GG_USER_NORMAL;
}

CStdString GG::msgToHtml(CStdString msg, void * formats, int formats_length) {
	/* Zamiast znacznikÛw wstawia:
	 < - 1
	 > - 2
	 " - 3
	*/
	str_tr((char*)msg.c_str(), "\1\2\3", "   ");
	// Przeglπdamy listÍ i modyfikujemy...
	void * formats_end = (char*)formats + formats_length;
	CStdString msg2 = "";
	struct cOpened {
		unsigned int bold :1;
		unsigned int italic : 1;
		unsigned int under : 1;
		unsigned int color: 1;
		void finish(CStdString & txt, int type) {
			if (color) {txt+="\1/font\2"; color = 0;}
			if (!(type & GG_FONT_UNDERLINE) && under) {txt+="\1/u\2"; under = 0;}
			if (!(type & GG_FONT_ITALIC) && italic) {txt+="\1/i\2"; italic = 0;}
			if (!(type & GG_FONT_BOLD) && bold) {txt+="\1/b\2";bold = 0;}
		}
	} opened;
	opened.bold = opened.italic = opened.under = opened.color = 0;
	size_t pos = 0;
	while (formats < formats_end) {
		gg_msg_richtext_format * rf = (gg_msg_richtext_format*)formats;
		if (rf->position >= msg.size())
			break; // b≥πd
		msg2 += msg.substr(pos, rf->position - pos);
		pos = rf->position;
		opened.finish(msg2, rf->font);
		if ((rf->font & GG_FONT_BOLD) && !opened.bold) {
			msg2+="\1b\2";
			opened.bold=1;
		}
		if ((rf->font & GG_FONT_ITALIC) && !opened.italic) {
			msg2+="\1i\2";
			opened.italic=1;
		}
		if ((rf->font & GG_FONT_UNDERLINE) && !opened.under) {
			msg2+="\1u\2";
			opened.under=1;
		}
		formats = rf+1;
		if (rf->font & GG_FONT_COLOR) {
			gg_msg_richtext_color * rc = (gg_msg_richtext_color*) formats;
			if (rc->red || rc->green || rc->blue) {
				opened.color=1;
				msg2 += "\1font color=\3#";
				msg2 += inttostr(rc->red, 16, 2, true);
				msg2 += inttostr(rc->green, 16, 2, true);
				msg2 += inttostr(rc->blue, 16, 2, true);
				msg2 += "\3\2";
			} else opened.color = 0;
			formats = rc+1;
		}
	}
	msg2 += msg.substr(pos);
	opened.finish(msg2, 0);
	msg = msg2;
	msg2.clear();
	// KoÒcowy efekt "enkodujemy"
	RegEx pr;
	msg = pr.replace("/[^ a-z0-9\1\2\3\\!\\@\\#\\$\\%\\*\\(\\)\\-_=+\\.\\,\\;':\\\\[\\]\\{\\}\\/\\?πÊÍ≥ÒÛúüø•∆ £—”åèØ]/i", Stamina::encodeCallback, msg);
	msg = pr.replace("/\\n/", "<br/>", msg);
	// Zamieniamy znaki kontrolne w znaczniki HTML
	str_tr((char*)msg.c_str(), "\1\2\3", "<>\"");
	return msg;
};

struct FormatState {
	int bold, under, italic, color;
	char rgb[3];
	FormatState() {
		bold = under = italic = color = 0;
	}
};

CStdString GG::htmlToMsg(CStdString msgIn, void * formats, int & length) {
	int max_len = length;
	length = 0;
	CStdString msg;
	SXML XML;
	RegEx preg;
	msgIn = preg.replace("#\\r|\\n#", "", msgIn);
	msgIn = preg.replace("#<br/?>#i", "\n", msgIn.c_str());
	XML.loadSource(msgIn);
	SXML::NodeWalkInfo ni;
	size_t last = 0;
	void * formats_start = formats;
	gg_msg_richtext * rt = (gg_msg_richtext*) formats;
	formats = rt+1;
	void * formats_last = formats;
	memset(formats, 0, sizeof(gg_msg_richtext_format));
/*
bleeeee<b>bold<b><font color="#FF0000">gnieø<u>døony</u></font></b>i <i>jesz</i>cze</b>koniec
*/
	FormatState state;
	stack <FormatState> spanStack;

	while (length < max_len - 20 && XML.nodeWalk(ni)) {
		XML.pos.start = XML.pos.end;
		XML.pos.start_end = XML.pos.end_end;
		msg += Stamina::decodeEntities(msgIn.substr(last, ni.start - last));
		last = ni.end;
		CStdString token = ni.path.substr(ni.path.find_last_of('/')+1);
		token.MakeLower();
		int oper = (ni.state == SXML::NodeWalkInfo::opened)? 1 : -1;
		if (token == "b" || token == "strong") {
			state.bold+=oper;
		} else if (token == "i") {
			state.italic+=oper;
		} else if (token == "u") {
			state.under+=oper;
		} else if (token == "font") {
			// Kolor musimy "zamknπÊ"
			if (oper > 0) state.color++;
			int c = chtoint(XML.getAttrib("color").c_str(), -1);
			state.rgb[0] = (c & 0xFF0000) >> 16;
			state.rgb[1] = (c & 0xFF00) >> 8;
			state.rgb[2] = (c & 0xFF);
		} else if (token == "span") {
			if (oper > 0) {
				spanStack.push(state);
				using Stamina::RegEx;
				static RegEx::oCompiled rcWeight(new RegEx::Compiled("/font-weight:bold/i"));
				//static RegEx::oCompiled rcSize = new RegEx::Compiled("/font-size:(\\d+)/i");
				static RegEx::oCompiled rcColor(new RegEx::Compiled("/color:([a-z]+|#[0-9A-F]+)/i"));
				static RegEx::oCompiled rcItalic(new RegEx::Compiled("/font-style:italic/i"));
				static RegEx::oCompiled rcUnderline(new RegEx::Compiled("/text-decoration:underline/i"));
				CStdString style = XML.getAttrib("style");
				if (RegEx::doMatch(rcWeight, style)) {
					state.bold += oper;
				}
				if (RegEx::doMatch(rcItalic, style)) {
					state.italic += oper;
				}
				if (RegEx::doMatch(rcUnderline, style)) {
					state.under += oper;
				}
				CStdString colorStr = RegEx::doGet(rcColor, style, 1);
				if (!colorStr.empty()) {
					int color = -1;
					if (colorStr[0] == '#') {
						color = Stamina::chtoint(colorStr, -1);
					} else if (colorStr == "red") {
						color = 0xFF0000;
					} else if (colorStr == "blue") {
						color = 0x0000FF;
					} else if (colorStr == "green") {
						color = 0x00FF00;
					}
					if (color > 0) {
						state.color++;
						state.rgb[0] = (color & 0xFF0000) >> 16;
						state.rgb[1] = (color & 0xFF00) >> 8;
						state.rgb[2] = (color & 0xFF);
					}
				}
			} else {
				if (spanStack.size()) {
					state = spanStack.top();
					spanStack.pop();
				}
			}
		}
		gg_msg_richtext_format * rf = (gg_msg_richtext_format*)formats_last;
		// Jeøeli stoimy w tym samym miejscu, wypada po≥πczyÊ si≥y...
		if (rf->position == msg.size())
			formats = formats_last;
		rf = (gg_msg_richtext_format*)formats;
		rf->position = msg.size();
		rf->font = (state.bold > 0?GG_FONT_BOLD:0) | (state.italic > 0?GG_FONT_ITALIC:0) | (state.under > 0?GG_FONT_UNDERLINE:0) | (state.color > 0?GG_FONT_COLOR:0);
		formats_last = formats;
		formats = rf+1;
		if (state.color) {
			gg_msg_richtext_color * rc = (gg_msg_richtext_color*) formats;
			rc->red = state.rgb[0];
			rc->green = state.rgb[1];
			rc->blue = state.rgb[2];
			formats = rc+1;
		}
		length = (char*)formats - (char*)formats_start;
		// "Zamykamy" kolor
		if (token == "font" && oper < 0)
			state.color--;
	}
	msg += Stamina::decodeEntities(msgIn.substr(ni.end));
	rt->flag = length>sizeof(gg_msg_richtext)?2:0;
	rt->length = length - sizeof(gg_msg_richtext);
	return msg;
};