/*
//这是一个接口类
*/



#ifndef __FUNCINTERFACE_H__
#define __FUNCINTERFACE_H__

#include "ONSvariable.h"
#include "../etc/LOStack.h"
#include "../etc/BinArray.h"
#include "../etc/LOString.h"
//#include "LOScriptPoint.h"
#include "../etc/LOEvent1.h"
#include <unordered_map>
#include <atomic>

class LOScriptPoint;

class FunctionInterface
{
public:
	FunctionInterface();
	~FunctionInterface();
	enum {
		RET_CONTINUE,
		RET_ERROR,      //致命错误，不应该继续运行了
		RET_WARNING,   //警告错误，跳过出错的命令
		RET_RETURN,    //如果已经到达堆栈尾部则应该返回
		RET_VIRTUAL,   //没有被实现的虚函数

		RET_PRETEXT = 21,   //从pretextgosub中返回
		RET_TEXT = 22,      //从textgosub中返回
		RET_LOAD = 23,      //从loadgosub中返回
	};

	enum {
		READER_WAIT_NONE,
		READER_WAIT_ENTER_PRINT,
		READER_WAIT_PREPARE,
	};

	enum {
		BUILT_FONT,  //内置字体
	};

	enum {
		USERGOSUB_PRETEXT,
		USERGOSUB_TEXT,
		USERGOSUB_LOAD
	};

	enum {
		SCRIPTER_EVENT_DALAY = 1024,   //脚本阻塞延迟
		SCRIPTER_EVENT_LEFTCLICK,
		SCRIPTER_EVENT_CLICK,
		SCRIPTER_BGMLOOP_NEXT,

		LAYER_TEXT_PREPARE,
		LAYER_TEXT_WORKING,

		AFTER_PREPARE_EFFECT_FINISH ,
		AFTER_PRINT_FINISH,
	};

	typedef int (FunctionInterface::*FuncBase)(FunctionInterface*);
	struct FuncLUT {
		const char *cmd;
		const char *param;
		const char *used;
		FuncBase method;
	};

	LOStack<ONSVariableRef> paramStack;
	FuncLUT* GetFunction(LOString &func);
	ONSVariableRef *GetParamRef(int index);
	virtual LOScriptPoint  *GetParamLable(int index);
	LOString GetParamStr(int index);
	int GetParamColor(int index);
	int GetParamInt(int index);
	int GetParamCount();

	//std::vector<intptr_t> eventStack;
	//void PushEvent(intptr_t eventID, intptr_t param, intptr_t vals);
	//bool isEvent(intptr_t eventID, intptr_t &param, intptr_t &vals);
	LOEventSlot blocksEvent;    //需要阻塞事件或者自身事件的事件槽，脚本使用

	static LOString StringFormat(int max, const char *format, ...);

	static FunctionInterface *imgeModule;    //图像模块
	static FunctionInterface *audioModule;   //视频模块
	static FunctionInterface *scriptModule;  //脚本模块
	static FunctionInterface *fileModule;    //文件系统
	static bool isExit;

	int RunCommand(FuncBase it);
	void AddWorkDir(LOString dir);
	FILE* OpenFile(const char *name, const char *rb);
	LOString GetFilePath(const char *name);
	char* ReadFileOnce(const char *name, int *len);
	LOString RandomStr(int count);

	BinArray *ReadFile(const char *fileName, bool err = true);
	virtual BinArray *ReadFile(LOString *fileName, bool err = true);
	virtual BinArray *GetBuiltMem(int type) { return NULL; }

	virtual bool NotUseTextrue(LOString *fname, uint64_t flag) { return false; };
	virtual int  AddBtn(int fullid, intptr_t ptr) { return 0; };
	virtual intptr_t GetSDLRender() { return 0; }

	//===== scripter virtual  ========
	virtual void GetGameInit(int &w, int &h) { return; };
	virtual intptr_t GetEncoder() { return 0; }
	virtual bool isName(const char* name) { return false; }
	virtual bool ExpandStr(LOString &s) { return false; }
	virtual const char* GetPrintName() { return "_main"; }
	virtual const char* GetCmdChar() { return "\0"; }
	virtual bool WaitExit(int overtime) { return false; }
	virtual void AddNextEvent(intptr_t e) { return; }
	virtual int GetCurrentLine() { return -1; }

	//===== audio virtual  ========
	virtual void PlayAfter() { return; };

	//======================  command =======================//
	virtual int movCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int operationCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int dimCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int textCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int addCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int intlimitCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int itoaCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int lenCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int midCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int movlCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int debuglogCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int numaliasCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int straliasCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int gotoCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int jumpCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int returnCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int skipCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int tablegotoCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int forCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int nextCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int breakCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int ifCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int elseCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int endifCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int resettimerCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int gettimerCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int delayCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int gameCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int endCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int defsubCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int getparamCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int labelexistCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int fileexistCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int fileremoveCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int readfileCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int splitCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int getversionCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int getenvCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int dateCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int usergosubCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int atoiCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int getmouseposCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int chkValueCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	virtual int lspCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int printCommand(FunctionInterface *reader) { return RET_VIRTUAL; };
	virtual int bgCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int cspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int mspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int cellCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int humanzCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int strspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int transmodeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgcopyCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int spfontCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int effectskipCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspmodeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspsizeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspposCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int vspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int allspCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int windowbackCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int lsp2Command(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int loadscriptCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int effectCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int windoweffectCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int btnwaitCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int spbtnCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int texecCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int textonCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int textbtnsetCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int setwindowCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int setwindow2Command(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int gettagCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int gettextCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int clickCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int erasetextwindowCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int btndefCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int btntimeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getpixcolorCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspposexCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getspalphaCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int chkcolorCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getscreenshotCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	virtual int bgmCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgmonceCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgmstopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int loopbgmCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgmfadeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int waveCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int bgmdownmodeCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int voicevolCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int chvolCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int dwaveCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int dwaveloadCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int dwaveplayCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int dwavestopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int getvoicevolCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int loopbgmstopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int stopCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	virtual int nsadirCommand(FunctionInterface *reader) { return RET_VIRTUAL; }
	virtual int nsaCommand(FunctionInterface *reader) { return RET_VIRTUAL; }

	static std::vector<LOString> workDirs;        //搜索目录，因为接口类到处都在使用，干脆把io部分挪到这里来
	static std::atomic_int flagPrepareEffect;        //print时要求截取当前画面
	static std::atomic_int flagRenderNew;            //每次完成画面刷新后，flagRenderNew都会置为1
	static LOString userGoSubName[3];
private:
	static std::unordered_map<std::string, FuncLUT*> baseFuncMap;

	static void RegisterBaseFunc();

};



#endif // !__FUNCINTERFACE_H__
