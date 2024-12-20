
#ifndef __BINARRAY_ONLY998_H__
#define __BINARRAY_ONLY998_H__

#include <stdlib.h>
#include <stdint.h>
#include <string>

//默认是Little Endian模式的
class BinArray
{
public:
	//字节集的默认大小和是否为流模式
	BinArray(int prepsize = 8 , bool isstream = false);

	//用内存中创建字节集，长度为-1表示自动检测字符串
	BinArray(const char *buf,int length = -1);

	~BinArray();

	void Clear(bool resize);
	uint32_t Length() { return realLen; }
	void SetLength(uint32_t len) { realLen = len; }

	char GetChar(int *pos);
	char GetChar(int pos) { return GetChar(&pos); }
	short int GetShortInt(int *pos,bool isbig = false);
	short int GetShortInt(int pos, bool isbig = false) { return GetShortInt(&pos, isbig); }
	int  GetInt(int *pos, bool isbig = false);
	int  GetInt(int pos, bool isbig = false) { return GetInt(&pos, isbig); }
	int8_t GetInt8(int *pos);
	int8_t GetInt8(int pos) { return GetInt8(&pos); }
	int16_t GetInt16(int *pos, bool isbig = false);
	int16_t GetInt16(int pos, bool isbig = false) { return GetInt16(&pos, isbig); }
	int32_t GetInt32(int *pos, bool isbig = false);
	int32_t GetInt32(int pos, bool isbig = false) { return GetInt32(&pos, isbig); }
	int64_t GetInt64(int *pos, bool isbig = false);
	int64_t GetInt64(int pos, bool isbig = false) { return GetInt64(&pos, isbig); }
	std::string GetString(int *pos);
	std::string GetString(int pos) { return GetString(&pos); }
	bool GetBool(int *pos);
	bool GetBool(int pos) { return GetBool(&pos); }
	float GetFloat(int *pos, bool isbig = false);
	float GetFloat(int pos, bool isbig = false) { return GetFloat(&pos, isbig); }
	double GetDouble(int *pos, bool isbig = false);
	double GetDouble(int pos, bool isbig = false) { return GetDouble(&pos, isbig); }

	void Append(const char *buf,int len);
	void Append(BinArray *v);

	int WriteChar(char v,int *pos = NULL);
	int WriteShortInt(short int v, int *pos = NULL, bool isbig = false);
	int WriteInt(int v, int *pos = NULL, bool isbig = false);
	int WriteInt16(int16_t v, int *pos = NULL, bool isbig = false);
	int WriteInt32(int32_t v, int *pos = NULL, bool isbig = false);
	int WriteInt64(int64_t v, int *pos = NULL, bool isbig = false);
	int WriteFloat(float v, int *pos = NULL, bool isbig = false);
	int WriteDouble(double v, int *pos = NULL, bool isbig = false);
	int WriteBool(bool v, int *pos = NULL, bool isbig = false);
	int WriteString(std::string *v, int *pos = NULL, bool isbig = false);
	int WriteString(const char *buf, int *pos = NULL, bool isbig = false);


	int WriteFillEmpty(int len, int *pos = NULL);

	static BinArray* ReadFromFile(const char* name);
	static BinArray* ReadFile(FILE *f,int pos,int len);

	char *bin;
private:
	enum {
		BIN_PREPLEN = 20
	};

	uint32_t realLen;  //实际使用的长度
	uint32_t prepLen;  //准备使用的长度
	bool isStream;     //流模式每次准备长度翻倍，非流模式每次准备长度+20
	const char *errerinfo;   //上一次错误信息

	void NewSelf(int prepsize, bool isstream);
	void ReverseBytes(char *buf,int len);
	void GetCharIntArray(char *buf, int len, int *pos, bool isbig);
	void WriteCharIntArray(char *buf, int len, int *pos, bool isbig);
	void AddMemory(int len);
};


#endif // !__BINARRAY_ONLY998_H__

