/*
//图像模块命令实现
*/
#include "../etc/LOString.h"
#include "LOImageModule.h"

int LOImageModule::lspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 362) {
	//	int debugbreak = 1;
	//}


	bool visiable = !reader->isName("lsph");
	int ids[] = { 0,255,255 };
	int fixpos = 0;

	ids[0] = reader->GetParamInt(0);
	if (reader->isName("lspc")) {
		ids[1] = reader->GetParamInt(1);
		fixpos = 1;
	}

	LOString tag = reader->GetParamStr(1 + fixpos);
	int xx = reader->GetParamInt(2);
	int yy = reader->GetParamInt(3);
	int alpha = -1;
	if (reader->GetParamCount() > 4 + fixpos) alpha = reader->GetParamInt(4 + fixpos);
	
	reader->ExpandStr(tag);
	LeveTextDisplayMode();
		//已经在队列里的需要释放
	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_SPRINT, ids), reader->GetPrintName());
	loadSpCore(info, tag, xx, yy, alpha);
	info->visiable = visiable;
	info->layerControl |= LOLayerInfo::CON_UPVISIABLE;
	return RET_CONTINUE;
}

int LOImageModule::lsp2Command(FunctionInterface *reader) {
	int ids[] = { reader->GetParamInt(0),255,255 };
	LOString tag = reader->GetParamStr(1);
	int alpha = -1;
	if (reader->GetParamCount() > 7) alpha = reader->GetParamInt(7);

	LeveTextDisplayMode();

	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_SPRINTEX, ids), reader->GetPrintName());
	loadSpCore(info, tag, reader->GetParamInt(2), reader->GetParamInt(3), alpha);

	if (info->texture) {
		int w, h;
		info->centerX = info->texture->baseW() / 2;
		info->centerY = info->texture->baseH() / 2;
	}

	info->scaleX = (double)reader->GetParamInt(4) / 100;
	info->scaleY = (double)reader->GetParamInt(5) / 100;
	info->showType |= LOLayerInfo::SHOW_SCALE;
	
	info->rotate = (double)reader->GetParamInt(6);
	if (fabs(info->rotate - 0.0) > 0.001) {
		info->showType |= LOLayerInfo::SHOW_ROTATE;
	}

	std::string cmd(reader->GetCmdChar());
	if (cmd.length() > 3) {
		if (cmd[3] == 'h') info->visiable = false;
		std::string eflag = cmd.substr(cmd.length() - 3, 3);
		if (eflag == "add" && info->texture) {
			info->texture->setBlendModel(SDL_BLENDMODE_ADD);
		}
		else if (eflag == "sub" && info->texture) {
			//info->texture->setBlendModel(SDL_BLENDMODE_INVALID);
			//注意并不是所有的渲染器都能正确实现
			//SDL_BlendMode bmodel = SDL_ComposeCustomBlendMode(SDL_BLENDFACTOR_SRC_COLOR, SDL_BLENDFACTOR_ONE_MINUS_DST_COLOR,
				//SDL_BLENDOPERATION_ADD, SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD);
			//info->texture->setBlendModel(bmodel);
			SimpleError("LONS not support [lsp2sub], because some renderers do not support!\n");
		}
	}

	return RET_CONTINUE;
}


int LOImageModule::printCommand(FunctionInterface *reader) {
	return printStack(reader, 0);
}


int LOImageModule::printStack(FunctionInterface *reader, int fix) {

	LeveTextDisplayMode();

	int no = reader->GetParamInt(fix);
	int paramCount = reader->GetParamCount() - fix;

	LOEffect *ef = GetEffect(no);
	if (!ef && no > 1) {
		SDL_Log("effect %d not find!", ef->id);
		ef = new LOEffect;
		ef->nseffID = 10;
		ef->time = 100;
	}

	if (ef) {
		if (paramCount > 1) ef->time = reader->GetParamInt(fix + 1);
		if (paramCount > 2) ef->mask = reader->GetParamStr(fix + 2);
	}

	ExportQuequ(reader->GetPrintName(), ef, true);
	if (ef) delete ef;
	return RET_CONTINUE;
}


int LOImageModule::bgCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 502) {
	//	int debugbreak = 0;
	//}
	LeveTextDisplayMode();

	int ids[] = { 5,255,255 };
	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_BG, ids), reader->GetPrintName());
	LOString tag = reader->GetParamStr(0);

	LOString tmp = tag.toLower();
	if (tmp == "white") tag = StringFormat(64, ":c;>%d,%d#ffffff", G_gameWidth, G_gameHeight);
	else if (tmp == "black") tag = StringFormat(64, ":c;>%d,%d#000000", G_gameWidth, G_gameHeight);
	else if (tmp.at(0) == '#') tag = StringFormat(64, ":c;>%d,%d%s", G_gameWidth, G_gameHeight, tag.c_str());
	else if (tmp.at(0) != ':') tag = ":c;" + tag;

	loadSpCore(info, tag, 0, 0, -1);
	if (reader->GetParamCount() > 1) return printStack(reader, 1);
	else return RET_CONTINUE;
}


int LOImageModule::cspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 422) {
	//	int debugbreak = -1;
	//}

	LeveTextDisplayMode();

	int ids[] = { reader->GetParamInt(0),255,255 };
	if (reader->isName("csp2")) CspCore(LOLayer::LAYER_SPRINTEX, ids, reader->GetPrintName());
	else CspCore(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	return RET_CONTINUE;
}

void LOImageModule::CspCore(LOLayer::SysLayerType sptype, int *cid, const char *print_name) {
	int index = GetInfoCacheFromName(print_name);
	auto map = mapList.at(index);
	if (cid[0] < 0) {
		//释放掉队列中的任务
		for (auto iter = map->begin(); iter != map->end(); iter++) {
			NotUseInfo(iter->second);
		}
		map->clear();
		//根据现有的图层新建删除队列
		LOLayer *layer = lonsLayers[sptype];
		for (auto iter = layer->childs.begin(); iter != layer->childs.end(); iter++) {
			LOLayerInfo *lyrinfo = (*iter).second->curInfo;
			LOLayerInfo *info = GetInfoNew(lyrinfo->fullid, print_name);
			info->layerControl |= LOLayerInfo::CON_DELLAYER;
		}
	}
	else {
		int fullid = GetFullID(sptype, cid);
		auto iter = map->find(fullid);
		if (iter != map->end()) {
			NotUseInfo(iter->second);
			map->erase(fullid);
		}
		LOLayer *layer = FindLayerInBase(GetFullID(sptype, cid));
		if (layer) {
			LOLayerInfo *info = GetInfoNew(layer->GetFullid(), print_name);
			info->layerControl |= LOLayerInfo::CON_DELLAYER;
		}
	}
}

int LOImageModule::mspCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 571) {
	//	int debugbreak = 1;
	//}

	LOLayer::SysLayerType sptype = LOLayer::LAYER_SPRINT;
	if (reader->isName("msp2") || reader->isName("amsp2")) sptype = LOLayer::LAYER_SPRINTEX;
	bool addmode = false;
	if (reader->isName("msp") || reader->isName("msp2")) addmode = true;
	int ids[] = { reader->GetParamInt(0),255,255 };

	LeveTextDisplayMode();
	SDL_LockMutex(layerQueMutex);
	//万恶的队列操作，麻烦
	LOLayerInfo *doinfo = LayerAvailable(sptype, ids, reader->GetPrintName());
	if (!doinfo) {
		SDL_UnlockMutex(layerQueMutex);
		return RET_CONTINUE;
	}
	LOLayerInfo *info = NULL;
	if (addmode) info = LayerInfomation(sptype, ids, reader->GetPrintName());
	SDL_UnlockMutex(layerQueMutex);

	//位置或者中心位置
	if (addmode) {
		doinfo->offsetX = reader->GetParamInt(1) + info->offsetX;
		doinfo->offsetY = reader->GetParamInt(2) + info->offsetY;
	}
	else {
		doinfo->offsetX = reader->GetParamInt(1);
		doinfo->offsetY = reader->GetParamInt(2);
	}
	doinfo->layerControl |= LOLayerInfo::CON_UPPOS;
	//sp2操作
	if (sptype == LOLayer::LAYER_SPRINTEX) {
		if (addmode) {
			doinfo->centerX = info->centerX;
			doinfo->centerY = info->centerY;
			doinfo->scaleX = ((double)reader->GetParamInt(3) / 100) + info->scaleX;
			doinfo->scaleY = ((double)reader->GetParamInt(4) / 100) + info->scaleY;
			doinfo->rotate = info->rotate + reader->GetParamInt(5);
		}
		else {
			doinfo->scaleX = ((double)reader->GetParamInt(3) / 100);
			doinfo->scaleY = ((double)reader->GetParamInt(4) / 100);
			doinfo->rotate = reader->GetParamInt(5);
		}
		doinfo->layerControl |= LOLayerInfo::CON_UPPOS2;
	}
	//透明度
	int fixp = 3;
	if (sptype == LOLayer::LAYER_SPRINTEX) fixp = 6;
	if (reader->GetParamCount() > fixp) {
		if (addmode) doinfo->AddAlpha(reader->GetParamInt(fixp));
		else doinfo->alpha = reader->GetParamInt(fixp);
		doinfo->layerControl |= LOLayerInfo::CON_UPAPLHA;
	}

	if (info) delete info;
	return RET_CONTINUE;
}


int LOImageModule::cellCommand(FunctionInterface *reader) {
	int ids[] = { reader->GetParamInt(0),255,255 };
	LOLayerInfo *info = LayerAvailable(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		info->layerControl |= LOLayerInfo::CON_UPCELL;
		info->cellNum = reader->GetParamInt(1);
	}
	return RET_CONTINUE;
}

int LOImageModule::humanzCommand(FunctionInterface *reader) {
	int no = reader->GetParamInt(0);
	if (no < 1 || no > 999) {
		SDL_Log("[humanz] command is just for 1~999, buf it's %d!", no);
		return RET_WARNING;
	}
	else {
		z_order = no;
		return RET_CONTINUE;
	}
}

int LOImageModule::strspCommand(FunctionInterface *reader) {
	int ids[] = { reader->GetParamInt(0), 255, 255 };
	LOString tag = "*S;" + reader->GetParamStr(1);
	LOFontWindow ww = winFont;
	ww.topx = reader->GetParamInt(2);
	ww.topy = reader->GetParamInt(3);
	ww.xcount = reader->GetParamInt(4);
	ww.ycount = reader->GetParamInt(5);
	ww.xsize = reader->GetParamInt(6);
	ww.ysize = reader->GetParamInt(7);
	ww.xspace = reader->GetParamInt(8);
	ww.yspace = reader->GetParamInt(9);
	ww.isbold = reader->GetParamInt(10);
	ww.isshaded = reader->GetParamInt(11);

	int colorCount = reader->GetParamCount() - 12;

	LeveTextDisplayMode();

	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_SPRINT, ids), reader->GetPrintName());
	SDL_Color *cc = new SDL_Color[3];
	info->maskName = (LOString*)(&ww);
	info->btnStr = (LOString*)cc;
	info->btnValue = colorCount;

	if (colorCount > 0) {
		for (int ii = 0; ii < colorCount; ii++) {
			int colorint = reader->GetParamColor(12 + ii);
			cc[ii].r = (colorint >> 16) & 0xff;
			cc[ii].g = (colorint >> 8) & 0xff;
			cc[ii].b = colorint & 0xff;
		}
	}
	else {
		cc[0] = spFont.fontColor;
		info->btnValue = 1;
	}

	loadSpCore(info, tag, ww.topx, ww.topy, -1);
	if (reader->isName("strsph")) {
		info->visiable = false;
		info->layerControl |= LOLayerInfo::CON_UPVISIABLE;
	}

	if (cc) delete[] cc;
	return RET_CONTINUE;
}

int LOImageModule::transmodeCommand(FunctionInterface *reader) {
	LOString keyword = reader->GetParamStr(0).toLower();
	if (keyword == "leftup") trans_mode = LOLayerInfo::TRANS_TOPLEFT;
	else if (keyword == "copy") trans_mode = LOLayerInfo::TRANS_COPY;
	else if (keyword == "alpha") trans_mode = LOLayerInfo::TRANS_ALPHA;
	else if (keyword == "righttup")trans_mode = LOLayerInfo::TRANS_TOPRIGHT;
	return RET_CONTINUE;
}

int LOImageModule::bgcopyCommand(FunctionInterface *reader) {
	//SDL_LockMutex(doQueMutex);
	//LOEvent *e = new LOEvent(LOEvent::MSG_Prepare_Effect);
	//e->saveParamInt(0, PARAM_BGCOPY);

	//LonsEvent.PostEvent(e);
	//e->waitEvent(1.0, -1.0);

	int ids[] = { 5,255,255 };
	LOLayerInfo *info = GetInfoNewAndFreeOld(GetFullID(LOLayer::LAYER_BG, ids), "_lons");
	assert("this function is not finish!");
	//info->texture = new LOtexture;
	//info->texture->tx = (SDL_Texture*)e->movePtr(true, 0);  //这个纹理后续还要继续使用的
	//info->texture->w = G_viewRect.w;
	//info->texture->h = G_viewRect.h;
	//info->fileName = new LOString;
	//RandomFileName(info->fileName, 'c');
	//LOString s = GetTextrueName(info->fileName, info->texFlag);
	//SetNameTextrue(&s, info->texture);
	info->layerControl |= LOLayerInfo::CON_UPFILE;

	if (IsGameScale()) {
		info->scaleX = 1.0 / G_gameScaleX;
		info->scaleY = 1.0 / G_gameScaleY;
		info->showType |= LOLayerInfo::SHOW_SCALE;
	}

	ExportQuequ("_lons", NULL, true);
	//SDL_UnlockMutex(doQueMutex);
	return RET_CONTINUE;
}

int LOImageModule::spfontCommand(FunctionInterface *reader) {
	LOString s = reader->GetParamStr(0).toLower();
	if (s == "clear") {
		spFont.Reset();
	}
	else {
		spFont.fontName = reader->GetParamStr(0);
		spFont.xsize = reader->GetParamInt(1);
		spFont.ysize = reader->GetParamInt(2);
		spFont.xspace = reader->GetParamInt(3);
		spFont.yspace = reader->GetParamInt(4);
		spFont.isbold = reader->GetParamInt(5);
		spFont.isshaded = reader->GetParamInt(6);
	}
	return RET_CONTINUE;
}

int LOImageModule::effectskipCommand(FunctionInterface *reader) {
	effectSkipFlag = reader->GetParamInt(0);
	return RET_CONTINUE;
}

int LOImageModule::getspmodeCommand(FunctionInterface *reader) {
	ONSVariableRef *v = reader->GetParamRef(0);
	int ids[] = { reader->GetParamInt(1), 255, 255 };
	int visiable = 0;

	LOLayerInfo *info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		visiable = info->visiable;
		delete info;
	}

	v->SetValue((double)visiable);
	return RET_CONTINUE;
}

int LOImageModule::getspsizeCommand(FunctionInterface *reader) {
	//if (reader->GetCurrentLine() == 423) {
	//	int debugbreak = 1;
	//}

	int ids[] = { reader->GetParamInt(0), 255, 255 };
	ONSVariableRef *v1 = reader->GetParamRef(1);
	ONSVariableRef *v2 = reader->GetParamRef(2);
	ONSVariableRef *v3 = NULL;
	if (reader->GetParamCount() > 3) v3 = reader->GetParamRef(3);

	int ww, hh, cell;
	ww = hh = 0;
	cell = 0;
	LOLayerInfo *info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		info->GetSize(ww, hh, cell);
		delete info;
	}

	v1->SetValue((double)ww);
	v2->SetValue((double)hh);
	if (v3) v3->SetValue((double)cell);

	return RET_CONTINUE;
}

int LOImageModule::getspposCommand(FunctionInterface *reader) {
	int ids[] = { reader->GetParamInt(0), 255, 255 };
	int xx, yy;
	xx = yy = 0;
	ONSVariableRef *v1 = reader->GetParamRef(1);
	ONSVariableRef *v2 = reader->GetParamRef(2);
	LOLayerInfo *info = NULL;
	info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		xx = info->offsetX;
		yy = info->offsetY;
		delete info;
	}

	v1->SetValue((double)xx);
	v2->SetValue((double)yy);
	return RET_CONTINUE;
}

int LOImageModule::getspalphaCommand(FunctionInterface *reader) {
	int ids[] = { reader->GetParamInt(0), 255, 255 };
	double val = 0.0;
	LOLayerInfo *info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		val = info->alpha;
		if (val < 0 || val > 255) val = 255;
		delete info;
	}

	ONSVariableRef *v = reader->GetParamRef(1);
	v->SetValue(val);
	return RET_CONTINUE;
}

int LOImageModule::getspposexCommand(FunctionInterface *reader) {
	ONSVariableRef *v[5];
	double val[5];
	int ids[] = { reader->GetParamInt(0), 255,255 };
	for (int ii = 0; ii < 5; ii++) {
		if (ii < reader->GetParamCount()) v[ii] = reader->GetParamRef(ii + 1);
		else v[ii] = NULL;
		val[ii] = 0.0;
	}

	val[2] = val[3] = 100.0;
	LOLayerInfo *info = NULL;
	if(reader->isName("getspposex")) info = LayerInfomation(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	else info = LayerInfomation(LOLayer::LAYER_SPRINTEX, ids, reader->GetPrintName());

	if (info) {
		val[0] = info->offsetX;
		val[1] = info->offsetY;
		val[2] = info->scaleX * 100;
		val[3] = info->scaleY * 100;
		val[4] = info->rotate;
	}

	for (int ii = 0; ii < 5; ii++) {
		if (v[ii]) v[ii]->SetValue(val[ii]);
	}

	return RET_CONTINUE;
}

int LOImageModule::vspCommand(FunctionInterface *reader) {
	int ids[] = { reader->GetParamInt(0), 255,255 };
	LOLayer::SysLayerType sptype = LOLayer::LAYER_SPRINT;
	if (reader->isName("vsp2")) sptype = LOLayer::LAYER_SPRINTEX;

	LeveTextDisplayMode();

	VspCore(sptype, ids, reader->GetPrintName(), reader->GetParamInt(1));
	return RET_CONTINUE;
}

void LOImageModule::VspCore(LOLayer::SysLayerType sptype, int *cid, const char *print_name, int vals) {
	LOLayerInfo *info = LayerAvailable(GetFullID(sptype, cid), print_name);
	if (info) {
		info->visiable = vals;
		info->layerControl |= LOLayerInfo::CON_UPVISIABLE;
	}
}


int LOImageModule::allspCommand(FunctionInterface *reader) {
	std::vector<int> *list_t = allSpList;
	auto sptype = LOLayer::LAYER_SPRINT;
	if (reader->isName("allsp2hide") || reader->isName("allsp2resume")) {
		sptype = LOLayer::LAYER_SPRINTEX;
		list_t = allSpList2;
	}

	if (!list_t) list_t = new std::vector<int>;
	//bool visiable = true;

	if (reader->isName("allsphide") || reader->isName("allsp2hide")) { //hide
		list_t->clear();
		//visiable = false;
		//队列中的新图像，和调整显示模式的任务被置为隐藏
		LOStack<LOLayerInfoCacheIndex> *list = FilterCacheQue(reader->GetPrintName(), sptype, LOLayerInfo::CON_UPFILE | LOLayerInfo::CON_UPVISIABLE);
		for (int ii = 0; ii < list->size(); ii++) {
			LOLayerInfo *info = &list->at(ii)->info;
			info->layerControl |= LOLayerInfo::CON_UPVISIABLE;
			info->visiable = false;
			list_t->push_back(info->fullid);
		}
		delete list;

		//显示中的图像被隐藏
		LOLayer *layer = lonsLayers[sptype];
		for (auto iter = layer->childs.begin(); iter != layer->childs.end(); iter++) {
			LOLayer *lyr = iter->second;
			LOLayerInfo *info = GetInfoNewAndNoFreeOld(lyr->GetFullid(), reader->GetPrintName());
			info->layerControl |= LOLayerInfo::CON_UPVISIABLE;
			info->visiable = false;
			list_t->push_back(info->fullid);
		}
	}
	else { //show
		int ids[] = { 1023,255,255 };
		for (int ii = 0; ii < list_t->size(); ii++) {
			LOLayerInfo *info = GetInfoNewAndNoFreeOld(list_t->at(ii), reader->GetPrintName());
			info->layerControl |= LOLayerInfo::CON_UPVISIABLE;
			info->visiable = true;
		}
	}

	if(list_t->size() > 0 ) ExportQuequ(reader->GetPrintName(), NULL, true);
	return RET_CONTINUE;
}

int LOImageModule::windowbackCommand(FunctionInterface *reader) {
	winbackMode = true;
	return RET_CONTINUE;
}

int LOImageModule::textCommand(FunctionInterface *reader) {
	LOString text = reader->GetParamStr(0);
	char currentEndFlag = reader->GetParamInt(1) & 0xff;
	dialogText.append(text);
	LoadDialogText(&dialogText, pageEndFlag[1] != '\\');
	pageEndFlag[0] = currentEndFlag ;
	pageEndFlag[1] = '/';
	//结尾符号进入参数传递
	LOEvent1 *e = new LOEvent1(LOEvent1::EVENT_TEXT_ACTION, FunctionInterface::LAYER_TEXT_PREPARE);
	e->value = pageEndFlag[0];
	e->SetUseCount(2);

	G_SendEvent(e);                         //进入layer初始化
	reader->blocksEvent.SendToSlot(e);      //阻塞脚本

	//LOEvent *msg = new LOEvent(LOEvent::MSG_Wait_Text);
	//msg->saveParamInt(0, LOEvent::PARAM_TEXT_PREPARE);
	//msg->saveParamInt(1, pageEndFlag[0]);
	//LonsEvent.PostEvent(msg);
	//reader->AddNextEvent((intptr_t)msg);   //脚本解析转入事件等待
	EnterTextDisplayMode(true);  //will display here

	//if (currentEndFlag != '/') Textgosub(reader); // textgosub将在文字动画完成后调用

	//after show text
	//if (fontManager->rubySize[2] == LOFontManager::RUBY_LINE) fontManager->rubySize[2] = LOFontManager::RUBY_OFF;
	return RET_CONTINUE;
}

int LOImageModule::effectCommand(FunctionInterface *reader) {
	//if (NotAtDefineError("effect"))return RET_ERROR;
	int no = reader->GetParamInt(0);
	if (reader->isName("effect")) {
		if (no >= 1 && no <= 18) {
			SimpleError("[effect] command id can't use 1~18,that's ONScripter system used!");
			return RET_ERROR;
		}
	}
	LOEffect *ef = new LOEffect;
	ef->id = no;
	ef->nseffID = reader->GetParamInt(1);
	if (reader->GetParamCount() > 2) ef->time = reader->GetParamInt(2);
	else ef->time = 10;   //少量的时间来避免潜在的错误
	if (reader->GetParamCount() > 3) ef->mask = reader->GetParamStr(3);


	auto iter = effectMap.find(no);
	if (iter != effectMap.end()) delete iter->second;
	effectMap[no] = ef;
	return RET_CONTINUE;
}

int LOImageModule::windoweffectCommand(FunctionInterface *reader) {
	if (winEffect) delete winEffect;
	winEffect = GetEffect(reader->GetParamInt(0));
	if (winEffect) {
		if (reader->GetParamCount() > 1) winEffect->time = reader->GetParamInt(1);
		if (reader->GetParamCount() > 2) winEffect->mask = reader->GetParamStr(2);
	}
	return RET_CONTINUE;
}


int LOImageModule::btnwaitCommand(FunctionInterface *reader) {
	if (reader->isName("btnwait") || reader->isName("btnwait2")) LeveTextDisplayMode();

	if (textbtnFlag && reader->isName("textbtnwait")) { //注册文字按钮
		int ids[] = { LOLayer::IDEX_DIALOG_TEXT,255,255 };
		LOLayerInfo *info = LayerAvailable(LOLayer::LAYER_DIALOG, ids, reader->GetPrintName());
		if (info) {
			info->layerControl |= LOLayerInfo::CON_UPBTN;
			info->btnValue = textbtnValue;
		}
	}

	ExportQuequ(reader->GetPrintName(), nullptr, true);
	ONSVariableRef *v1 = reader->GetParamRef(0);

	//参数
	auto *param = new LOEventParamBtnRef;
	param->ptr1 = SDL_GetTicks(); //时间戳
	//btntime的设定只会在btndef时清除
	if (btnOverTime > 0) param->ptr2 = btnOverTime;
	else param->ptr2 = 0xfffffff;  //74小时
	param->ref = new ONSVariableRef;
	param->ref->CopyFrom(v1);

	LOEvent1 *e = new LOEvent1(LOEvent1::EVENT_CATCH_BTN, param);
	//按钮需要响应按钮、脚本线程阻塞、声音播放完毕事件
	if (btnUseSeOver) {
		e->SetUseCount(3);
		G_SendEventMulit(e, LOEvent1::EVENT_SEZERO_FINISH);
	}
	else e->SetUseCount(2);
	G_SendEvent(e);   //按钮队列
	reader->blocksEvent.SendToSlot(e);  //线程阻塞

	return RET_CONTINUE;
}


int LOImageModule::spbtnCommand(FunctionInterface *reader) {
	int ids[] = { reader->GetParamInt(0),255,255 };
	LOLayerInfo *info = LayerAvailable(LOLayer::LAYER_SPRINT, ids, reader->GetPrintName());
	if (info) {
		info->btnValue = reader->GetParamInt(1);
		info->layerControl |= LOLayerInfo::CON_UPBTN;
	}
	return RET_CONTINUE;
}


int LOImageModule::texecCommand(FunctionInterface *reader) {
//texthide/textshow 只对文字进行操作，不影响对话窗口
//texton/textoff 对整个对话窗口进行操作
//textclear 清除文字，不影响对话框
//texec '\'作为结尾符时立即清除文字，"@"作为结尾符时不清除文字
//对话框受erasetextwindow影响，当erasetextwindow有效时，leveatextmode对话框消失

	//
	pageEndFlag[1] = pageEndFlag[0];
	if (pageEndFlag[1] == '\\') {
		ClearDialogText(pageEndFlag[0]);
		DialogWindowSet(0, -1, -1);
		dialogDisplayMode = DISPLAY_MODE_NORMAL;
	}
	return RET_CONTINUE;
}

int LOImageModule::textonCommand(FunctionInterface *reader) {
	if (reader->isName("texton")) {
		DialogWindowSet(1, 1, 1);
		dialogDisplayMode = DISPLAY_MODE_TEXT;
	}
	else if (reader->isName("textclear")) {
		pageEndFlag[1] = pageEndFlag[0];
		ClearDialogText('\\');
		DialogWindowSet(0, -1, -1);
		dialogDisplayMode = DISPLAY_MODE_NORMAL;
	}
	else { //textoff
		pageEndFlag[1] = pageEndFlag[0];
		ClearDialogText(pageEndFlag[0]);
		DialogWindowSet(0, 0, 0);
		dialogDisplayMode = DISPLAY_MODE_NORMAL;
	}
	return RET_CONTINUE;
}

int LOImageModule::textbtnsetCommand(FunctionInterface *reader) {
	if (reader->GetParamCount() == 1) {
		textbtnFlag = true;
		textbtnValue = reader->GetParamInt(0);
	}
	else textbtnFlag = false;
	return RET_CONTINUE;
}


int LOImageModule::setwindowCommand(FunctionInterface *reader) {
	winFont.topx = reader->GetParamInt(0);
	winFont.topy = reader->GetParamInt(1);
	winFont.xcount = reader->GetParamInt(2);
	winFont.ycount = reader->GetParamInt(3);
	winFont.xsize = reader->GetParamInt(4);
	winFont.ysize = reader->GetParamInt(5);
	winFont.xspace = reader->GetParamInt(6);
	winFont.yspace = reader->GetParamInt(7);
	G_textspeed = reader->GetParamInt(8);
	winFont.isbold = reader->GetParamInt(9);
	winFont.isshaded = reader->GetParamInt(10);
	winstr = reader->GetParamStr(11);
	winoff.x = reader->GetParamInt(12);
	winoff.y = reader->GetParamInt(13);

	if (winstr.length() > 0 && winstr.at(0) == '#') {
		winoff.w = reader->GetParamInt(14);
		winoff.h = reader->GetParamInt(15);
		winstr = StringFormat(64, ">%d,%d,%s", winoff.w, winoff.h, winstr.c_str());
	}

	return RET_CONTINUE;
}

int LOImageModule::setwindow2Command(FunctionInterface *reader) {
	winstr = reader->GetParamStr(11);
	if (winstr.length() > 0 && winstr.at(0) == '#') {
		if (winoff.w < 1) winoff.w = 1;
		if (winoff.h < 1) winoff.h = 1;  //safe value
		winstr = StringFormat(64, ">%d,%d,%s", winoff.w, winoff.h, winstr.c_str() + 1);
	}
	return RET_CONTINUE;
}


int LOImageModule::clickCommand(FunctionInterface *reader) {
	LOEvent1 *e ;
	if (reader->isName("lrclick")) e = new LOEvent1(FunctionInterface::SCRIPTER_EVENT_LEFTCLICK, (int64_t)0);
	else e = new LOEvent1(FunctionInterface::SCRIPTER_EVENT_CLICK, (int64_t)0);  //左键右键都可以
	G_SendEventMulit(e, LOEvent1::EVENT_CATCH_BTN);																	 //LonsEvent.PostEvent(e);
	//reader->AddNextEvent((intptr_t)e);
	return RET_CONTINUE;
}

int LOImageModule::erasetextwindowCommand(FunctionInterface *reader) {
	winEraseFlag = reader->GetParamInt(0);
	return RET_CONTINUE;
}

int LOImageModule::getmouseposCommand(FunctionInterface *reader) {
	ONSVariableRef *v1 = reader->GetParamRef(0);
	ONSVariableRef *v2 = reader->GetParamRef(1);
	v1->SetValue((double)mouseXY[0]);
	v1->SetValue((double)mouseXY[1]);
	return RET_CONTINUE;
}


int LOImageModule::btndefCommand(FunctionInterface *reader) {
	//btndef会清除上一次的btndef定义和btn定义
	LOString tag = reader->GetParamStr(0);

	RemoveBtn(-1);  //所有的按钮定义都会被清除

	//无论如何btn的系统层都将被清除
	int ids[] = { LOLayer::IDEX_NSSYS_BTN,255,255 };
	CspCore(LOLayer::LAYER_NSSYS, ids, "_lons");
	ExportQuequ("_lons", nullptr, true);
	//All button related settings are cleared
	btndefStr.clear();
	BtndefCount = 0;
	exbtn_count = 0;
	btnOverTime = 0;
	btnUseSeOver = false;

	if (!tag.toLower().compare("clear")) {
		if (tag[0] != ':') btndefStr = ":c;" + tag;
		else btndefStr = tag;
	}
	return RET_CONTINUE;
}


int LOImageModule::btntimeCommand(FunctionInterface *reader) {
	//btnOverTime 只有在btndef时才会被重置
	btnOverTime = reader->GetParamInt(0);
	if (reader->isName("btntime2")) btnUseSeOver = true;
	return RET_CONTINUE;
}

int LOImageModule::getpixcolorCommand(FunctionInterface *reader) {
	int xx = reader->GetParamInt(1);
	int yy = reader->GetParamInt(2);
	LOString s = "000000";
	if (xx >= 0 && yy >= 0 && xx <= G_gameWidth && yy <= G_gameHeight) {
		//画面可能被放大，因此需要找到 x,y对应的放大区域的中心位置
		for (int ii = xx + 1; ii <= G_gameWidth ; ii++) {
			if (ii * G_gameScaleX != xx * G_gameScaleX) {
				xx = (ii + xx - 1) / 2;
				break;
			}
		}
		for (int ii = yy + 1; ii <= G_gameHeight; ii++) {
			if (ii * G_gameScaleY != yy * G_gameScaleY) {
				yy = (ii + yy - 1) / 2;
				break;
			}
		}
		SDL_Rect rect;
		char tmp[16];
		memset(tmp, 0, 16);
		rect.x = xx * G_gameScaleX;
		rect.y = yy * G_gameScaleY;
		rect.w = 1;
		rect.h = 1;
		
		//防止帧刷新
		SDL_LockMutex(layerQueMutex);
		SDL_RenderReadPixels(render, &rect, SDL_PIXELFORMAT_RGB888, tmp, 4);
		SDL_UnlockMutex(layerQueMutex);
		int val = *(int*)(&tmp[0]);
		sprintf(&tmp[4], "%06x", val & 0xffffff );
		s.assign(&tmp[4]);
	}
	ONSVariableRef *v = reader->GetParamRef(0);
	v->SetValue(&s);
	return RET_CONTINUE;
}


int LOImageModule::gettextCommand(FunctionInterface *reader) {
	ONSVariableRef *v = GetParamRef(0);
	v->SetValue(&dialogText);
	return RET_CONTINUE;
}



//检查指定区域是否全部为某种颜色，允许少量容差
int LOImageModule::chkcolorCommand(FunctionInterface *reader) {
	SDL_Rect src, dst;
	src.x = reader->GetParamInt(0); src.y = reader->GetParamInt(1);
	src.w = reader->GetParamInt(2); src.h = reader->GetParamInt(3);
	dst.x = 0; dst.y = 0;
	dst.w = src.w; dst.h = src.h;

	LOSurface *su = ScreenShot(&src, &dst);
	delete su;

	return RET_CONTINUE;
}



int LOImageModule::getscreenshotCommand(FunctionInterface *reader) {
	SDL_Rect src, dst;
	src.x = 0; src.y = 0;
	src.w = G_gameWidth; src.h = G_gameHeight;

	dst.x = 0; dst.y = 0;
	dst.w = reader->GetParamInt(0);
	dst.h = reader->GetParamInt(1);

	LOSurface *su =  ScreenShot(&src, &dst);
	//SDL_SaveBMP(su->GetSurface(), "test.bmp");
	delete su;

	return RET_CONTINUE;
}