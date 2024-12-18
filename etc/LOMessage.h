#include "LOStack.h"
#include <SDL.h>
#include <unordered_map>

class LOThread{
public:
	LOThread(const char* na, const char* la, bool ismain);
	~LOThread();

	std::string name;
	std::string lable;
	SDL_threadID threadID;
	bool isMain = false ;
private:
};


class LOMessage
{
public:
	enum {
		STATE_NONE,
		STATE_EDIT,
		STATE_FINISH,
		STATE_INVALID
	};

	LOMessage();
	~LOMessage();

	bool isFinish();
	bool isState(int sa);
	bool FinishMe();
	bool isInvalid();
	bool InvalidMe();
	bool reSetState();
	//void __setState(int sa) { SDL_AtomicSet(&state, sa); }
	bool enterEdit();
	bool closeEdit();

	bool ChangeEvent(int evID, intptr_t p, intptr_t v);   //ֻ��������finish�� ��Ϣ����ת���¼���ת�����õ��ȴ���Ӧ״̬

	unsigned int hashID;
	int eventID;
	intptr_t param;
	intptr_t vals;
	
private:
	SDL_atomic_t state;
	//bool upState(int sa);
};


class LOMsgCenter {
public:

	LOMsgCenter();
	~LOMsgCenter();
	enum {  //max 31 count? maybe we use uint64 to 63 count.
		MSG_Program_Exit = 1,
		MSG_Prepare_Effect = 2,
		//MSG_Wait_Effect_Finish = 4,
		MSG_Wait_Print = 8,
		MSG_Catch_Btn = 16,
		MSG_Se_Zero_Finish = 32,
		MSG_Wait_Text = 64,
		MSG_Catch_Left_Click = 128
	};

	enum {
		DELAY_TEXT_FINISH = 1,
		DELAY_PREPARE_EFFECT_FINISH,
		DELAY_EFFECT_WORKING,
	};

	enum {
		TEXT_PREPARE = 1,
		TEXT_WORKING = 2
	};

	//void PostMessage(LOMessage *msg);
	LOMessage* PostMessage(int msgID, intptr_t param, intptr_t val);
	LOMessage* EditMessage(int msgID);
	LOMessage* EditMessageWithParam(int msgID, intptr_t param);
	bool HasMessage(int msgID);

	void SNCinvalidMessage();    //ɾ�������е���Ч��Ϣ��ע��ֻӦ������Ϣά���̵߳���
	void Exit();
	bool isExit();
	
	int  WaitMsg(LOMessage *msg, int sleeptime, int overtime, bool checkexit);
	bool AnswerMsg(int msgID, intptr_t param, intptr_t val);

	bool WaitAllThreadLeave(int overtime);

	int msgCount() { return msgList.size(); }

	LOStack<LOThread> threadList;
private:
	SDL_mutex *mutex;
	LOStack<LOMessage> msgList;
	SDL_atomic_t exitFlag;
	LOMessage* GetMsg(int msgID, bool edit, bool useParam, intptr_t paramData);
	LOMessage *GetBaseMsg(int msgID);
	//��ʵ���ģ��Ҿ���ʹ�õ���Ϣ
	LOMessage prepareMsg;
	LOMessage waitPrintMsg;
	//LOMessage waitEffectMsg;
};

extern LOMsgCenter msgManager;

//����������һ�����⣬���߳�ͬ���Ĺ����У������¼�����ȴ�֡ˢ����ɺ�Ŵ�������ź�
//��֡ˢ�µĹ����д��ݿ��ܻᵼ�����ص�����
extern std::vector<intptr_t> delayEventList;
extern void AddDelayEvent(int eveID, intptr_t vals);

