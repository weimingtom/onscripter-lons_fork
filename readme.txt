see ext\SDL2-2.0.10\src\dynapi\SDL_dynapi.h

#if defined(_MSC_VER) /*&& _MSC_VER <= 1200*/
#undef SDL_DYNAMIC_API
#define SDL_DYNAMIC_API 0
#endif


search _MSC_VER <= 1200

-------------------------
LOString.cpp

const char* LOString::NextLine(const char *buf) {
	const char* ebuf = c_str() + length();
--->	if (*ebuf == 0) return ebuf;  //FIXME:added
	
	
------------------
LOLayer.cpp

#if 0	
		SDL_RenderCopyLONS(render, curInfo->texture->GetTexture(), &src, &dst, fpt);
#else
		SDL_Rect dst_ = { (int)dst.x, (int)dst.y, (int)dst.w, (int)dst.h };
		SDL_RenderCopy(render, curInfo->texture->GetTexture(), &src, &dst_);
#endif

---------------


int LOImageModule::MainLoop() {


---------------------


why skip here

void LOImageModule::UpDataLayer(LOLayer *layer, Uint32 curTime, int from, int dest, int level) {
-->	if (layer->childs.size() == 0) return;
-->	auto iter = layer->childs.rbegin();
	
	