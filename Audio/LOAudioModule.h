/*
//音频部分
*/
#ifndef __LOAUDIO_H__
#define __LOAUDIO_H__



#include "../Scripter/FuncInterface.h"
#include "LOAudioElement.h"
#include <SDL.h>
#include <SDL_mixer.h>

#define DEFAULT_AUDIOBUF 4096

class LOAudioModule :public FunctionInterface{
public:
	LOAudioModule();
	~LOAudioModule();
	
	enum {
		//0~49 is se
		INDEX_WAVE = 50,
		INDEX_MUSIC = 51,
	};
	LOString afterBgmName;
	int InitAudioModule();
	void PlayAfter();
	void channelFinish_t(int channel);
	

	int bgmCommand(FunctionInterface *reader);
	int bgmonceCommand(FunctionInterface *reader);
	int bgmstopCommand(FunctionInterface *reader);
	int loopbgmCommand(FunctionInterface *reader);
	int bgmfadeCommand(FunctionInterface *reader);
	int waveCommand(FunctionInterface *reader);
	int bgmdownmodeCommand(FunctionInterface *reader);
	int voicevolCommand(FunctionInterface *reader);
	int chvolCommand(FunctionInterface *reader);
	int dwaveCommand(FunctionInterface *reader);
	int dwaveloadCommand(FunctionInterface *reader);
	int dwaveplayCommand(FunctionInterface *reader);
	int dwavestopCommand(FunctionInterface *reader);
	int getvoicevolCommand(FunctionInterface *reader);
	int loopbgmstopCommand(FunctionInterface *reader);
	int stopCommand(FunctionInterface *reader);

private:
	int bgmFadeInTime;
	int bgmFadeOutTime;
	int currentChannel;
	bool bgmDownFlag;

	//每次播放都新建一个LOAudioElement，因此设置，读取时必须加锁
	LOAudioElement *_audioPtr[INDEX_MUSIC + 1];  //不要直接操作
	int _audioVol[INDEX_MUSIC + 1];
	std::mutex ptrMutex;

	
	void SetAudioEl(int index, LOAudioElement *aue);
	void SetAudioElAndPlay(int index, LOAudioElement *aue);
	LOAudioElement* GetAudioEl(int index);
	void FreeAudioEl(int index);
	void BGMCore(LOString &s, int looptimes);
	void SeCore(int channel, LOString &s, int looptimes);
	void SetBGMvol(int vol, double ratio);
	void SetSevol(int channel, int vol);
	bool CheckChannel(int channel,const char* info);
};



#endif // !__LOAUDIO_H__