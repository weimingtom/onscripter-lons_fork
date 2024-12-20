/*
//音频部分
*/
#include "../etc/LOEvent1.h"
#include "LOAudioModule.h"
#include <mutex>

LOAudioModule::LOAudioModule() {
	audioModule = this;
	memset(_audioPtr, NULL, sizeof(LOAudioElement*) * (INDEX_MUSIC + 1));
	bgmFadeInTime = 0;
	bgmFadeOutTime = 0;
	currentChannel = 0;
	bgmDownFlag = false;
	for (int ii = 0; ii < INDEX_MUSIC + 1; ii++) _audioVol[ii] = 100;
}

LOAudioModule::~LOAudioModule() {
	for (int ii = 0; ii < INDEX_MUSIC + 1; ii++) {
		if(_audioPtr[ii]) delete _audioPtr[ii];
	}
}

void LOAudioModule::SetBGMvol(int vol, double ratio) {
	vol = ratio * vol / 100 * MIX_MAX_VOLUME;
	Mix_VolumeMusic(vol);
}

void LOAudioModule::SetSevol(int channel, int vol) {
	vol = (double)vol / 100 * MIX_MAX_VOLUME;
	Mix_Volume(channel, vol);
}

void LOAudioModule::FreeAudioEl(int index) {
	LOAudioElement *aue = GetAudioEl(index);
	if (aue) {
		if (aue->isBGM()) aue->Stop(bgmFadeOutTime);
		else aue->Stop(0);
		delete aue;
	}
}

bool LOAudioModule::CheckChannel(int channel, const char* info) {
	if (channel < 0 || channel > 49) {
		SimpleError(info, channel);
		return false;
	}
	else return true;
}

int LOAudioModule::bgmCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	BGMCore(s, -1);
	return RET_CONTINUE;
}

int LOAudioModule::bgmonceCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0);
	BGMCore(s, 0);
	return RET_CONTINUE;
}

void LOAudioModule::BGMCore(LOString &s, int looptimes) {
	FreeAudioEl(INDEX_MUSIC);

	BinArray *bin = fileModule->ReadFile(&s, false);
	LOAudioElement* aue = new LOAudioElement;
	aue->SetData(bin, -1, looptimes);
	aue->Name = s;

	SetBGMvol(_audioVol[INDEX_MUSIC], 1.0);
	SetAudioElAndPlay(INDEX_MUSIC, aue);
}

void LOAudioModule::SeCore(int channel, LOString &s, int looptimes) {
	FreeAudioEl(channel);

	BinArray *bin = fileModule->ReadFile(&s, false);
	LOAudioElement* aue = new LOAudioElement;
	aue->SetData(bin, channel, looptimes);
	aue->Name = s;

	SetSevol(channel, _audioVol[channel]);
	//播放声音时降低音量
	if (bgmDownFlag && aue->isAvailable()) SetBGMvol(_audioVol[INDEX_MUSIC], 0.6);
	SetAudioElAndPlay(channel, aue);
}

int LOAudioModule::bgmstopCommand(FunctionInterface *reader) {
	FreeAudioEl(INDEX_MUSIC);
	return RET_CONTINUE;
}

void musicFinished() {
	//NEVER call SDL Mixer functions, nor SDL_LockAudio, from a callback function.
	//让主脚本进程再回调LOAudioModule中的相关函数
	LOAudioModule *au = (LOAudioModule*)(FunctionInterface::audioModule);
	if (au->afterBgmName.length() > 0) {
		LOEvent1 *e = new LOEvent1(FunctionInterface::SCRIPTER_BGMLOOP_NEXT, (int64_t)1 );
		FunctionInterface::scriptModule->blocksEvent.SendToSlot(e);
		//LOEvent *e = new LOEvent(LOEvent::MSG_BGMLOOP_NEXT);
		//FunctionInterface::scriptModule->AddNextEvent((intptr_t)e);
	}
}

void channelFinish(int channel) {
	LOAudioModule *au = (LOAudioModule*)(FunctionInterface::audioModule);
	au->channelFinish_t(channel);
	//0频道绑定了按钮超时事件
	if (channel == 0) {
		LOEvent1 *e = G_GetEvent(LOEvent1::EVENT_SEZERO_FINISH);
		if (e) e->InvalidMe();  //直接失效事件
	}
}

void LOAudioModule::channelFinish_t(int channel) {
	LOAudioElement *aue = GetAudioEl(channel);
	if (aue) delete aue;
	if (bgmDownFlag) SetBGMvol(_audioVol[INDEX_MUSIC], 1.0);
}


int LOAudioModule::InitAudioModule() {
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, DEFAULT_AUDIOBUF) < 0) return false;
	Mix_AllocateChannels(INDEX_MUSIC);
	Mix_ChannelFinished(channelFinish);
	Mix_HookMusicFinished(musicFinished);
	return true;
}

int LOAudioModule::loopbgmCommand(FunctionInterface *reader) {
	LOString s1 = reader->GetParamStr(0);
	afterBgmName = reader->GetParamStr(1);

	LOAudioElement *aue = GetAudioEl(INDEX_MUSIC);
	aue->Stop(bgmFadeOutTime);
	Mix_HookMusicFinished(musicFinished);

	BGMCore(s1, 1);
	return RET_CONTINUE;
}

int LOAudioModule::loopbgmstopCommand(FunctionInterface *reader) {
	afterBgmName.clear();
	FreeAudioEl(INDEX_MUSIC);
	return RET_CONTINUE;
}


void LOAudioModule::PlayAfter() {
	//防止某些情况下出现死循环
	LOString s = afterBgmName;
	afterBgmName.clear();
	if (s.length() > 0) BGMCore(s, -1);
}

int LOAudioModule::bgmfadeCommand(FunctionInterface *reader) {
	if (reader->isName("bgmfadein") || reader->isName("mp3fadein")) {
		bgmFadeInTime = reader->GetParamInt(0);
	}
	else {
		bgmFadeOutTime = reader->GetParamInt(0);
	}
	if (bgmFadeInTime < 0) bgmFadeInTime = 0;
	if (bgmFadeOutTime < 0) bgmFadeOutTime = 0;
	return RET_CONTINUE;
}

int LOAudioModule::waveCommand(FunctionInterface *reader) {
	if (reader->isName("wavestop")) {
		FreeAudioEl(INDEX_WAVE);
	}
	else {
		int looptime = 0;
		if (reader->isName("waveloop")) looptime = -1;
		LOString s = reader->GetParamStr(0);
		SeCore(INDEX_WAVE, s, looptime);
	}
	return RET_CONTINUE;
}


//get以后内容就被置空了，要注意
LOAudioElement* LOAudioModule::GetAudioEl(int index) {
	ptrMutex.lock();
	LOAudioElement *ptr = _audioPtr[index];
	_audioPtr[index] = nullptr;
	ptrMutex.unlock();
	return ptr;
}

//set以后，原有的资源就被释放了，注意必须先暂停播放
void LOAudioModule::SetAudioEl(int index, LOAudioElement *aue) {
	ptrMutex.lock();
	LOAudioElement *ptr = _audioPtr[index];
	_audioPtr[index] = aue;
	ptrMutex.unlock();
	if (ptr) delete ptr;
}

void LOAudioModule::SetAudioElAndPlay(int index, LOAudioElement *aue) {
	ptrMutex.lock();
	LOAudioElement *ptr = _audioPtr[index];
	_audioPtr[index] = aue;
	if (aue) {
		if (aue->isBGM()) aue->Play(bgmFadeInTime);
		else aue->Play(0);
	}
	ptrMutex.unlock();
	if (ptr) delete ptr;
}

int LOAudioModule::bgmdownmodeCommand(FunctionInterface *reader) {
	bgmDownFlag = reader->GetParamInt(0);
	if (!bgmDownFlag) SetBGMvol(_audioVol[INDEX_MUSIC], 1.0); //恢复已经被设置的音量 
	return RET_CONTINUE;
}



int LOAudioModule::voicevolCommand(FunctionInterface *reader) {
	int vol = reader->GetParamInt(0);
	if (vol < 0 || vol > 100) vol = 100;

	if (reader->isName("bgmvol") || reader->isName("mp3vol")) {
		_audioVol[INDEX_MUSIC] = vol;
		SetBGMvol(vol, 1.0);
	}
	else if (reader->isName("voicevol")) {
		_audioVol[INDEX_WAVE] = vol;
		SetSevol(INDEX_WAVE, vol);
	}
	else {
		for (int ii = 0; ii < INDEX_WAVE; ii++) {
			_audioVol[ii] = vol;
			SetSevol(ii, vol);
		}
	}

	return RET_CONTINUE;
}

int LOAudioModule::chvolCommand(FunctionInterface *reader) {
	int channel = reader->GetParamInt(0);
	int vol = reader->GetParamInt(1);
	if (vol < 0 || vol > 100) vol = 100;
	if (!CheckChannel(channel, "[chvol] channel out of range:%d")) return RET_CONTINUE;

	_audioVol[channel] = vol;
	SetSevol(channel, _audioVol[channel]);

	return RET_CONTINUE;
}

int LOAudioModule::dwaveCommand(FunctionInterface *reader) {
	int channel = reader->GetParamInt(0);
	LOString s = reader->GetParamStr(1);

	if (!CheckChannel(channel, "[dwave] channel out of range:%d")) return RET_CONTINUE;
	
	int loopcount = 0;
	if (reader->isName("dwaveloop")) loopcount = -1;
	SeCore(channel, s, loopcount);
	currentChannel = channel;
	return RET_CONTINUE;
}

int LOAudioModule::dwaveloadCommand(FunctionInterface *reader) {
	int channel = reader->GetParamInt(0);
	LOString s = reader->GetParamStr(1);

	if (!CheckChannel(channel, "[dwaveload] channel out of range:%d")) return RET_CONTINUE;

	FreeAudioEl(channel);
	BinArray *bin = fileModule->ReadFile(&s, false);
	LOAudioElement *aue = new LOAudioElement;
	aue->SetData(bin, channel, 0);
	SetAudioEl(channel, aue);

	return RET_CONTINUE;

}

int LOAudioModule::dwaveplayCommand(FunctionInterface *reader) {
	int channel = reader->GetParamInt(0);
	if (!CheckChannel(channel, "[dwaveplay] channel out of range:%d")) return RET_CONTINUE;
	int looptime = 0;
	if (reader->isName("dwaveplayloop"))  looptime = -1;
	LOAudioElement *aue = GetAudioEl(channel);
	if (aue) {
		aue->loopCount = looptime;
		SetAudioElAndPlay(channel, aue);
	}
	currentChannel = channel;
	return RET_CONTINUE;
}

int LOAudioModule::dwavestopCommand(FunctionInterface *reader) {
	FreeAudioEl(currentChannel);
	return RET_CONTINUE;
}

int LOAudioModule::getvoicevolCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	int vol = _audioVol[INDEX_MUSIC];
	if (reader->isName("getvoicevol")) vol = _audioVol[INDEX_WAVE];
	else if (reader->isName("getsevol")) vol = _audioVol[currentChannel];
	v->SetValue((double)vol);
	return RET_CONTINUE;
}

int LOAudioModule::stopCommand(FunctionInterface *reader) {
	//禁用回调函数
	Mix_ChannelFinished(NULL);
	Mix_HookMusicFinished(NULL);
	for (int ii = 0; ii < INDEX_MUSIC + 1; ii++) {
		LOAudioElement *aue = GetAudioEl(ii);
		if (aue) {
			if (aue->isBGM()) aue->Stop(bgmFadeOutTime);
			else aue->Stop(0);
			delete aue;
		}
	}
	Mix_ChannelFinished(channelFinish);
	Mix_HookMusicFinished(musicFinished);
	return RET_CONTINUE;
}