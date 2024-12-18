#include "LOMessage.h"

LOMsgCenter msgManager;
std::vector<intptr_t> delayEventList;

void CPUdelayLitte(int count) {
	int sum = rand();
	for (int ii = 0; ii < count; ii++) {
		sum ^= ii;
		if (ii % 2) sum++;
		else sum += 2;
	}
}

double GetHticks(Uint64 last, Uint64 current) {
	Uint64 perHtickTime = SDL_GetPerformanceFrequency() / 1000;
	return ((double)(current - last)) / perHtickTime;
}

void AddDelayEvent(int eveID, intptr_t vals) {
	delayEventList.push_back(eveID);
	delayEventList.push_back(vals);
}


LOThread::LOThread(const char* na, const char* la, bool ismain){
	name.assign(na);
	lable.assign(la);
	isMain = ismain;
}

LOThread::~LOThread()
{
}

LOMessage::LOMessage() {
	hashID = NULL;
	eventID = NULL;
	param = 0;
	vals = 0;
	SDL_AtomicSet(&state, STATE_NONE);
}

LOMessage::~LOMessage() {

}

bool LOMessage::isFinish() {
	return (SDL_AtomicGet(&state) == STATE_FINISH);
}

bool LOMessage::isInvalid() {
	return (SDL_AtomicGet(&state) == STATE_INVALID);
}

bool LOMessage::reSetState() {
	return (SDL_AtomicCAS(&state, STATE_FINISH, STATE_NONE) == SDL_TRUE);
}

bool LOMessage::ChangeEvent(int evID, intptr_t p, intptr_t v) {
	if (!isFinish()) return false;
	eventID = evID;
	param = p;
	vals = v;
	return (SDL_AtomicCAS(&state, STATE_FINISH, STATE_NONE) == SDL_TRUE);
}


//这么做的原因是为了保证 完成的或者无效的消息不能被改写
//bool LOMessage::upState(int sa) {
//	int last, count = 1;
//	do {
//		last = SDL_AtomicGet(&state);
//		if (last > sa) return false;
//		else if (last < sa && SDL_AtomicCAS(&state, last, sa) == SDL_TRUE) return true;
//		count++;
//	} while (count <= 2);
//	return false;
//}

bool LOMessage::enterEdit() {
	return (SDL_AtomicCAS(&state, STATE_NONE, STATE_EDIT) == SDL_TRUE);
}

//从edit状态返回None
bool LOMessage::closeEdit() {
	return (SDL_AtomicCAS(&state, STATE_EDIT, STATE_NONE) == SDL_TRUE);
}

//必须完成
bool LOMessage::FinishMe() {
	int cat;
	while (true) {
		cat = SDL_AtomicGet(&state);
		if (cat >= STATE_FINISH) return true;
		SDL_AtomicCAS(&state, cat, STATE_FINISH);
	}
	return true;
}

//必须完成
bool LOMessage::InvalidMe() {
	int cat;
	while (true) {
		cat = SDL_AtomicGet(&state);
		if (cat >= STATE_INVALID) return true;
		SDL_AtomicCAS(&state, cat, STATE_INVALID);
	}
	return true;
}

bool LOMessage::isState(int sa) {
	return (SDL_AtomicGet(&state) == sa);
}



LOMsgCenter::LOMsgCenter() {
	mutex = SDL_CreateMutex();
	SDL_AtomicSet(&exitFlag, 0);
	prepareMsg.FinishMe();
	waitPrintMsg.FinishMe();
	//waitEffectMsg.FinishMe();
}

LOMsgCenter::~LOMsgCenter() {
	SDL_DestroyMutex(mutex);
}


//void LOMsgCenter::PostMessage(LOMessage *msg) {
//	SDL_LockMutex(mutex);
//	msgList.push(msg);
//	SDL_UnlockMutex(mutex);
//}

LOMessage* LOMsgCenter::PostMessage(int msgID, intptr_t param, intptr_t val) {
	LOMessage *msg = GetBaseMsg(msgID);
	if (msg) {
		msg->eventID = msgID;
		msg->param = param;
		msg->vals = val;
		msg->reSetState();
	}
	else {
		msg = new LOMessage;
		msg->eventID = msgID;
		msg->param = param;
		msg->vals = val;
		SDL_LockMutex(mutex);
		msgList.push(msg);
		SDL_UnlockMutex(mutex);
	}
	return msg;
}

void LOMsgCenter::Exit() {
	SDL_AtomicSet(&exitFlag, 1);
}

bool LOMsgCenter::isExit() {
	return (SDL_AtomicGet(&exitFlag) == 1);
}




LOMessage *LOMsgCenter::GetBaseMsg(int msgID) {
	switch (msgID) {
	case MSG_Prepare_Effect:
		return &prepareMsg;
	case MSG_Wait_Print:
		return &waitPrintMsg;
	default:
		return NULL;
	}
	return NULL;
}


LOMessage* LOMsgCenter::GetMsg(int msgID, bool edit, bool useParam, intptr_t paramData) {
	LOMessage *msg = GetBaseMsg(msgID);
	if (msg) {
		if (!msg->isState(LOMessage::STATE_NONE)) return NULL;  //无聊是否进入编辑状态，有效的消息应该在初始状态
		if (edit && !msg->enterEdit()) return NULL;
		if (useParam && msg->param != paramData) {
			if(edit) msg->closeEdit();   //不是指定参数，重置状态
			return NULL;
		}
		return msg;
	}

	SDL_LockMutex(mutex);
	for (int ii = 0; ii < msgList.size(); ii++) {
		LOMessage *msg = msgList.at(ii);
		if (msg->eventID & msgID) {
			if (!msg->isState(LOMessage::STATE_NONE)) continue;
			if (edit && !msg->enterEdit()) continue;
			if (useParam && msg->param != paramData) {
				if (edit) msg->closeEdit();   //不是指定参数，重置状态
				continue;
			}
			SDL_UnlockMutex(mutex);
			return msg;
		}
	}
	SDL_UnlockMutex(mutex);
	return NULL;
}

LOMessage* LOMsgCenter::EditMessage(int msgID) {
	return GetMsg(msgID, true, false, 0);
}

bool LOMsgCenter::HasMessage(int msgID) {
	return GetMsg(msgID, false, false, 0);
}

LOMessage* LOMsgCenter::EditMessageWithParam(int msgID, intptr_t param) {
	return GetMsg(msgID, true, true, param);
}

void LOMsgCenter::SNCinvalidMessage() {
	SDL_LockMutex(mutex);
	int oldCount = msgList.size();
	for (int ii = 0; ii < msgList.size(); ii++) {
		LOMessage *msg = msgList.at(ii);
		if (msg->isInvalid()) {
			msgList.swap((LOMessage*)NULL, ii);  //注意重载函数NULL会被识别为 0，最好用nullptr
			if (!GetBaseMsg(msg->eventID)) delete msg;  //非静态消息会被删除
		}
	}
	msgList.clearEmpty();
	if (oldCount != msgList.size()) SDL_Log("msgCount is:%d\n", msgList.size());
	SDL_UnlockMutex(mutex);
}

//overtime 0, exit -1 , ok 1
int  LOMsgCenter::WaitMsg(LOMessage *msg, int sleeptime, int overtime, bool checkexit) {
	Uint64 startT;
	if (overtime > 0) {
		startT = SDL_GetPerformanceCounter();
	}

	while (!msg->isFinish()) {
		if (sleeptime > 0) SDL_Delay(sleeptime);
		else {
			int sum = rand(); //CPUdelayLitte(200)
			for (int ii = 0; ii < 200; ii++) {
				sum ^= ii;
				if (ii % 2) sum++;
				else sum += 2;
			}
		};
		if (checkexit && isExit()) return -1;
		if (overtime > 0 && GetHticks(startT, SDL_GetPerformanceCounter()) > overtime) return 0;
	}
	return 1;
}

//true if we answer a msg.false not msg or not answer
bool LOMsgCenter::AnswerMsg(int msgID, intptr_t param, intptr_t val) {
	LOMessage *msg = EditMessage(msgID);
	if (!msg) return false;
	msg->param = param;
	msg->vals = val;
	msg->FinishMe();
	return true;
}


bool LOMsgCenter::WaitAllThreadLeave(int overtime) {
	Uint64 startT = SDL_GetPerformanceCounter();
	while (threadList.size() > 0) {
		SDL_Delay(2);
		if (GetHticks(startT, SDL_GetPerformanceCounter()) > overtime) return false;
	}
	return true;
}



