/*
项目地址：https://gitee.com/only998/onscripter-lons
邮件联系：pngchs@qq.com
编辑时间：2020——2021
*/

#include "Render/LOImageModule.h"
#include "Scripter/LOScriptReader.h"
#include "File/LOFileModule.h"
#include "Audio/LOAudioModule.h"
#include "etc/LOEvent1.h"

#include <SDL.h>
#include <chrono>

/*
#include <SDL_thread.h>
#undef SDL_CreateThread
#define SDL_CreateThread(fn, name, data) SDL_CreateThread(fn, name, data, (pfnSDL_CurrentBeginThread)_beginthreadex, (pfnSDL_CurrentEndThread)_endthreadex)
#include <windows.h>
*/

#ifdef ANDROID
#include <android/log.h>
#include <unistd.h>
#endif


//ONScripter ons;

void GetIntSet(int *a, int *b, char *buf) {
	//int c = *a >> 16;
	//if (!(c >> 16)) {
	//	std::string val;
	//	buf = GetWord(&val, buf);
	//	buf = GetWord(&val, buf);
	//	*a = CharToInt(val.c_str());
	//	if (b) {
	//		buf = GetWord(&val, buf);
	//		buf = GetWord(&val, buf);
	//		*b = CharToInt(val.c_str());
	//	}
	//}
}

//读取lons.cfg的配置文件
void ReadConfig() {
	//FILE *f = tryOpenFile("lons.cfg", "r");
	//if (!f)return;
	//char *line = new char[256];
	//std::string keywork;

	//while (fgets(line, 255, f)) {
	//	char *buf = SkipSpace(line);
	//	buf = GetWord(&keywork, buf);
	//	if (keywork == "fullscreen") GetIntSet(&G_cfgIsFull,nullptr, buf);
	//	else if (keywork == "window") GetIntSet(&G_cfgwindowW,&G_cfgwindowH,buf);
	//	else if (keywork == "outline")GetIntSet(&G_cfgoutlinePix, nullptr, buf);
	//	else if (keywork == "shadow") GetIntSet(&G_cfgfontshadow, nullptr, buf);
	//	else if (keywork == "fps") GetIntSet(&G_cfgfps, nullptr, buf);
	//	else if (keywork == "ratio") GetIntSet(&G_gameRatioW, &G_gameRatioH, buf);
	//	else if (keywork == "position") GetIntSet(&G_cfgposition, nullptr, buf);
	//	else if (keywork == "logfile") GetIntSet(&G_useLogFile, nullptr, buf);
	//}
	//fclose(f);
	//delete[] line;
}

void FreeModules(LOFileModule *&filemodule, LOScriptReader *&reader, LOImageModule *&imagemodule, LOAudioModule *&audiomodule) {
	if (filemodule) delete filemodule;
	filemodule = NULL;

	if (reader) {
		while (LOScriptReader::readerList.size() > 0) delete LOScriptReader::readerList.pop();
	}
	filemodule = NULL;

	if (imagemodule) delete imagemodule;
	imagemodule = NULL;

	if (audiomodule)delete audiomodule;
	audiomodule = NULL;
}

int ScripterThreadEntry(void *ptr) {
	//init labels
	LOScriptReader::InitScriptLabels();
	LOScriptReader *reader = (LOScriptReader*)ptr;
	reader->MainTreadRunning();
	return 0;
}



int main(int argc, char **argv) {

#ifdef LONS_DEBUG
	LONS::printInfo("debug model has run...");
	//chdir("/storage/emulated/0/Download/test");
	chdir("/storage/emulated/0/Download/YZ");
	char path[255];
	memset(path, 0, sizeof(path));
	getcwd(path, sizeof(path) - 1);
	G_workdir.assign(path);
	LONS::printInfo("debug dir is:%s", path);
#endif

	//SDL_LogSetOutputFunction(LogFunction, NULL);
	SDL_Log("LONS engine has been run from the main() function!");

	//GlobalInit();
	ReadConfig();
	//if (G_useLogFile) UseLogFile();

	//SDL_Log("LONS version %s(%d.%02d)\n", ONS_VERSION, NSC_VERSION / 100, NSC_VERSION % 100);
	SDL_Log("***==========LONS,New generation of high performance ONScripter engine==========*** \n");
	// ================ test ================== //
	//G_EventQue.AddEvent(nullptr);

	// ================ start ================== //
	LOFileModule *filemodule = NULL;
	LOScriptReader *reader = NULL;
	LOImageModule *imagemodule = NULL;
	LOAudioModule *audiomodule = NULL;
	

	int exitflag = -1;
	srand(SDL_GetPerformanceCounter());   //初始化随机数种子
	G_InitSlots();   //初始化线程同步事件槽

	//int ccc = sizeof(LOImageModule);

	while (exitflag == -1) {
		//reset的时候直接整个重来
		FreeModules(filemodule, reader, imagemodule,audiomodule);

		//初始化IO，必须优先进行IO，因为后面要读文件
		filemodule = new LOFileModule;

		//添加路径搜索

		//初始化脚本模块
		reader = LOScriptReader::EnterScriptReader(MAINSCRIPTER_NAME);
		reader->SetMain();

		imagemodule = new LOImageModule;
		//ResetTimer();

		//加载脚本
		if (reader->DefaultStep()) {
			
			//uint32_t tttt = GetTimer();
			//ResetTimer();

			//初始化渲染模块
			if (imagemodule->InitImageModule()) {
				//tttt = GetTimer();

				//初始化音频模块
				audiomodule = new LOAudioModule;
				if (audiomodule->InitAudioModule()) {

					//启动脚本线程，渲染开始进入主循环
					SDL_CreateThread(ScripterThreadEntry, "script", (void*)reader);
					imagemodule->MainLoop();
					exitflag = 0;
				}
				else exitflag = -4;
			}
			else exitflag = -3;
		}
		

		else {
			SDL_LogError(0, "LONS can't find [nscript.dat] or [0.txt~99.txt]!\n");
			exitflag = -2;
		}
	}

	FreeModules(filemodule, reader, imagemodule, audiomodule);
	G_DestroySlots();  //释放线程同步信号槽

	if (exitflag == -1) exitflag = 0;
	return exitflag;
}

