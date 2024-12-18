#include "LOScriptReader.h"
#include "Buil_in_script.h"
#include <stdarg.h>
//#include <SDL.h>
//#include "LOMessage.h"
#if defined(_MSC_VER)
#include <Windows.h>
#endif

//=========== static ========================== //
LOStack<LOScriptReader> LOScriptReader::readerList;  //脚本解析分组
LOStack<LOScriptReader> LOScriptReader::workingList;    //处于活动状态的脚本解析分组
LOStack<LOScripFile> LOScriptReader::filesList;
std::unordered_map<std::string, int> LOScriptReader::numAliasMap;   //整数别名
std::unordered_map<std::string, int> LOScriptReader::strAliasMap;   //字符串别名，存的是字符串位置
std::vector<LOString> LOScriptReader::strAliasList;  //字符串别名
std::vector<LOString> LOScriptReader::workDirs;
std::unordered_map<std::string, LOScriptPoint*> LOScriptReader::defsubMap;
int LOScriptReader::pollCount = 0;
int LOScriptReader::sectionState ;
bool LOScriptReader::scripterhasExit = true;

char _errinfo[1024];

void SimpleError(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	memset(_errinfo, 0, 1024);
	vsnprintf(_errinfo, 1024 - 1, format, ap);
	va_end(ap);
	printf("%s", _errinfo);
#if defined(_MSC_VER)
	::OutputDebugString(_errinfo);
	::OutputDebugString("\n");
#endif
}

void SimpleInfo(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	memset(_errinfo, 0, 1024);
	vsnprintf(_errinfo, 1024 - 1, format, ap);
	va_end(ap);
	printf("%s",_errinfo);
}

LOScripFile* LOScriptReader::AddScript(char *buf, int length, const char* filename) {
	LOScripFile *file = new LOScripFile(buf, length, filename);
	filesList.push(file);
	return file;
}

void LOScriptReader::InitScriptLabels() {
	for (int ii = 0; ii < filesList.size(); ii++) {
		filesList.at(ii)->InitLables();
	}
}

 void LOScriptReader::AddWorkDir(LOString dir) {
	 workDirs.push_back(dir);
 }

 LOScriptReader* LOScriptReader::GetScriptReader(LOString &name) {
	 for (int ii = 0; ii < readerList.size(); ii++) {
		 if (readerList.at(ii)->Name == name) return readerList.at(ii);
	 }
	 LOScriptReader *reader = new LOScriptReader;
	 reader->Name = name;
	 readerList.push(reader);
	 return reader;
 }

 LOScriptReader* LOScriptReader::EnterScriptReader(LOString name) {
	 LOScriptReader *reader = GetScriptReader(name);
	 if (reader->isWorking) {
		 SimpleError("ScriptReader[ %s ] already running!", name.c_str());
		 return NULL;
	 }
	 else {
		 reader->isWorking = true;
		 workingList.push(reader);
		 return reader;
	 }
 }

 void LOScriptReader::LeaveScriptReader(LOScriptReader* reader) {
	 for (int ii = 0; ii < workingList.size(); ii++) {
		 if (workingList.at(ii) == reader) {
			 workingList.removeAt(ii);
			 break;
		 }
	 }
	 reader->isWorking = false;
 }



 // ================ class =========================== //

LOScriptReader::LOScriptReader()
{
	//RegisterBaseFunc();

	isMain = false;    //默认都不是主脚本
	isWorking = false;
	nslogic = new LogicPointer(LogicPointer::TYPE_IF);
	subStack = new LOStack<LOScriptPoint>;
	loopStack = new LOStack<LogicPointer>;
	//timerID = 0;
	lastCmdCheckFlag = 0;
	parseDeep = 0;
	printName = "_main";
	//threadID = 0;
	//ttimer = STEADY_CLOCK::now();
	ttimer = SDL_GetTicks();
}

LOScriptReader::~LOScriptReader()
{
	delete nslogic;
	if(subStack) delete subStack;
	if(loopStack) delete loopStack;
}

int LOScriptReader::MainTreadRunning() {
	if (!isMain) {
		SimpleError("LOScriptReader::MainTreadRunning() must run at main scriptReader!");
		return -1;
	}

	scripterhasExit = false;
	LOString lableName;
	int ret = -1;

	for (int ii = 0; ii < 2 && ret != RET_ERROR; ii++) {
		if (ii == 0) {
			lableName = "define";
			sectionState = SECTION_DEFINE;
		}
		else {
			EnterScriptReader(MAINSCRIPTER_NAME);
			lableName = "start";
			sectionState = SECTION_NORMAL;
		}

		LOScriptPoint *p = GetScriptPoint(lableName);
		if (!p) break;
		RunScript(p);
		ret = MainStartParse();
	}
	scripterhasExit = true;
	SimpleInfo("%s has finished running!\n", MAINSCRIPTER_NAME);

	if (ret == RET_ERROR) ret = -1;
	else return 0;
} 

bool LOScriptReader::WaitExit(int overtime) {
	STEADY_CLOCK::time_point t1 = STEADY_CLOCK::now();
	int postime = 0;
	while (postime < overtime && !scripterhasExit) {
		std::this_thread::sleep_for(std::chrono::microseconds(500));
		auto it = std::chrono::duration_cast<std::chrono::milliseconds>(STEADY_CLOCK::now() - t1);
		postime = it.count() & 0xfffff;
	}
	if (postime < overtime) return true;
	else return false;
}


//void LOScriptReader::SetError(const char *fmt, ...) {
//	va_list ap;
//	va_start(ap, fmt);
//	char *errbuf = errinfo + errlen;
//	if (currentLable) {
//		LOString lines;
//		encoder->NextLine(currentLable->current_address, &lines);
//		errlen += snprintf(errinfo + errlen, 1024 - errlen - 1, "%s at line %d:%s\n", GetCurrentFile().c_str(),
//			currentLable->current_line, lines.c_str());
//	}
//	errlen += vsnprintf(errinfo + errlen, 1024 - errlen - 1, fmt, ap);
//	va_end(ap);
//	LOG_PRINT(errbuf);
//}



//开始一个新的运行点，当前运行点的逻辑栈、子函数栈被推入堆栈中，执行完成后返回
int LOScriptReader::RunScript(LOScriptPoint *lable) {
	isAddLine = true;
	//开始执行
	ReadyToRun(lable);
	return 0;
}


//是否运行到RunScript应该返回的位置
bool LOScriptReader::isEndSub() {
	return subStack->size() == 0;
	//uintptr_t ptr = (uintptr_t)subStack->top();
	//if (ptr < 0x8) return true;
	//else return false;
}

//准备转到下一个运行点
void LOScriptReader::ReadyToRun(LOScriptPoint *label, int callby) {
	if (callby != LOScriptPoint::CALL_BY_NORMAL) subStack->push((LOScriptPoint*)callby);
	subStack->push(label);
	scriptbuf = label->file->GetBuf();
	currentLable = subStack->top();
	currentLable->c_buf = currentLable->s_buf;
	currentLable->c_line = currentLable->s_line;
}

void LOScriptReader::ReadyToRun(LOString *lname, int callby) {
	if (lname->length() == 0) return;
	LOScriptPoint *p = GetScriptPoint(*lname);
	if (!p) {
		SimpleError("not lable name:[%s]", lname->c_str());
		return;
	}
	ReadyToRun(p, callby);
}

//准备返回上一个运行点，同时抛弃本个运行点
int LOScriptReader::ReadyToBack() {
	uintptr_t callby = 0;
	LOScriptPoint *last = subStack->pop();
	currentLable = NULL;
	if (!isEndSub()) {
		callby = (uintptr_t)subStack->top();
		if (callby < 0x10) subStack->pop();
		else callby = LOScriptPoint::CALL_BY_NORMAL;
		currentLable = subStack->top();
		scriptbuf = currentLable->file->GetBuf();

		if (callby == LOScriptPoint::CALL_BY_EVAL) {//删除eval的点
			assert("not do it!");
		}
	}

	int ret = RET_RETURN | ((callby & 0xff) << 8);
	return ret;
}

LOScriptPoint* LOScriptReader::GetScriptPoint(LOString lname) {
	lname = lname.toLower();
	LOScriptPoint *p = NULL;
	for (int ii = 0; ii < filesList.size(); ii++) {
		p = filesList.at(ii)->FindLable(lname);
		if (p) return p;
	}
	SimpleError("no lable:%s", lname.c_str());
	return p;
}

LOScriptPoint  *LOScriptReader::GetParamLable(int index) {
	LOString s = GetParamStr(index);
	if (s.length() == 0) return NULL;
	return GetScriptPoint(s);
}


//转移运行点位置，0表示转到top，1表示top的前一个
bool LOScriptReader::ChangePointer(int esp) {
	//寻找真正的位置
	int index = subStack->size() - 1;
	int available = 0;
	while (index >= 0 && available < esp) {
		if (((intptr_t)subStack->at(index)) > 0x10) available++;
		index--;
	}
	if (index < 0) return false;

	currentLable = subStack->at(index);
	scriptbuf = currentLable->file->GetBuf();
	return true;
}

LOScriptReader* LOScriptReader::GetPollReader() {
	pollCount = pollCount % (readerList.size());
	LOScriptReader *reader = readerList.at(pollCount);
	pollCount++;
	return reader;
}


void LOScriptReader::ClearParams(bool isall) {
	if (isall) {
		while (paramStack.size() > 0) {
			ONSVariableRef *v = paramStack.pop();
			if ((intptr_t)v > 0x10) delete v;
		}
	}
	else {
		if (paramStack.size() > 0) {
			intptr_t paramcount = (intptr_t)paramStack.pop();
			for (int ii = 0; ii < paramcount && paramStack.size() > 0; ii++) delete paramStack.pop();
		}
	}
}


//根据参数将变量推入paramStack中，完成后推入一个NULL,出错返回假
bool LOScriptReader::PushParams(const char *param, const char* used) {
	if (param[0] == 0) {
		paramStack.push(NULL);
		return true;
	}

	//if (currentLable->c_line == 603) {
	//	int debugbreak = 0;
	//}

	bool hasnormal, haslabel, hasvariable;
	int allow_type, paramcount = 0;
	const char *fromparam = param;
	const char *fromuse = used;
	const char *lastparam, *lastused;

	while (param[0] != 0) {
		if (paramcount == 0) {
			if (used[0] == 'N' && !NextSomething()) break;
			else if (used[0] == 'Y' && !NextSomething()) {
				SimpleError("function paragram %d type error!", paramcount + 1);
				return false;
			}
		}
		
		lastparam = param; lastused = used;
		hasvariable = hasnormal = haslabel = false;
		allow_type = ONSVariableRef::GetTypeAllow(param);

		for (int ii = 0; param[ii] != ',' && param[ii] != 0; ii++) {
			if (param[ii] == 'N') hasnormal = true;
			else if (param[ii] == 'L') haslabel = true;
			else hasvariable = true;
		}
		//get it
		ONSVariableRef *v = nullptr;
		if (hasnormal) 
			v = TryNextNormalWord();
		if (!v && haslabel) v = ParseLabel2();
		if (!v && hasvariable) {
			v = ParseVariableBase();
			if (!v ||  !(v->vtype & allow_type)) {
				SimpleError("[%s] paragram %d type error!\n",curCmdbuf, paramcount + 1);
				ClearParams(true);
				return false;
			}
		}
		//push it
		if (v) {
			paramcount++;
			paramStack.push(v);
		}
		else {
			SimpleError("function paragram %d type error!", paramcount + 1);
			ClearParams(true);
			return false;
		}
		//check next
		while (param[0] != ',' && param[0] != 0) param++;
		if (param[0] == ',') param++;
		while (used[0] != ',' && used[0] != 0) used++;
		if (used[0] == ',') used++;

		if (used[0] == 0) {
			paramStack.push((ONSVariableRef*)paramcount);
			return true;
		}
		else if (used[0] == 'Y') {
			NextComma();
		}
		else if (used[0] == 'N') {
			if (!NextComma(true)) {
				paramStack.push((ONSVariableRef*)paramcount);
				return true;
			}
			used++;
		}
		else if (used[0] == '*') {
			if (!NextComma(true)) {
				paramStack.push((ONSVariableRef*)paramcount);
				return true;
			}
			param = fromparam;
			used = fromuse;
		}
		else if (used[0] == '#') {
			if (!NextComma(true)) {
				paramStack.push((ONSVariableRef*)paramcount);
				return true;
			}
			param = lastparam;
			used = lastused;
		}
	}
	return true;
}

//根据特殊的返回返回值确定操作
int LOScriptReader::ReturnEvent(int ret, const char *&buf, int &line) {
	int call_by = (ret >> 8) & 0xff;
	ret = ret & 0xff;
	if (ret == RET_WARNING) buf = TryToNextCommand(buf);
	else if (ret == RET_ERROR) {
		if (!isEndSub()) ReadyToBack();
	}
	else if (ret == RET_RETURN) {
		if (isEndSub()) return ret;
		if (call_by) {
			if (call_by == LOScriptPoint::CALL_BY_PRETEXT_GOSUB) {
				line += TextPushParams(buf);
				ret = imgeModule->textCommand(this);
				ClearParams(false);
				ReadyToRun(&userGoSubName[USERGOSUB_TEXT], LOScriptPoint::CALL_BY_TEXT_GOSUB);
			}
		}
	}
	return ret;
}


int LOScriptReader::ContinueRun() {
	LOString cmd;
	int ret;
	int callby = 0;

	ret = ContinueEvent();
	if (ret != RET_VIRTUAL) return ret;
	ret = RET_CONTINUE;

	switch (IdentifyLine(currentLable->c_buf))
	{
	case LINE_NOTES:case LINE_LABEL:case LINE_END:
		currentLable->c_buf = scriptbuf->NextLine(currentLable->c_buf);
		if (isAddLine) currentLable->c_line++;
		break;
	case LINE_CONNECT:
		currentLable->c_buf++;
		break;
	case LINE_TEXT:
		//这个过程看起来有点奇怪，显示文本前我们必须进入 pretextgosub ，等到从pretextgosub返回时再进入到文字显示
		//文字显示后进入 textgosub，预先准备好tagstring
		TagString = scriptbuf->GetTagString(currentLable->c_buf);
		ReadyToRun(&userGoSubName[USERGOSUB_PRETEXT], LOScriptPoint::CALL_BY_PRETEXT_GOSUB);
		return RET_CONTINUE;
	case LINE_CAMMAND:
		ret = RunCommand(currentLable->c_buf);
		ret = ReturnEvent(ret, currentLable->c_buf, currentLable->c_line);
		if (isEndSub()) return ret;
		break;
	case LINE_JUMP:
		currentLable->c_buf++;
		break;
	case LINE_EXPRESSION:
		//if (expdeep == 0) { //$表达式最多只展开一层
		//	LOString s = ParseStrVariable();
		//	if (s.length() > 0) {

		//	}
		//}
		//else buf++;
		break;
	default:
		currentLable->c_buf = scriptbuf->NextLine(currentLable->c_buf);
		//SDL_LogError(0, "unknow line type at line:%d,buf[0]:%s", currentLable->current_line, cmd.c_str());
		if (isAddLine) currentLable->c_line++;
		break;
	}
	return ret;
}


//因为采用了轮询来模拟多线程脚本，所以部分需等待的命令是通过事件存储的
int LOScriptReader::ContinueEvent() {
	LOEvent1 *e = blocksEvent.GetHeaderEvent();
	//LOEventParamBtnRef *param;
	bool isfinish = false;
	bool issleep = true;

	if (e) {
		switch (e->eventID){
		case SCRIPTER_EVENT_DALAY:
			//如果事件已经在left click完成，那么计算超时不会影响结果
			if (DelayTimeCheck(e)) {
				//延时事件没有后续操作，事件可以直接失效
				e->InvalidMe(); isfinish = true;
			}
			break;
		case LOEvent1::EVENT_CATCH_BTN:  //按钮捕获
			//按钮是否超时
			if (!e->isFinish() && DelayTimeCheck(e)) {
				e->value = -2; e->FinishMe();
			}
			if (e->isFinish()) { //按钮事件完成，赋值，同时线程阻塞失效
				BtnCatchFinish(e);
				e->InvalidMe();
			}
			break;
		case SCRIPTER_EVENT_LEFTCLICK:case SCRIPTER_EVENT_CLICK:
			e->InvalidMe(); isfinish = true;
			break;
		case SCRIPTER_BGMLOOP_NEXT:  
			FunctionInterface::audioModule->PlayAfter();  //bgmloop的时候，播放下一首
			e->InvalidMe();
			break;
		case LOEvent1::EVENT_TEXT_ACTION:
			if (e->isFinish()) {
				if(e->value & 0xff == '/' ) ReadyToRun(&userGoSubName[USERGOSUB_TEXT], LOScriptPoint::CALL_BY_TEXT_GOSUB); 
				e->InvalidMe();
			}
			break;
		default:
			break;
		}

		//事件处理结果
		if (isfinish) return RET_CONTINUE;     //事件完成，进入下一个轮询（阻塞当前脚本）
		else {  //事件没有得到处理，脚本继续等待
			//单线程脚本时防止CPU占用率过高.
			if (workingList.size() == 1 && issleep) SDL_Delay(1);
			return RET_CONTINUE;    //事件没有完成，进入下一个轮询（阻塞当前脚本）
		}
	}

	return RET_VIRTUAL;   //没有阻塞事件，进入下一个命令
}

//检查延时是否完成
bool LOScriptReader::DelayTimeCheck(LOEvent1 *e) {
	auto *param = (LOEventParamBase*)e->param;
	Uint32 tnow = SDL_GetTicks();
	Uint32 told = param->GetParam(0) & 0xffffffff;
	Uint32 tdst = param->GetParam(1) & 0xffffffff;
	if (tnow - told > tdst - 2 ) return true;
	//else if (tnow - told > tdst - 10) { //延时差距10毫秒内，直接阻塞完成
	//	printf("enter 10ms wait.\n");
	//	tnow = SDL_GetTicks();
	//	do {
	//		G_PrecisionDelay(0.5);
	//		//SDL_Delay(1);
	//	} while (SDL_GetTicks() - told < tdst - 1);
	//	return true;
	//}
	return false;
}

void LOScriptReader::BtnCatchFinish(LOEvent1 *e) {
	auto *param = (LOEventParamBtnRef*)e->param;
	int val = (int)e->value;
	param->ref->SetValue((double)val);
}


//void LOScriptReader::RunInserCommand(LOEvent *e) {
	//LOString *text = (LOString*)e->param;
	//text->append("\n return");

	//char *buf = new char[text->length() + 1];
	//memset(buf, 0, text->length() + 1);
	//memcpy(buf, text->c_str(), text->length());

	//LOScriptPoint *p = new LOScriptPoint(LOScriptPoint::CALL_BY_NORMAL,true) ;
	//p->start_address = 0;
	//p->start_line = -9999;
	//p->file = 0;
	//p->name = RandomStr(8);
	//p->current_address = p->start_address;
	//p->current_line = p->start_line;

	//ReadyToRun(p, LOScriptPoint::CALL_BY_EVAL);

	//delete text;
	//e->param = NULL;
//}

//$表达式最多只展开一层
int LOScriptReader::MainStartParse() {
	int ret = RET_RETURN;
	while (workingList.size() > 0) {
		pollCount = pollCount % workingList.size();
		activeReader = workingList.at(pollCount);
		pollCount++;

		ret = activeReader->ContinueRun();

		if (ret == RET_ERROR) break;
		else if (ret == RET_RETURN) {
			if (activeReader->isEndSub()) LeaveScriptReader(activeReader);
		}

		if (isExit) workingList.clear();
	}
	return ret;
}

//识别出从指定位置的语句类型
int LOScriptReader::IdentifyLine(const char *&buf) {
	buf = scriptbuf->SkipSpace(buf);
	char ch = buf[0];
	if (ch == ';') return LINE_NOTES;
	if (ch == '*')return LINE_LABEL;
	if (ch == ':')return LINE_CONNECT;
	if (ch == '`')return LINE_TEXT;
	if (ch == '\n')return LINE_END;
	if (ch == '[')return LINE_TEXT;
	if (ch == '$')
		return LINE_TEXT;
	if (ch == '~') return LINE_JUMP;
	if(ch == '\\' || ch == '@' || ch == '/') return LINE_TEXT;
	int type = scriptbuf->GetCharacter(buf);
	if (type == LOCodePage::CHARACTER_LETTER) return LINE_CAMMAND;
	else if (type == LOCodePage::CHARACTER_MULBYTE) return LINE_TEXT;
	return LINE_UNKNOW;
}

int LOScriptReader::RunCommand(const char *&buf) {
	//if (currentLable->current_line == 67) {
	//	int deebgu = 0;
	//}

	LOString tmpcmd = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER).toLower();
	if (tmpcmd.length() > 0) {
		//优先尝试用户定义的函数，这样可以实现一些特殊自定义的系统
		auto useit = defsubMap.find(tmpcmd);
		if (useit != defsubMap.end()) {
			curCmd = tmpcmd;
			curCmdbuf = curCmd.c_str();

			if (!useit->second) {
				SimpleError("no label for this function:%s!\n", curCmdbuf);
				return RET_ERROR;
			}
			buf = scriptbuf->SkipSpace(buf);
			gosubCore(useit->second, false);
			return RET_CONTINUE;
		}
		//

		FuncLUT *ptr = GetFunction(tmpcmd);
		if (ptr) {
			lastCmd = curCmd;
			curCmd = tmpcmd;
			curCmdbuf = curCmd.c_str();
			buf = scriptbuf->SkipSpace(buf);
			LOG_PRINT("[%d]:[%s]\n", currentLable->c_line, tmpcmd.c_str());
			//if (currentLable->current_line == 54) {
			//	int debugbreak = 1;
			//}
			if (!PushParams(ptr->param, ptr->used))return RET_ERROR;
			int ret = FunctionInterface::RunCommand(ptr->method);
			ClearParams(false);
			return ret;
		}
		else {
			LOG_PRINT("[%s]:[%d]:[%s] is not supported yet!!\n",
				GetCurrentFile().c_str(), currentLable->c_line, tmpcmd.c_str());
			return RET_WARNING;
		}

		return RET_ERROR;
	}
}

LOString LOScriptReader::GetCurrentFile() {
	if (currentLable && currentLable->file) {
		return currentLable->file->Name;
	}
	return LOString();
}


//获取下一个关键词，参数用于确定是否往下移动
void LOScriptReader::NextWord(bool movebuf) {
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	word = scriptbuf->GetWord(buf);
	if (movebuf) currentLable->c_buf = buf;
}


void LOScriptReader::NextNormalWord(bool movebuf) {
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if(scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_LETTER) {
		word = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
	}
	if (movebuf) currentLable->c_buf = buf;
}


bool LOScriptReader::NextSomething() {
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == ':' || buf[0] == '\n' || buf[0] == 0) return false;
	return true;
}

//非尝试模式时会报错
bool LOScriptReader::NextComma(bool isTry) {
	bool spaceisComma = false;
	if (scriptbuf->GetCharacter(currentLable->c_buf) == LOCodePage::CHARACTER_SPACE)spaceisComma = true;
	const char* buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == ',') {
		currentLable->c_buf = buf + 1;
		return true;
	}
	else if (!isTry) {
		//有一些麻烦的空格即为分隔的写法，真不想支持
		if (spaceisComma) return true;
		SimpleError("NextComma not at a comma!");
		return false;
	}
	else {
		currentLable->c_buf = buf;
		return false;
	}
}

bool LOScriptReader::isName(const char* name) {
	return (curCmd.compare(name) == 0);
}


ONSVariableRef* LOScriptReader::ParseIntExpression(const char *&buf, bool isalias) {
	LOStack<ONSVariableRef> s2;
	buf = GetRPNstack(&s2, buf, true);
	//auto ss = TransformStack(&s2);
	CalculatRPNstack(&s2);
	return *(s2.begin());
}


ONSVariableRef *LOScriptReader::ParseVariableBase(ONSVariableRef *ref, bool str_alias) {
	//if (currentLable->c_line == 13) {
	//	int debugbreak = 111;
	//}

	const char *buf, *obuf;
	buf = obuf = scriptbuf->SkipSpace(currentLable->c_buf);
	int ret, vardeep = 1;
	bool isok = false;
	bool isstr_alias = false;
	
	//优先尝试%XX $XX立即数，速度最快
	char vart = buf[0];
	if (vart == '%' || vart == '$' || vart == '"' || vart == '-') buf++;
	char ch = buf[0];
	if (ch == '%' && (vart == '%' || vart == '$')) {  //最多搜索两层，更多的交给表达式处理
		buf++;
		vardeep++; 
	}
	if(vart == '"') buf = TryGetImmediateStr(ret, buf, isok);
	else buf = TryGetImmediate(ret, buf, true, isok);
	if (!isok && str_alias) buf = TryGetStrAlias(ret, buf, isstr_alias); //这里有个风险，同名的整数别名可能会覆盖掉文字别名
	if (isok || isstr_alias) {
		if(!ref) ref = new ONSVariableRef;
		if (vart == '-' && !isstr_alias) ret = 0 - ret;
		if (vardeep > 1) { //获取最里面一层的%值
			ref->SetRef(ONSVariableRef::TYPE_INT, ret);
			ret = ref->GetReal();
		}
		//包装结果
		if (vart == '"') {
			LOString s = scriptbuf->substr(obuf + 1, ret);
			ref->SetImVal(&s);
		}
		else if (isstr_alias)ref->SetImVal(&strAliasList[ret]);
		else if (vart == '$') ref->SetRef(ONSVariableRef::TYPE_STRING, ret);
		else if(vart == '%') ref->SetRef(ONSVariableRef::TYPE_INT, ret);
		else ref->SetImVal((double)ret);
		currentLable->c_buf = buf;
		return ref;
	}

	//if (GetCharacter(buf) != CHARACTER_NUMBER)return NULL; //不是数字开头，也不是别名

	//尝试表达式
	buf = obuf;
	ONSVariableRef *v = ParseIntExpression(buf, true);
	currentLable->c_buf = buf;
	if (!ref) return v;
	else {
		ref->CopyFrom(v);
		delete v;
		return ref;
	}
}

//只允许数值型
int LOScriptReader::ParseIntVariable() {
	ONSVariableRef *v = ParseVariableBase();
	if (!v) {
		SimpleError("Is not a valid expression");
		return 0;
	}
	if (!v->isReal()) SimpleError("You should use an integer value!");
	int ret = (int)v->GetReal();
	delete v;
	return ret;
}


LOString LOScriptReader::ParseStrVariable() {
	ONSVariableRef *v = ParseVariableBase(NULL,true);
	if (!v) {
		SimpleError("Is not a valid expression");
		return LOString();
	}
	if (!v->isStr()) SimpleError("You should use an string value!");
	LOString s;
	LOString *vs = v->GetStr();
	if (vs) s.assign(*vs);
	delete v;
 	return s;
}

//返回的字符串不包含'*'
LOString LOScriptReader::ParseLabel(bool istry) {
	if (NextStartFrom('*')) {
		while(CurrentOP() == '*')NextAdress();
		NextNormalWord(true);
		return LOString(word);
	}
	else if (NextStartFrom('$') || NextStartFrom('"')) {
		LOString tmp = ParseStrVariable();
		int ulen = 0;
		while ( tmp[ulen] == '*') ulen++;
		return LOString(tmp.c_str() + ulen);
	}
	else if(!istry){
		SimpleError("it not a label or a string value!");
	}
	return LOString();
}

ONSVariableRef *LOScriptReader::ParseLabel2() {
	LOString tmp = ParseLabel(true);
	if (tmp.length() > 0) {
		ONSVariableRef *v = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
		v->SetValue(&tmp);
		return v;
	}
	else return NULL;
}

//尝试取得立即数
const char* LOScriptReader::TryGetImmediate(int &val, const char *buf, bool isalias, bool &isok) {
	const char *obuf = buf;
	isok = false;

	buf = scriptbuf->SkipSpace(buf);
	int type = scriptbuf->GetCharacter(buf);
	if (type == LOCodePage::CHARACTER_NUMBER) {
		double vv = scriptbuf->GetReal(buf);
		buf = scriptbuf->SkipSpace(buf);
		if (scriptbuf->IsOperator(buf)) return obuf; //后面有计算符号，说明是复杂算式
		val = (int)vv;
		isok = true;
		return buf;
	}
	else if (buf[0] == '-') { //负数
		int ret;
		buf = TryGetImmediate(ret, buf + 1, false, isok);
		if (isok) {
			val = 0 - ret;
			return buf;
		}
	}
	else if (isalias && type == LOCodePage::CHARACTER_LETTER) {
		LOString s = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
		buf = scriptbuf->SkipSpace(buf);
		if (scriptbuf->IsOperator(buf) ) return obuf;  //后面有计算符号，说明是复杂算式
		val = GetAliasRef(s, false, isok);
		if (isok) return buf;
		else return obuf;
	}
	else return obuf;
	return obuf;
}

int LOScriptReader::GetAliasRef(LOString &s, bool isstr, bool &isok) {
	std::unordered_map<std::string, int> *tmap = &numAliasMap;
	if (isstr) tmap = &strAliasMap;

	auto iter = tmap->find(s);
	if (iter == tmap->end()) {
		isok = false;
		return -1;
	}
	else {
		isok = true;
		return iter->second;
	}
}

const char*  LOScriptReader::TryGetImmediateStr(int &len, const char *buf, bool &isok) {
	const char *obuf = buf;
	isok = false;
	len = scriptbuf->GetStringLen(buf);
	buf += len;
	if (buf[0] == '"') buf++;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == '+') return obuf;
	else {
		isok = true;
		return buf;
	}
}

const char* LOScriptReader::TryGetStrAlias(int &ret, const char *buf, bool &isok) {
	const char *obuf = buf;
	isok = false;
	if (strAliasMap.size() < 1) return obuf;
	if (scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_LETTER) {
		LOString s = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
		ret = GetAliasRef(s, true, isok);
		if (isok) return buf;
		else return obuf;
	}
	return obuf;
}

//获取逆波兰式的堆栈
const char* LOScriptReader::GetRPNstack(LOStack<ONSVariableRef> *s2, const char *buf, bool isalias) {
	//初始化符号栈
	LOStack<ONSVariableRef> s1;
	ONSVariableRef *v = new ONSVariableRef;
	LOString s;
	bool first = true;
	s1.push(v);
	//===================//
	int type;
	while (true) {
		buf = scriptbuf->SkipSpace(buf);
		type = scriptbuf->GetCharacter(buf);
		if (type == LOCodePage::CHARACTER_NUMBER || (buf[0] == '-' && first)) { //必须处理首个负数
			double tdd;
			if (buf[0] == '-') {
				buf++;
				tdd = 0 - scriptbuf->GetReal(buf);
			}
			else tdd = scriptbuf->GetReal(buf);
			v = new ONSVariableRef(ONSVariableRef::TYPE_REAL, tdd);
			s2->push(v);  //操作数立即入栈
			//检查符号栈，如果是% $则全部弹出这两个符号
			//while (s1.top()->oper == '%' || s1.top()->oper == '$') s2->push(s1.pop());

			//下一个符号不是运算符则停止
			buf = scriptbuf->SkipSpace(buf);
			if (!scriptbuf->IsOperator(buf)) break;
		}
		else if (type == LOString::CHARACTER_SYMBOL || scriptbuf->IsMod(buf)) { //因为有mod这种特殊的，所以不能只使用类型判断
			v = new ONSVariableRef;
			buf += v->GetOperator(buf);
			if (v->isOperator()) {
				//数组用的方括号不在这里处理
				if (v->oper == '(') {
					s1.push(v);
				}
				else if (v->oper == ')') PopRPNstackUtill(&s1, s2, '(');
				else if (v->oper == '[') {
					PopRPNstackUtill(&s1, s2, '?');
					s1.push(v);
				}
				else if (v->oper == ']') PopRPNstackUtill(&s1,s2,'[');
				else {
					//如果是数组，则压入一个特殊符号
					if (v->oper == '?') {
						s2->push(new ONSVariableRef(ONSVariableRef::TYPE_ARRAY_FLAG));
					}
					//
					auto iter = s1.end() - 1;
					if (v->order > (*iter)->order) s1.push(v);
					else if (v->order == (*iter)->order && v->order == ONSVariableRef::ORDER_GETVAR) s1.push(v);
					else {
						while (v->order <= (*iter)->order && (*iter)->oper != '(' &&
							(*iter)->oper != '[') {
							s2->push(*iter);
							s1.erase(iter--);
						}
						s1.push(v);
					}
				}
			}
			else if (buf[0] == '"') { //string
				v->vtype = ONSVariableRef::TYPE_STRING_IM;
				LOString s = scriptbuf->GetString(buf);
				v->SetValue(&s);
				s2->push(v);
				buf = scriptbuf->SkipSpace(buf);
				if (!scriptbuf->IsOperator(buf)) break;
			}
			else {
				delete v;
				break;
			}
		}
		else if (isalias && type == LOCodePage::CHARACTER_LETTER) { //允许别名则处理别名
			s = scriptbuf->GetWordStill(buf, LOCodePage::CHARACTER_LETTER | LOCodePage::CHARACTER_NUMBER);
			//别名作为立即数处理
			bool isok;
			v = new ONSVariableRef(ONSVariableRef::TYPE_REAL, GetAliasRef(s, false, isok));
			s2->push(v);
		}
		else {
			//正常语句结束
			char ch = buf[0];
			if (ch == ',' || ch == '\n' || ch == ':') break; 
			else SimpleInfo("line at [%d] may be have error.\n", currentLable->c_line);
			break;
		}

		first = false;
	}
	PopRPNstackUtill(&s1, s2, '\0');
	if (!s2->top() || !s2->top()->isOperator()) SimpleError("Expression error");
	return buf;
}


//计算逆波兰式
void LOScriptReader::CalculatRPNstack(LOStack<ONSVariableRef> *stack) {
	auto iter = stack->begin();
	int docount;  //操作数的个数
	ONSVariableRef *op,*v1 = nullptr,*v2 = nullptr;

	while (iter != stack->end()) {
		op = (*iter);
		if (op->isOperator()) {
			//运算符位于首个符号是不可能的
			if (iter == stack->begin()) SimpleError("Expression error!");
			//该符号需要的操作数
			docount = op->GetOpCount();
			//检查操作数的数量是否还符合要求
			if (stack->size() < docount + 1) {
				SimpleError("[ %c ] Insufficient operands required by operator!", (*iter)->oper);
				return;
			}
			else if (docount == 1) {
				v1 = *(iter - 1);
				if ((*iter)->oper == '?') {
					//找到数组操作的起始标记
					while ((*iter)->vtype != ONSVariableRef::TYPE_ARRAY_FLAG) iter--;
					stack->erase(iter);   //删除数组标记
					v1 = (*iter);  //被操作的数组编号
					v1->DownRefToIm(ONSVariableRef::TYPE_REAL);
					v1->InitArrayIndex();
					iter++;
					for (int ii = 0; ii < MAXVARIABLE_ARRAY && !(*iter)->isOperator(); ii++) {
						v1->SetArrayIndex((int)(*iter)->GetReal(), ii);
						stack->erase(iter, true);  //删除，同时指向下一个指针
					}
					v1->UpImToRef(ONSVariableRef::TYPE_ARRAY);
				}
				else if((*iter)->oper == '$') v1->UpImToRef(ONSVariableRef::TYPE_STRING);
				else if ((*iter)->oper == '%') v1->UpImToRef(ONSVariableRef::TYPE_INT);
				else SimpleError("%s","LOScriptReader::CalculatRPNstack() unknow docount = 1 type!");
				stack->erase(iter, true);  //单操作数只删除符号
			}
			else{
				v1 = *(iter - 2);
				v2 = *(iter - 1);
				if(!v1->Calculator(v2, (*iter)->oper ,false )) break;
				iter--;  //指向开始删除的位置
				stack->erase(iter, true); //删除后一个数
				stack->erase(iter, true); //删除后一个符号
			}
		}
		else iter++;
	}

	//计算完成后堆栈只能剩一个元素
	if (stack->size() > 1) SimpleError("Stack error after calculation, please check the formula!");
	return ;
}

void LOScriptReader::PopRPNstackUtill(LOStack<ONSVariableRef> *s1, LOStack<ONSVariableRef> *s2, char op) {
	auto iter = s1->end()-1;
	while (s1->size() > 1) {
		if ((*iter)->oper == op) {
			if (op != '?') {
				s1->erase(iter,true);   //抛弃'('  '['
			}
			return;
		}
		s2->push(*iter);
		s1->erase(iter--);  //符号转移到s2，不删除
	}
	if (op == '\0') {
		s1->clear(true);
		return;
	}
	SimpleError("missing match '%c'", op);
}



std::vector<ONSVariableRef*> LOScriptReader::TransformStack(LOStack<ONSVariableRef> *s1) {
	std::vector<ONSVariableRef*> s;
	for (auto iter = s1->begin(); iter != s1->end(); iter++) s.push_back(*iter);
	return s;
}

bool LOScriptReader::NextStartFrom(char op) {
	currentLable->c_buf = scriptbuf->SkipSpace(currentLable->c_buf);
	return ( currentLable->c_buf[0] == op);
}

bool LOScriptReader::NextStartFrom(const char* op) {
	const char *buf = currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	while (op[0] != 0) {
		if (tolower(op[0]) != tolower(buf[0])) return false;
		op++;
		buf++;
	}
	if (scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_LETTER) return false; //下一个依然是字母说明是连续的字母
	currentLable->c_buf = buf;
	return true;
}

bool LOScriptReader::NextStartFromStrVal() {
	return (NextStartFrom('"') || NextStartFrom('$'));
}

//普通单词不包括 stralias
ONSVariableRef* LOScriptReader::TryNextNormalWord() {
	const char* buf =  currentLable->c_buf;
	buf = scriptbuf->SkipSpace(buf);
	if (buf[0] == '#') {
		ONSVariableRef *v = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
		LOString s = scriptbuf->substr(buf, 7);
		v->SetValue(&s);
		currentLable->c_buf = buf + 7;
		return v;
	}
	else if (NextStartFrom('"') || NextStartFrom('$')) return ParseVariableBase();
	else {
		NextNormalWord(false);
		auto iter = strAliasMap.find(word);
		if (iter == strAliasMap.end()) {
			ONSVariableRef *v = new ONSVariableRef(ONSVariableRef::TYPE_STRING_IM);
			v->SetValue(&word);
			currentLable->c_buf += word.length();
			return v;
		}
	}
	return NULL;
}

bool LOScriptReader::LineEndIs(const char*op) {
	const char *buf = scriptbuf->NextLine(currentLable->c_buf);
	if (buf[0] == '\n') buf--;
	while (scriptbuf->GetCharacter(buf) == LOCodePage::CHARACTER_SPACE) buf--;
	buf -= strlen(op) -1;
	if (scriptbuf->GetCharacter(buf - 1) == LOCodePage::CHARACTER_LETTER) return false; //前一个是字母
	while (op[0] != 0) {
		if (tolower(op[0]) != tolower(buf[0])) return false;
		op++;
		buf++;
	}
	return true;
}

void LOScriptReader::BackLineStart() {
	const char *buf = currentLable->c_buf;
	while (buf[0] == '\n') buf--;  //已经在行尾了，往前一个符号
	while (buf[0] != '\n') buf -= scriptbuf->GetEncoder()->GetLastCharLen(buf);
	buf++;
	currentLable->c_buf = buf;
}

int LOScriptReader::LogicJump(bool iselse) {
	int deep = 0;  //if then 的嵌套层数
	while (CurrentOP() != 0) {
		NextLineStart();
		if (NextStartFrom("if") && LineEndIs("then")) deep++;
		//else 后面直接换行就是接的
		else if (iselse && NextStartFrom("else")) {
			const char *buf = currentLable->c_buf;
			buf = scriptbuf->SkipSpace(buf);
			if (buf[0] == '\n' && deep == 0)return 1; //没有深层嵌套才返回
			else if (buf[0] == '\0') return -1;
		}
		else if (NextStartFrom("endif")) {
			if (deep == 0) return 2;
			else deep--;
		}
	}
	return -1;
}


void LOScriptReader::NextLineStart() {
	currentLable->c_buf = scriptbuf->NextLine(currentLable->c_buf);
	currentLable->c_line++;
}

//用于goto等强制跳转
bool LOScriptReader::ChangePointer(LOScriptPoint *label) {
	subStack->pop();
	ReadyToRun(label);
	return true;
}

//切换当前堆栈
//bool LOScriptReader::ChangeStackData(int totype) {
//	subStack = &stackData[totype];
//	loopStack = &loopStackData[totype];
//	if (subStack->size() > 0) currentLable = subStack->top();
//	return true;
//}


bool LOScriptReader::ParseLogicExp() {
	int comtype,ret = 1; //默认为真
	int nextconet = ONSVariableRef::LOGIC_AND;
	ONSVariableRef *v1 = nullptr;
	ONSVariableRef *v2 = nullptr;
	while (true) {
		if (v1) delete v1;
		v1 = ParseVariableBase(NULL,true);
		if (NextStartFrom("==")) comtype = ONSVariableRef::LOGIC_EQUAL;
		else if (NextStartFrom("!=") || NextStartFrom("<>"))comtype = ONSVariableRef::LOGIC_UNEQUAL;
		else if (NextStartFrom(">="))comtype = ONSVariableRef::LOGIC_BIGANDEQUAL;
		else if (NextStartFrom("<="))comtype = ONSVariableRef::LOGIC_LESSANDEQUAL;
		else if (NextStartFrom('>')) {
			comtype = ONSVariableRef::LOGIC_BIGTHAN;
			NextAdress();
		}
		else if (NextStartFrom('<')) {
			comtype = ONSVariableRef::LOGIC_LESS;
			NextAdress();
		}
		else if (NextStartFrom('=')) {
			comtype = ONSVariableRef::LOGIC_EQUAL;
			NextAdress();
		}
		else {
			SimpleError("Logical expression error!");
			return false;
		}

		if (v2) delete v2;
		v2 = ParseVariableBase(NULL, true);
		if (!v2) {
			SimpleError("Logical expression error!");
			return false;
		}
		int ret0 = v1->Compare(v2, comtype,false);
		if (nextconet == ONSVariableRef::LOGIC_AND) ret &= ret0;
		else if (nextconet == ONSVariableRef::LOGIC_OR) ret |= ret0;
		if (NextStartFrom("&&") || NextStartFrom("&")) nextconet = ONSVariableRef::LOGIC_AND;
		else if (NextStartFrom("||") || NextStartFrom("|")) nextconet = ONSVariableRef::LOGIC_OR;
		else break;
	}

	if (v1) delete v1;
	if (v2) delete v2;
	if (ret == 0) return false;
	else return true;
}

LOString LOScriptReader::GetTextTagString(){
	return TagString;
}

//部分文本需展开$符号
bool LOScriptReader::ExpandStr(LOString &s) {
	LOCodePage *encoder = s.GetEncoder();
	LOString *tmp = nullptr;
	bool isexp = false;
	int ulen;
	const char* buf = s.c_str();
	const char* backbuf = currentLable->c_buf;

	while (buf[0] != 0) {
		if (buf[0] == '$' || buf[0] == '%') {
			isexp = true; //进入表达式解析模式
			if (!tmp) tmp = new LOString(s.c_str(), buf - s.c_str());
			tmp->SetEncoder(s.GetEncoder());

			LOString *optr = scriptbuf;
			scriptbuf = &s;
			currentLable->c_buf = buf;

			ONSVariableRef *v1 = ParseVariableBase();
			ONSVariableBase::AppendStrCore(tmp, v1->GetStr());
			delete v1;

			buf = currentLable->c_buf; //当前到达的位置
			scriptbuf = optr;
		}
		else {
			ulen = encoder->GetCharLen(buf);
			if (isexp) tmp->append(buf, ulen);
			buf += ulen;
		}
	}

	if (isexp) {
		s.assign(*tmp);
		delete tmp;
	}

	currentLable->c_buf = backbuf;

	return isexp;
}


const char* LOScriptReader::TryToNextCommand(const char*buf) {
	int ulen;
	while (buf[0] != 0 && buf[0] != '\n' && buf[0] != ':') {
		buf += scriptbuf->ThisCharLen(buf);
		if (buf[0] == '"') {
			const char* tbuf =  currentLable->c_buf;
			currentLable->c_buf = buf;
			ParseStrVariable();
			buf = currentLable->c_buf;
			currentLable->c_buf = tbuf;
		}
	}
	return buf;
}

int LOScriptReader::DefaultStep() {
	bool isok = false;
	LOString fn ;
	BinArray *bin;

	//0.txt

	//优先考虑00.txt-99.txt
	for (int ii = 0; ii < 100; ii++) {
		fn = std::to_string(ii) + ".txt";
		bin = ReadFile(&fn, false);
		if (!bin && ii < 10) {
			fn = "0" + fn;
			bin = ReadFile(&fn, false);
		}

		if (bin) {
			AddScript(bin->bin, bin->Length(), fn.c_str());
			SimpleInfo("scripter[%s] has read.\n", fn.c_str());
			delete bin;
			isok = true;
		}
	}

	//然后考虑通常的脚本
	if (!isok) {
		fn = "nscript.dat";
		bin = ReadFile(&fn, false);
		if (bin) {
			unsigned char *buf = (unsigned char*)bin->bin;
			for (int ii = 0; ii < bin->Length() - 1; ii++) {
				buf[ii] ^= 0x84;
			}
			AddScript(bin->bin, bin->Length(), fn.c_str());
			SimpleInfo("scripter[%s] has read.\n", fn.c_str());
			delete bin;
			isok = true;
		}
	}
#if 0
#endif

	//加入内置系统脚本
	char *tmp = (char*)(intptr_t)(__buil_in_script__);
	AddScript(tmp, strlen(__buil_in_script__), "Buil_in_script.h");
	return isok;
}

void LOScriptReader::SetMain() {
	isMain = true;
	FunctionInterface::scriptModule = this;
}

void LOScriptReader::GetGameInit(int &w, int &h) {
	w = 640; h = 480;
	if (filesList.size() == 0) return;

	LOScriptPoint *p = GetScriptPoint("__init__");
	ReadyToRun(p);
	const char *buf = currentLable->c_buf;
	const char *obuf = buf;
	while(buf[0] != 0 && buf - obuf < 4096){
		buf = scriptbuf->SkipSpace(buf);
		if (buf[0] == ';' && buf[1] == '$') {  //进入设置读取
			buf += 2;
			buf = scriptbuf->SkipSpace(buf);
			LOString s;
			while (buf[0] != 0 && buf[0] != '\n') {
				buf = scriptbuf->SkipSpace(buf);
				s = scriptbuf->GetWord(buf);
				if (s == "mode") {
					if (!strcmp(s.c_str(),"800" )) {
						w = 800; h = 600; buf += 3;
					}
					else if (!strcmp(s.c_str(), "400")) {
						w = 400; h = 300; buf += 3;
					}
					else if (!strcmp(s.c_str(), "320")) {
						w = 320; h = 240; buf += 3;
					}
					else if (!strcmp(s.c_str(), "w720")) {
						w = 1280; h = 720; buf += 4;
					}
					else break;
				}
				else if (s == "value" || s == "g" || s == "G") {
					buf = scriptbuf->SkipSpace(buf);
					s = scriptbuf->GetWord(buf);
					//global_variable_border = CharToInt(s.c_str());
				}
				else if (s == "v" || s == "V") {
					//LONS总是允许最大的变量数，忽略这个
					s = scriptbuf->GetWord(buf);
				}
				else if (s == "s" || s == "S") {
					w = scriptbuf->GetInt(buf);
					while (buf[0] == ',' || buf[0] == ' ' || buf[0] == '\t') buf++;
					h = scriptbuf->GetInt(buf);
				}
				else if (s == "l" || s == "L") {  //LONS标签数不受限制，不用管这个
					buf = scriptbuf->SkipSpace(buf);
					scriptbuf->GetInt(buf);
				}
				else if (s != ",") break;
			}
			break;
		}
		else {
			const char *ori_buf = buf;
			buf = scriptbuf->NextLine(buf);
			// FIXME: added, infinite loop
			if (buf == ori_buf) {
				break;
			}
		}
	}
	ReadyToBack();
}


int LOScriptReader::FileRemove(const char *name) {
	LOScriptReader *reader = readerList.at(0);
	if (reader->Name != Name) return reader->FileRemove(name); //转到主脚本执行，这样可以通过子类函数使用另外的读写函数
	else {
		return remove(name);
	}
}


const char* LOScriptReader::GetPrintName() {
	return activeReader->printName.c_str();
}


intptr_t LOScriptReader::GetEncoder() {
	return (intptr_t)scriptbuf->GetEncoder();
}

const char* LOScriptReader::debugCharPtr(int cur) {
	return scriptbuf->c_str() + cur;
}




//void LOScriptReader::Serialize(BinArray *sbin) {
//	sbin->WriteString("scriptStart");
//	sbin->WriteString(&Name);
//	sbin->WriteString(&printName);
//	sbin->WriteInt32((int32_t)interval);
//	sbin->WriteString(&curCmd);
//	SerializeScriptPoint(sbin, currentLable);
//	sbin->WriteInt(subStack->size());
//	for (int ii = 0; ii < subStack->size(); ii++) {
//		SerializeScriptPoint(sbin, subStack->at(ii));
//	}
//	sbin->WriteInt(loopStack->size());
//	for (int ii = 0; ii < loopStack->size(); ii++) {
//		SerializeLogicPoint(sbin, loopStack->at(ii));
//	}
//}
//
//void LOScriptReader::SerializeScriptPoint(BinArray *sbin, ScriptPoint *p) {
//	sbin->WriteString("scp");
//	if ((uintptr_t)(p) < 10) {
//		sbin->WriteInt(-1);
//		//sbin->WriteInt((int)(p));
//	}
//	else {
//		sbin->WriteInt(p->type);
//		sbin->WriteString(&p->fileName);
//		sbin->WriteString(&p->name);
//		//sbin->WriteInt(p->current_line - p->start_line);
//		sbin->WriteInt(p->current_address - p->start_address);  //相对于标签开始的位置
//		sbin->WriteBool(p->canfree);
//	}
//}
//
//void LOScriptReader::SerializeLogicPoint(BinArray *sbin, LogicPointer *p) {
//	sbin->WriteString("lcp");
//	sbin->WriteInt(p->type);
//	if (p->type == LogicPointer::TYPE_FOR || p->type == LogicPointer::TYPE_WHILE) {
//		sbin->WriteInt(p->relAdress);
//		sbin->WriteString(p->lableName);
//		sbin->WriteInt(p->step);
//		if (p->var) {
//			sbin->WriteBool(true);
//			//p->var->Serialize(sbin);
//		}
//		else sbin->WriteBool(false);
//		if (p->dstvar) {
//			sbin->WriteBool(true);
//			//p->dstvar->Serialize(sbin);
//		}
//		else sbin->WriteBool(false);
//	}
//	else {
//		sbin->WriteBool(p->ifret);
//	}
//}