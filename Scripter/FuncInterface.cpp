/*
//这是一个接口类，所有命令都继承这个类
//运行时优先尝试脚本类-->图像类-->音频类
*/
#include "FuncInterface.h"
#include <stdarg.h>

std::unordered_map<std::string, FunctionInterface::FuncLUT*> FunctionInterface::baseFuncMap;
FunctionInterface *FunctionInterface::imgeModule = NULL;
FunctionInterface *FunctionInterface::audioModule = NULL;
FunctionInterface *FunctionInterface::scriptModule = NULL;
FunctionInterface *FunctionInterface::fileModule = NULL;    //文件系统
bool FunctionInterface::isExit = false;
std::vector<LOString> FunctionInterface::workDirs;
std::atomic_int FunctionInterface::flagPrepareEffect{};
std::atomic_int FunctionInterface::flagRenderNew{};
LOString FunctionInterface::userGoSubName[3] = {"lons_pretextgosub__","lons_textgosub__",""};

//R-ref, I-realref, S-strref, N-normal word, L-label, A-arrayref, C-color
//r-any, i-real, s-string, *-repeat, #-repeat last
static FunctionInterface::FuncLUT func_lut[] = {
	//scripter module
	{"mov",      "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov3",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov4",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov5",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov6",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov7",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov8",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov9",     "\0",         "\0",    &FunctionInterface::movCommand},
	{"mov10",    "\0",         "\0",    &FunctionInterface::movCommand},

	{"movl",      "A,i,#",     "Y,Y,#",    &FunctionInterface::movlCommand},
	{"cos",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"dec",      "I",         "Y",         &FunctionInterface::operationCommand},
	{"inc",      "I",         "Y",         &FunctionInterface::operationCommand},
	{"div",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"mul",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"rnd",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"rnd2",     "I,i,i",     "Y,Y,Y",     &FunctionInterface::operationCommand},
	{"sin",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"sub",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"tan",      "I,i",       "Y,Y",       &FunctionInterface::operationCommand},
	{"dim",      "A",         "Y",         &FunctionInterface::dimCommand},
	{"add",      "R,r",       "Y,Y",       &FunctionInterface::addCommand},
	{"intlimit", "i,i,i",     "Y,Y,Y",     &FunctionInterface::intlimitCommand},
	{"itoa",     "S,i",       "Y,Y",       &FunctionInterface::itoaCommand},
	{"itoa2",    "S,i",       "Y,Y",       &FunctionInterface::itoaCommand},
	{"len",		 "I,s",       "Y,Y",       &FunctionInterface::lenCommand},
	{"lenword",  "I,s",       "Y,Y",       &FunctionInterface::lenCommand},
	{"mid",		 "S,s,i,i",   "Y,Y,Y,Y",   &FunctionInterface::midCommand},
	{"midword",	 "S,s,i,i",   "Y,Y,Y,Y",   &FunctionInterface::midCommand},
	{"debuglog", "r,*",       "Y,*",       &FunctionInterface::debuglogCommand},
	{"numalias", "N,i",       "Y,Y",       &FunctionInterface::numaliasCommand},
	{"stralias", "N,s",       "Y,Y",       &FunctionInterface::straliasCommand},
	{"gosub",    "L",         "Y",         &FunctionInterface::gotoCommand},
	{"goto",     "L",         "Y",         &FunctionInterface::gotoCommand},
	{"jumpb",     "\0",       "\0",        &FunctionInterface::jumpCommand},
	{"jumpf",     "\0",       "\0",        &FunctionInterface::jumpCommand},
	{"return",    "L",        "N",         &FunctionInterface::returnCommand},
	{"skip",      "i",        "Y",         &FunctionInterface::skipCommand},
	{"tablegoto", "I,L",      "Y,Y,#",     &FunctionInterface::tablegotoCommand},
	{"for",       "\0",       "\0",        &FunctionInterface::forCommand},
	{"next",      "\0",       "\0",        &FunctionInterface::nextCommand},
	{"break",     "\0",       "\0",        &FunctionInterface::breakCommand},
	{"if",        "\0",       "\0",        &FunctionInterface::ifCommand},
	{"notif",     "\0",       "\0",        &FunctionInterface::ifCommand},
	{"else",      "\0",       "\0",        &FunctionInterface::elseCommand},
	{"endif",     "\0",       "\0",        &FunctionInterface::endifCommand},
	{"resettimer","\0",       "\0",        &FunctionInterface::resettimerCommand},
	{"gettimer",  "I",        "Y",         &FunctionInterface::gettimerCommand},
	{"delay",     "i",        "Y",         &FunctionInterface::delayCommand},
	{"wait",      "i",        "Y",         &FunctionInterface::delayCommand},
	{"waittimer", "i",        "Y",         &FunctionInterface::delayCommand},
	{"game",      "\0",       "\0",        &FunctionInterface::gameCommand},
	{"end",       "\0",       "\0",        &FunctionInterface::endCommand},
	{"defsub",    "N",        "Y",         &FunctionInterface::defsubCommand},
	{"getparam",  "r,#",      "Y,#",       &FunctionInterface::getparamCommand},
	{"labelexist", "I,L",     "Y,Y",       &FunctionInterface::labelexistCommand},
	{"fileexist",  "I,s",     "Y,Y",       &FunctionInterface::fileexistCommand},
	{"fileremove",  "s",      "Y",         &FunctionInterface::fileremoveCommand},
	{"readfile",   "S,s",     "Y,Y",       &FunctionInterface::readfileCommand},
	{"split",      "s,s,r,#", "Y,Y,Y,#",   &FunctionInterface::splitCommand},
	{"getversion", "R",       "Y",         &FunctionInterface::getversionCommand},
	{"getenvironment", "S,S,S",  "Y,N,N",  &FunctionInterface::getenvCommand},
	{"date",       "r,r,r",   "Y,Y,Y",     &FunctionInterface::dateCommand},
	{"time",       "r,r,r",   "Y,Y,Y",     &FunctionInterface::dateCommand},
	{"loadscript", "s,I",     "Y,N",       &FunctionInterface::loadscriptCommand},
	{"atoi",       "I,s",     "Y,Y",       &FunctionInterface::atoiCommand},
	{"_chkval_",   "R,r,s",   "Y,Y,Y",     &FunctionInterface::chkValueCommand},

	{"pretextgosub", "L",     "Y",         &FunctionInterface::usergosubCommand},
	{"textgosub",    "L",     "Y",         &FunctionInterface::usergosubCommand},
	{"loadgosub",    "L",     "Y",         &FunctionInterface::usergosubCommand},

	//image module
	{"lsp",      "i,s,i,i,i",     "Y,Y,Y,Y,N",     &FunctionInterface::lspCommand},
	{"lsph",     "i,s,i,i,i",     "Y,Y,Y,Y,N",     &FunctionInterface::lspCommand},
	{"lspc",     "i,i,s,i,i,i",   "Y,Y,Y,Y,Y,N",   &FunctionInterface::lspCommand},
	{"lsp2",     "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsp2add",  "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsp2sub",  "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsph2",    "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsph2add", "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"lsph2sub", "i,s,i,i,i,i,i,i", "Y,Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::lsp2Command},
	{"print",    "i,i,s",         "Y,N,N",         &FunctionInterface::printCommand},
	{"bg",       "Ns,i,i,s",     "Y,N,N,N",        &FunctionInterface::bgCommand},
	{"csp",      "i",            "Y",              &FunctionInterface::cspCommand},
	{"csp2",     "i",            "Y",              &FunctionInterface::cspCommand},
	{"msp",      "i,i,i,i",      "Y,Y,Y,N",        &FunctionInterface::mspCommand},
	{"msp2",     "i,i,i,i,i,i,i","Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::mspCommand},
	{"amsp",     "i,i,i,i",      "Y,Y,Y,N",        &FunctionInterface::mspCommand},
	{"amsp2",    "i,i,i,i,i,i,i","Y,Y,Y,Y,Y,Y,N",  &FunctionInterface::mspCommand},
	{"cell",     "i,i",          "Y,Y",            &FunctionInterface::cellCommand},
	{"humanz",   "i",            "Y",              &FunctionInterface::humanzCommand},
	{"strsp",    "i,s,i,i,i,i,i,i,i,i,i,i,N,#", "Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,N,#", &FunctionInterface::strspCommand},
	{"strsph",   "i,s,i,i,i,i,i,i,i,i,i,i,N,#", "Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,N,#", &FunctionInterface::strspCommand},
	{"transmode", "Ns",          "Y",              &FunctionInterface::transmodeCommand},
	{"bgcopy",    "",            "",               &FunctionInterface::bgcopyCommand},
	{"spfont",    "Ns,i,i,i,i,i,i", "Y,N,N,N,N,N,N", &FunctionInterface::spfontCommand},
	{"effectskip","i",           "Y",              &FunctionInterface::effectskipCommand},
	{"getspmode", "I,i",         "Y,Y",            &FunctionInterface::getspmodeCommand},
	{"getspsize", "i,I,I,I",     "Y,Y,Y,N",        &FunctionInterface::getspsizeCommand},
	{"getsppos",  "i,I,I",       "Y,Y,Y",          &FunctionInterface::getspposCommand },
	{"getspalpha","i,I",         "Y,Y",            &FunctionInterface::getspalphaCommand },
	{"getspposex","i,I,I,I,I,I", "Y,Y,Y,N,N,N",    &FunctionInterface::getspposexCommand },
	{"getspposex2","i,I,I,I,I,I", "Y,Y,Y,N,N,N",   &FunctionInterface::getspposexCommand },
	{"vsp",       "i,i",         "Y,Y",            &FunctionInterface::vspCommand},
	{"vsp2",      "i,i",         "Y,Y",            &FunctionInterface::vspCommand},
	{"allsphide", "\0",          "\0",             &FunctionInterface::allspCommand},
	{"allspresume",    "\0",        "\0",          &FunctionInterface::allspCommand},
	{"allsp2hide",     "\0",        "\0",          &FunctionInterface::allspCommand},
	{"allsp2resume",   "\0",        "\0",          &FunctionInterface::allspCommand},
	{"windowback",     "\0",        "\0",          &FunctionInterface::windowbackCommand},
	{ "effect",        "i,i,i,s",   "Y,Y,N,N",     &FunctionInterface::effectCommand },
	{ "windoweffect",  "i,i,s",     "Y,N,N",       &FunctionInterface::windoweffectCommand },
	{ "btnwait",       "I",         "Y",           &FunctionInterface::btnwaitCommand },
	{ "spbtn",         "i,i",       "Y,Y",         &FunctionInterface::spbtnCommand },
	{ "texec",         "\0",        "\0",          &FunctionInterface::texecCommand },
	{ "texthide",      "\0",        "\0",          &FunctionInterface::texecCommand },
	{ "texton",        "\0",        "\0",          &FunctionInterface::textonCommand },
	{ "textbtnoff",    "\0",        "\0",          &FunctionInterface::textbtnsetCommand },
	{ "textclear",     "\0",        "\0",          &FunctionInterface::textbtnsetCommand },
	{ "textbtnstart",  "i",         "Y",           &FunctionInterface::textbtnsetCommand },
	{ "textbtnwait",   "I",         "Y",           &FunctionInterface::btnwaitCommand },
	{ "setwindow",     "i,i,i,i,i,i,i,i,i,i,i,Ns,i,i,i,i","Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,Y,N,N", &FunctionInterface::setwindowCommand },
	{ "setwindow2",    "Ns",        "Y",           &FunctionInterface::setwindow2Command },
	{ "gettag",        "R,R,#",     "Y,N,#",       &FunctionInterface::gettagCommand },
	{ "gettext",       "S",         "Y",           &FunctionInterface::gettextCommand },
	{ "click",         "\0",        "\0",          &FunctionInterface::clickCommand },
	{ "lrclick",       "\0",        "\0",          &FunctionInterface::clickCommand },
	{ "erasetextwindow","i",        "Y",           &FunctionInterface::erasetextwindowCommand },
	{ "getmousepos",   "I,I",       "Y,Y",         &FunctionInterface::getmouseposCommand },
	{ "btndef",        "Ns",        "Y",           &FunctionInterface::btndefCommand },
	{ "btntime",       "i",         "Y",           &FunctionInterface::btntimeCommand },
	{ "btntime2",      "i",         "Y",           &FunctionInterface::btntimeCommand },
	{ "getpixcolor",   "S,i,i",     "Y,Y,Y",       &FunctionInterface::getpixcolorCommand },
	{ "_chkcolor_",    "i,i,i,i,S", "Y,Y,Y,Y,Y",   &FunctionInterface::chkcolorCommand },
	{ "getscreenshot", "i,i",       "Y,Y",         &FunctionInterface::getscreenshotCommand },

	//audio
	{ "bgm",           "s",         "Y",           &FunctionInterface::bgmCommand },
	{ "play",          "s",         "Y",           &FunctionInterface::bgmCommand },
	{ "mp3loop",       "s",         "Y",           &FunctionInterface::bgmCommand },
	{ "bgmonce",       "s",         "Y",           &FunctionInterface::bgmonceCommand },
	{ "mp3",           "s",         "Y",           &FunctionInterface::bgmonceCommand },
	{ "mp3save",       "s",         "Y",           &FunctionInterface::bgmonceCommand },
	{ "playonce",      "s",         "Y",           &FunctionInterface::bgmonceCommand },
	{ "bgmstop",       "\0",        "Y",           &FunctionInterface::bgmstopCommand },
	{ "mp3stop",       "\0",        "Y",           &FunctionInterface::bgmstopCommand },
	{ "playstop",      "\0",        "Y",           &FunctionInterface::bgmstopCommand },
	{ "loopbgm",       "s,s",       "Y,Y",         &FunctionInterface::loopbgmCommand },
	{ "loopbgmstop",   "\0",        "",            &FunctionInterface::loopbgmstopCommand },
	{ "bgmfadein",     "i",         "Y",           &FunctionInterface::bgmfadeCommand },
	{ "mp3fadein",     "i",         "Y",           &FunctionInterface::bgmfadeCommand },
	{ "bgmfadeout",    "i",         "Y",           &FunctionInterface::bgmfadeCommand },
	{ "mp3fadeout",    "i",         "Y",           &FunctionInterface::bgmfadeCommand },
	{ "wave",          "s",         "Y",           &FunctionInterface::waveCommand },
	{ "wavestop",      "\0",        "Y",           &FunctionInterface::waveCommand },
	{ "waveloop",      "s",         "Y",           &FunctionInterface::waveCommand },
	{ "bgmdownmode",   "i",         "Y",           &FunctionInterface::bgmdownmodeCommand },
	{ "bgmvol",        "i",         "Y",           &FunctionInterface::voicevolCommand },
	{ "mp3vol",        "i",         "Y",           &FunctionInterface::voicevolCommand },
	{ "sevol",         "i",         "Y",           &FunctionInterface::voicevolCommand },
	{ "voicevol",      "i",         "Y",           &FunctionInterface::voicevolCommand },
	{ "chvol",         "i,i",       "Y,Y",         &FunctionInterface::chvolCommand },
	{ "dwave",         "i,s",       "Y,Y",         &FunctionInterface::dwaveCommand },
	{ "dwaveload",     "i,s",       "Y,Y",         &FunctionInterface::dwaveloadCommand },
	{ "dwaveloop",     "i,s",       "Y,Y",         &FunctionInterface::dwaveCommand },
	{ "dwaveplay",     "i",         "Y",           &FunctionInterface::dwaveplayCommand },
	{ "dwavestop",     "\0",        "Y",           &FunctionInterface::dwavestopCommand },
	{ "getbgmvol",     "I",         "Y",           &FunctionInterface::getvoicevolCommand },
	{ "getmp3vol",     "I",         "Y",           &FunctionInterface::getvoicevolCommand },
	{ "getsevol",      "I",         "Y",           &FunctionInterface::getvoicevolCommand },
	{ "getvoicevol",   "I",         "Y",           &FunctionInterface::getvoicevolCommand },
	{ "stop",          "\0",        "",            &FunctionInterface::stopCommand },

	//file
	{ "nsadir",        "s",         "Y",           &FunctionInterface::nsadirCommand },
	{ "addnsadir",     "s",         "Y",           &FunctionInterface::nsadirCommand },
	{ "nsa",           "\0",        "",            &FunctionInterface::nsaCommand },
	{ "ns2",           "\0",        "",            &FunctionInterface::nsaCommand },
	{ "ns3",           "\0",        "",            &FunctionInterface::nsaCommand },

	{NULL, NULL, NULL, NULL},
};

FunctionInterface::FunctionInterface() {
	RegisterBaseFunc();
}

FunctionInterface::~FunctionInterface() {
}

void FunctionInterface::RegisterBaseFunc() {
	if (baseFuncMap.size() > 0) return;
	FuncLUT *ptr = &func_lut[0];
	while (ptr->cmd) {
		//printf(ptr->cmd);
		baseFuncMap[LOString(ptr->cmd)] = ptr;
		ptr++;
	}
}

FunctionInterface::FuncLUT* FunctionInterface::GetFunction(LOString &func) {
	auto iter = baseFuncMap.find(func);
	if (iter == baseFuncMap.end()) return NULL;
	else return iter->second;
}

int FunctionInterface::RunCommand(FuncBase it) {
	int ret = (this->*it)(this);
	if (ret == RET_VIRTUAL && imgeModule) ret = (imgeModule->*it)(this);
	if (ret == RET_VIRTUAL && audioModule) ret = (audioModule->*it)(this);
	if (ret == RET_VIRTUAL && fileModule) ret = (fileModule->*it)(this);
	return ret;
}


LOScriptPoint  *FunctionInterface::GetParamLable(int index) {
	return NULL;
}

LOString FunctionInterface::GetParamStr(int index) {
	ONSVariableRef *v = GetParamRef(index);
	if (v && v->GetStr()) return *(v->GetStr());
	else return LOString();
}


int FunctionInterface::GetParamInt(int index) {
	ONSVariableRef *v = GetParamRef(index);
	if (v) return (int)v->GetReal();
	else return -1;
}

int FunctionInterface::GetParamColor(int index) {
	ONSVariableRef *v = GetParamRef(index);
	if (v) {
		LOString *s = v->GetStr();
		if (s) {
			char *buf = (char*)(intptr_t)(s->c_str());
			if (buf[0] == '#') buf++;
			int sum = 0;
			while (buf[0]) {
				char hh = tolower(*buf);
				if (hh >= 'a' && hh <= 'f') sum = sum * 16 + hh - 87;
				else if (hh >= '0' && hh <= '9') sum = sum * 16 + hh - '0';
				else return sum;
				buf++;
			}
			return sum;
		}
	}
	return 0;
}

int FunctionInterface::GetParamCount() {
	return ((intptr_t)paramStack.top()) & 0xff;
}

//获取指定位置的ref变量，失败返回NULL
ONSVariableRef *FunctionInterface::GetParamRef(int index) {
	int max = GetParamCount();
	if (index > max) {
		SimpleError("GetParamRef() out of range! max is %d, index is %d.", max, index);
		return NULL;
	}
	return paramStack.at(paramStack.size() - 1 - max + index);
}


LOString FunctionInterface::StringFormat(int max, const char *format, ...) {
	va_list ap;
	va_start(ap, format);

	char *tmp = new char[max];
	memset(tmp, 0, max);
	int len = vsnprintf(tmp, max - 1, format, ap);
	tmp[len] = 0;
	LOString rets(tmp);
	delete[] tmp;
	va_end(ap);
	return rets;
}

void FunctionInterface::AddWorkDir(LOString dir) {
	workDirs.push_back(dir);
}

FILE* FunctionInterface::OpenFile(const char *name, const char *rb) {
	FILE *f = fopen(name, rb);
	if (!f && name[0] != '/' && name[1] != ':') {  //不是一个绝对路径，尝试搜索所有的路径
		for (int ii = 0; ii < workDirs.size() && !f; ii++) {
			LOString dir = workDirs.at(ii);
			dir.append("/");
			dir.append(name);
			f = fopen(dir.c_str(), rb);
		}
	}
	return f;
}

LOString FunctionInterface::GetFilePath(const char *name) {
	LOString dir;
	FILE *f = fopen(name, "rb");
	if (!f && name[0] != '/' && name[1] != ':') {  //不是一个绝对路径，尝试搜索所有的路径
		for (int ii = 0; ii < workDirs.size() && !f; ii++) {
			dir = workDirs.at(ii);
			dir.append("/");
			dir.append(name);
			f = fopen(dir.c_str(), "rb");
		}
	}
	else dir.assign(name);
	if (f) fclose(f);
	return dir;
}

char* FunctionInterface::ReadFileOnce(const char *name, int *len) {
	FILE *f = OpenFile(name, "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		*len = ftell(f);
		char *buf = new char[*len];
		memset(buf, 0, *len);
		fseek(f, 0, SEEK_SET);
		fread(buf, 1, *len, f);
		fclose(f);
		return buf;
	}
	else {
		*len = 0;
		return NULL;
	}
}

//应由文件系统实现
BinArray *FunctionInterface::ReadFile(const char *fileName, bool err) {
	if (fileModule) {
		LOString s(fileName);
		return fileModule->ReadFile(&s, err);
	}
	else return NULL;
}

//应由文件系统实现
BinArray *FunctionInterface::ReadFile(LOString *fileName, bool err) {
	if (fileModule) {
		return fileModule->ReadFile(fileName, err);
	}
	else return NULL;
}



LOString FunctionInterface::RandomStr(int count) {
	char ch;
	LOString s;
	for (int ii = 0; ii < count; ii++) {
		ch = rand() % ('Z' - 'A' + 1) + 'A';
		s.append(&ch, 1);
	}
	return s;
}