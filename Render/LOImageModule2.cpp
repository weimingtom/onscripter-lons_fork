/*
图像队列的处理
*/
#include "../etc/LOEvent1.h"
#include "LOImageModule.h"


LOLayerInfo *LOImageModule::GetInfoNewAndFreeOld(int fullid, const char* print_name) {
	LOLayerInfo *info = GetInfoNewAndNoFreeOld(fullid, print_name);
	FreeLayerInfoData(info);
	info->Reset();
	info->fullid = fullid;
	return info;
}

LOLayerInfo *LOImageModule::GetInfoNewAndNoFreeOld(int fullid, const char* print_name) {
	SDL_LockMutex(poolMutex);

	LOLayerInfo *info = NULL;
	int index = -1;
	LOLayerInfoCacheIndex *minfo = GetCacheIndexFromName(fullid, print_name, &index);
	if (minfo) {
		info = &minfo->info;
		//FreeLayerInfoData(info);
		//info->Reset();
		info->fullid = fullid;
	}
	else {
		minfo = GetCacheIndexFromPool(fullid, nameList[index].c_str());
		minfo->map = mapList.at(index);
		(*minfo->map)[fullid] = minfo;
		info = &minfo->info;
	}
	SDL_UnlockMutex(poolMutex);
	return info;
}

LOLayerInfo *LOImageModule::GetInfoNew(int fullid, const char* print_name) {
	LOLayerInfo *info = NULL;
	int index = -1;
	LOLayerInfoCacheIndex *minfo = GetCacheIndexFromName(fullid, print_name, &index);

	SDL_LockMutex(poolMutex);
	minfo = GetCacheIndexFromPool(fullid, nameList[index].c_str());
	minfo->map = mapList.at(index);
	(*minfo->map)[fullid] = minfo;
	info = &minfo->info;
	SDL_UnlockMutex(poolMutex);
	return info;
}

void LOImageModule::NotUseInfo(LOLayerInfoCacheIndex *minfo) {
	LOLayerInfo *info = &minfo->info;
	info->Reset();
	info->ResetOther();
	minfo->iswork = false;
}

LOLayerInfoCacheIndex * LOImageModule::GetCacheIndexFromName(int fullid, const char *print_name, int *mapindex) {
	*mapindex = GetInfoCacheFromName(print_name);
	auto map = mapList.at(*mapindex);
	auto iter = map->find(fullid);
	if (iter != map->end()) return iter->second;
	else return NULL;
}

//根据提供的控制类型提取出列表
LOStack<LOLayerInfoCacheIndex>* LOImageModule::FilterCacheQue(const char *print_name, int layertype, int conid) {
	LOStack<LOLayerInfoCacheIndex>* list = new LOStack<LOLayerInfoCacheIndex>;
	int index = GetInfoCacheFromName(print_name);
	auto map = mapList.at(index);
	for (auto iter = map->begin(); iter != map->end(); iter++) {
		LOLayerInfo *info = &iter->second->info;
		if (layertype >= 0 && GetIDs(info->fullid, 0) != layertype) continue; //filter type
		if (info->layerControl & conid) list->push(iter->second);
	}
	return list;
}

void LOImageModule::ClearCacheMap(LOStack<LOLayerInfoCacheIndex> *list) {
	SDL_LockMutex(poolMutex);
	for (int ii = 0; ii < list->size(); ii++) {
		list->at(ii)->Reset();
	}
	SDL_UnlockMutex(poolMutex);
	list->clear();
}

int LOImageModule::GetInfoCacheFromName(const char *buf) {
	for (int ii = 0; ii < nameList.size(); ii++) {
		if (strcmp(buf, nameList[ii].c_str()) == 0) return ii;
	}
	nameList.push_back(std::string(buf));
	mapList.push(new std::unordered_map<int, LOLayerInfoCacheIndex*>);
	return mapList.size() - 1;
}

//由外部锁定线程锁
LOLayerInfoCacheIndex *LOImageModule::GetCacheIndexFromPool(int fullid, const char* cacheN) {
	LOLayerInfoCacheIndex *minfo = NULL;
	//使用环形队列
	int count = 0;
	for (count = 0; count < poolData.size(); count++) {
		minfo = poolData.at(poolCurrent);
		poolCurrent = (poolCurrent + 1) % poolData.size();
		if (!minfo->iswork) break;
	}
	//检查是否需要增加 信息池
	if (poolData.size() - count < 10) {
		poolCurrent = poolData.size();
		for (int ii = 0; ii < 50; ii++) poolData.push(new LOLayerInfoCacheIndex);
		minfo = poolData.at(poolCurrent);
		SDL_Log("poolData's size be to %d\n", poolData.size());
	}
	if (!minfo || minfo->iswork) {
		SDL_LogError(0, "ONScripterImage::GetCacheIndexFromPool() faild!");
		return NULL;
	}
	minfo->iswork = true;
	minfo->name = cacheN;
	minfo->info.fullid = fullid;
	return minfo;
}


//优先考虑父对象，再考虑子对象的原则排列信息组
LOStack<LOLayerInfoCacheIndex> * LOImageModule::SortCacheMap(std::unordered_map<int, LOLayerInfoCacheIndex*> *map) {
	LOStack<LOLayerInfoCacheIndex> *list = new LOStack<LOLayerInfoCacheIndex>;
	auto iter = map->begin();
	while (iter != map->end()) {
		int fullid = iter->second->info.fullid;
		if (GetIDs(fullid, 1) == 255) {
			list->push(iter->second);
			iter = map->erase(iter);
		}
		else iter++;
	}

	iter = map->begin();
	while (iter != map->end()) {
		int fullid = iter->second->info.fullid;
		if (GetIDs(fullid, 2) == 255) {
			list->push(iter->second);
			iter = map->erase(iter);
		}
		else iter++;
	}

	iter = map->begin();
	while (iter != map->end()) {
		list->push(iter->second);
		iter++;
	}
	map->clear();
	return list;
}

#define threeParam ((LOEventParamBtnRef*)e->param)

void EventDo(LOEvent1 *e, double postime) {
	if (!e->enterEdit()) return;  //事件已经不再有效

	auto *img = (LOImageModule*)FunctionInterface::imgeModule;
	switch (e->eventID) {
	case LOEvent1::EVENT_PREPARE_EFFECT:   //print2-18准备活动完成，并且进行第一帧
		if (e->param == LOImageModule::PARAM_SCREENSHORT) { //截图
			img->ScreenShotCountinue(e);
		}
		else if (e->param == LOImageModule::PARAM_BGCOPY) {

		}
		else {
			img->PrepareEffect((LOEffect*)(threeParam->ptr1), (const char*)(threeParam->ptr2));
			e->FinishMe();
		}
		break;

	case LOEvent1::EVENT_WAIT_PRINT:    //等待effect运行完
		if (img->ContinueEffect((LOEffect*)(threeParam->ptr1), postime)) {  //effect运算，完成返回真
			e->FinishMe();
		}
		else e->closeEdit();  //关闭编辑状态，下次才能继续获取事件
		break;
	default:
		break;
	}
}

void LOImageModule::DoDelayEvent(double postime) {
	//延迟事件队列只应该用于 updisplay() 里需要等待帧完成后刷新后通知的情况
	//虽然增加了事件执行的复杂度，但是我认为这是必要的，统一处理事件更容易确定程序中用了哪些通讯事件，以及外部处理中断事件
	LOEventSlot *slot = GetEventSlot(LOEvent1::EVENT_IMGMODULE_AFTER);
	
	//通过函数调用，避免暴露 LOEventSlot 的内部成员
	int ecount = slot->ForeachCall(&EventDo, postime);
	//运行时的同时事件不可能超过20个，超过该检查问题了
	if (ecount > 20) SimpleError("LonsEvent is too much! check event logic please!");
}


int LOImageModule::ExportQuequ(const char *print_name, LOEffect *ef, bool iswait) {
	int index = GetInfoCacheFromName(print_name);
	auto map = mapList.at(index);
	if (map->size() == 0) return READER_WAIT_NONE;

	iswait = true;  //刷新都改为同步的，异步暂时不考虑

	if (iswait) SDL_LockMutex(doQueMutex);
	else {
		if (SDL_TryLockMutex(doQueMutex) != 0) return READER_WAIT_ENTER_PRINT;
	}

	//非print1则要求抓取当前显示的图像，下一帧在继续执行
	if (ef) {
		auto *param = new LOEventParamBtnRef;
		param->ptr1 = (intptr_t)ef;
		param->ptr2 = (intptr_t)print_name;
		LOEvent1 *e = new LOEvent1(LOEvent1::EVENT_PREPARE_EFFECT, param);
		G_SendEvent(e);

		if(!iswait) return READER_WAIT_PREPARE;
		e->waitEvent(1, -1);
		e->InvalidMe();
	}

	//we will add layer or delete layer and btn ,so we lock it,main thread will not render.
	SDL_LockMutex(layerQueMutex);
	SDL_LockMutex(btnQueMutex);

	LOLayer *layer, *temp;
	LOStack<LOLayerInfoCacheIndex> *list = SortCacheMap(map);
	for (int ii = 0; ii < list->size(); ii++) {
		LOLayerInfoCacheIndex *minfo = list->at(ii);
		if (!minfo->iswork || minfo->info.layerControl == LOLayerInfo::CON_NONE) continue;
		//LONS::printError("id[0]:%d,%d,%x",idd[0],idd[1],info->fullid);
		//图层被删除，或重新载入，应删除上一个按钮
		//优先处理删除事件
		if (minfo->info.layerControl & LOLayerInfo::CON_DELLAYER) {
			LOLayer::SysLayerType ltype;
			int ids[3];
			GetTypeAndIds( (int*)(&ltype), ids, minfo->info.fullid);
			LOLayer *layer = FindLayerInBase(ltype, ids);
			if (layer) {
				layer->Root->RemodeChild(layer->id[0]);
				delete layer;
			}
			//删除按钮定义
			RemoveBtn(minfo->info.fullid);
		}
		else {
			layer = GetLayerOrNew(minfo->info.fullid);
			if (!layer->parent) { //插入图层
				layer->Root = GetRootLayer(minfo->info.fullid);
				layer->SetFullID(minfo->info.fullid);
				layer->Root->InserChild(layer);
			}
			layer->UseControl(&minfo->info, btnMap);
		}
	}
	ClearCacheMap(list);
	delete list;
	//提交一次刷新标记
	LOEvent1 *ep = NULL;
	if (iswait) {
		auto *param = new LOEventParamBtnRef;
		param->ptr1 = (intptr_t)ef;
		param->ptr2 = (intptr_t)print_name;
		ep = new LOEvent1(LOEvent1::EVENT_WAIT_PRINT, param);
		G_SendEvent(ep);
	}
	SDL_UnlockMutex(layerQueMutex);
	SDL_UnlockMutex(btnQueMutex);
	if (ep) {
		ep->waitEvent(1, -1);
		ep->InvalidMe();
	}
	SDL_UnlockMutex(doQueMutex);
}


void LOImageModule::ExportQuequ2(std::unordered_map<int, LOLayerInfoCacheIndex*> *map) {
	int ii = sizeof(FunctionInterface);
}



//捕获SDL事件，将对指定的事件进行处理
void LOImageModule::CaptureEvents(SDL_Event *event) {
	LOEvent1 *catMsg = NULL;
	LOEventParamBtnRef *param;
	LOLayer *layer;

	switch (event->type)
	{
	case SDL_MOUSEMOTION:
		//更新鼠标位置
		if (!TranzMousePos(event->motion.x, event->motion.y)) break;
		//鼠标在窗口内移动事件，将在这里检查按钮
		//只选择纯正的按钮事件
		catMsg = G_GetEvent(LOEvent1::EVENT_CATCH_BTN, LOEvent1::EVENT_CATCH_BTN);

		if (catMsg) catMsg->closeEdit(); //这里只是确认有按钮事件，按钮不在这里编辑
		if (catMsg && SDL_TryLockMutex(btnQueMutex) == 0) {
			LOLayer *layer = FindLayerInBtnQuePosition(mouseXY[0], mouseXY[1]);
			//当前激活的按钮不是同一个，或者没有激活按钮，重置上一次激活的按钮状态
			if (lastActiveLayer) {
				lastActiveLayer->HideBtnMe();
				if (lastActiveLayer->isExbtn()) {
					//if (!layer || !layer->isExbtn())exbtn_d_HasRun = false; //离开了复合按钮
					if (lastActiveLayer != layer) lastActiveLayer->exbtnHasRun = false; //离开了按钮
				}
				lastActiveLayer = NULL;
			}
			if (layer) {
				//if (!layer->exbtnHasRun) RunSpString(layer->curInfo->btnStr, NULL);
				layer->exbtnHasRun = true;
				layer->ShowBtnMe();
				lastActiveLayer = layer;
			}

			//if (!exbtn_d_HasRun && exbtn_count > 0) {
			//	exbtn_d_HasRun = true;
			//	RunSpString(&exbtn_dStr, NULL);
			//}

			SDL_UnlockMutex(btnQueMutex);
		}

		break;

		//鼠标点击完成
	case SDL_MOUSEBUTTONUP:
		//更新鼠标位置
		if (!TranzMousePos(event->button.x, event->button.y))break;

		//鼠标左点击
		if (event->button.button == SDL_BUTTON_LEFT) {
			//最优响应的是print事件
			catMsg = G_GetEvent(LOEvent1::EVENT_WAIT_PRINT);
			if (catMsg) {
				param = (LOEventParamBtnRef*)catMsg->param;
				ContinueEffect((LOEffect *)param->ptr1, 1000.0 * 1000.0);
				catMsg->FinishMe();
				break;
			}
			//第二个响应的是文字显示事件，则完成整个显示，同时忽略点击的其他作用
			catMsg = G_GetEvent(LOEvent1::EVENT_TEXT_ACTION, FunctionInterface::LAYER_TEXT_WORKING);
			//catMsg = LonsEvent.GetEvent(LOEvent::MSG_Wait_Text, LOEvent::PARAM_TEXT_WORKING);
			if (catMsg) {
				catMsg->closeEdit();
				CutDialogueAction();
				break;
			}

			//左键事件
			catMsg = G_GetEvent(LOEvent1::EVENT_CATCH_BTN);
			if (catMsg) {
				switch (catMsg->eventID){
				case SCRIPTER_EVENT_DALAY:
					catMsg->InvalidMe();  //延迟事件直接失效，脚本不再阻塞
					break;
				case SCRIPTER_EVENT_LEFTCLICK:
					catMsg->InvalidMe(); //无需获取具体按钮的，比如click lrclick直接无效事件
					break;
				case LOEvent1::EVENT_CATCH_BTN:
					layer = FindLayerInBtnQuePosition(mouseXY[0], mouseXY[1]);
					if (layer) catMsg->value = layer->curInfo->btnValue;
					else catMsg->value = 0; //没有点中任何按钮
					LeaveCatchBtn(catMsg);
					break;
				default:
					printf("unkown left click event %d!\n", catMsg->eventID);
				}
			}
		}
		else { //鼠标右点击
			LOEvent1 *catMsg = G_GetEvent(LOEvent1::EVENT_CATCH_BTN);
			if (catMsg)
			{
				switch (catMsg->eventID)
				{
				case LOEvent1::EVENT_CATCH_BTN:
					catMsg->value = -1;
					LeaveCatchBtn(catMsg);
					break;
				case SCRIPTER_EVENT_LEFTCLICK:
					catMsg->closeEdit();
					break;  //右键不响应左键信息
				default:
					catMsg->InvalidMe();
					break;
				}
			}
		}
		break;
	case SDL_MOUSEBUTTONDOWN:
		break;
	default:
		break;
	}
}

//转换鼠标位置，超出视口位置等于事件没有发生
bool LOImageModule::TranzMousePos(int xx, int yy) {
	xx -= G_viewRect.x;
	yy -= G_viewRect.y;
	if (xx < 0 || yy < 0) return false;
	if (xx > G_viewRect.w || yy > G_viewRect.h) return false;
	mouseXY[0] = 1.0 / G_gameScaleX * xx;
	mouseXY[1] = 1.0 / G_gameScaleY * yy;
	return true;
	//LONS::printInfo("%d,%d,%d,%d\n", mouseX, mouseY,xx,yy);
}

//立刻按钮捕获模式
void LOImageModule::LeaveCatchBtn(LOEvent1 *msg) {
	lastActiveLayer = nullptr;
	//赋值交给脚本线程执行，避免出现线程竞争
	msg->FinishMe();
}


void LOImageModule::ClearDialogText(char flag) {
	int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
	int fullid = GetFullID(LOLayer::LAYER_DIALOG, ids);
	if (flag == '\\') {
		dialogText.clear();
		LOLayerInfo *info = GetInfoNewAndFreeOld(fullid, "_lons");
		info->layerControl |= LOLayerInfo::CON_DELLAYER;
	}
}


//立即完成对话文字
void LOImageModule::CutDialogueAction() {
	int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
	LOLayer *layer = FindLayerInBase(LOLayer::LAYER_DIALOG, ids);
	if (layer) {
		LOAnimation *aib = layer->GetAnimation(LOAnimation::ANIM_TEXT);
		if (aib) {
			LOAnimationText *ai = (LOAnimationText*)(aib);
			layer->DoTextAnima(layer->curInfo, ai, ai->lastTime + 0xfffff);
		}
	}
}


LOSurface* LOImageModule::ScreenShot(SDL_Rect *srcRect, SDL_Rect *dstRect) {
	LOEvent1 *e = new LOEvent1(LOEvent1::EVENT_PREPARE_EFFECT, PARAM_SCREENSHORT);
	auto *param = new LOEventParamBtnRef;
	param->ptr1 = (intptr_t)srcRect;
	param->ptr2 = (intptr_t)dstRect;
	e->SetValuePtr(param);

	G_SendEvent(e);
	e->waitEvent(1, -1);

	LOSurface *su = (LOSurface*)param->ptr2;
	e->InvalidMe();
	return su;
}

void LOImageModule::ScreenShotCountinue(LOEvent1 *e) {
	if (!e->value) return;
	LOEventParamBtnRef *param = (LOEventParamBtnRef*)e->value;
	SDL_Rect *rect = (SDL_Rect*)param->ptr1;
	SDL_Rect *dst = (SDL_Rect*)param->ptr2;
	if (!rect || rect->w == 0 || rect->h == 0) return;
	//核算截图的位置
	int xx, yy, ww, hh;
	xx = G_gameScaleX * rect->x; yy = G_gameScaleY * rect->y;
	ww = G_gameScaleX * rect->w; hh = G_gameScaleY * rect->h;
	if (xx < 0) xx = 0;
	if (yy < 0) yy = 0;
	if (ww > G_viewRect.w || ww < 0) ww = G_viewRect.w;
	if (hh > G_viewRect.h || hh < 0) hh = G_viewRect.h;
	SDL_Rect src;
	src.x = xx; src.y = yy;
	src.w = ww; src.h = hh;

	//把画面帧缩放转移到新的buf中
	SDL_Texture *tex = SDL_CreateTexture(render, G_Texture_format, SDL_TEXTUREACCESS_TARGET, dst->w, dst->h);
	SDL_SetRenderTarget(render, tex);
	SDL_RenderClear(render);
	SDL_RenderCopy(render, effectTex, &src, dst);

	//转写到surface中，注意如果是一个比较大的图像尺寸，会比较耗时
	LOSurface *su = new LOSurface(dst->w, dst->h, 32, G_Texture_format);
	SDL_Surface *su_t = su->GetSurface();
	SDL_LockSurface(su_t);
	int cks = SDL_RenderReadPixels(render, dst, G_Texture_format, su_t->pixels, su_t->pitch);
	SDL_UnlockSurface(su_t);
	//SDL_SaveBMP(su_t, "gggg.bmp");

	//还原渲染器状态
	SDL_SetRenderTarget(render, NULL);
	SDL_DestroyTexture(tex);

	param->ptr2 = (intptr_t)su;  //替换掉dst的指针
	e->FinishMe();
}