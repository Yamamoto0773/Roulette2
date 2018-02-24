#pragma warning( disable : 4995 )				// 警告を無視
#pragma warning( disable : 4996 )				// 警告を無視
#include "CGame.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define DEBUGMODE

#include "libfiles/DEBUG.H"

#define STR(var) #var


/////////////////////////////////////////////////////////////////////////
// コンストラクタ
/////////////////////////////////////////////////////////////////////////
CGame::CGame() {
	bLostDevice		= FALSE;
	eState			= INIT;
	
	iRouletteState	= 0b0000;

	iWiningRoom		= 0;
	iWiningRoomid		= 0;

	ZeroMemory(&bOnKey, sizeof(bOnKey));
}


/////////////////////////////////////////////////////////////////////////
// デストラクタ
/////////////////////////////////////////////////////////////////////////
CGame::~CGame() {
	Clear();
}


///////////////////////////////////////////////////////
// 初期化＆ゲーム形成
///////////////////////////////////////////////////////
BOOL CGame::Init(HINSTANCE hinst) {
	const int windowW = 960, windowH = 720;

	// ウインドウ生成
	win.SetWindowStyle(WS_OVERLAPPEDWINDOW);					// 枠無しウインドウ(フルスクリーン時はWS_POPUPのみ、ウィンドウモード時はさらにWS_CAPTION|WS_SYSMENUなどを付ける)
	if (!win.Create(hinst, L"Roulette", 1, windowW, windowH)) {	// ウィンドウサイズは720p
		DEBUG("Window create error\n");
		return FALSE;
	}
	ImmAssociateContext(win.hWnd, NULL);			// IMEを出さないようにする

	// Direct3D生成
	// フルスクリーンの1920*1080の32bitカラーにセットする。
	// ※2つ目の引数をFALSEにするとウインドウモードに出来る
	if (!dd.Create(win.hWnd, FALSE, windowW, windowH, 32, 0, TRUE)) {
		DEBUG("Direct3D create error\n");
		return FALSE;
	}

	// DirectInput生成
	if (!di.Create(win.hWnd, win.hInstance)) {
		DEBUG("DirectInput生成失敗\n");
		return FALSE;
	}

	// キーボードを使う
	if (!di.CreateKeyboard()) {
		DEBUG("キーボード使用不可\n");
		return FALSE;
	}

	if (!df.Init(dd.GetD3DDevice()))
		return FALSE;

	// DirectXText生成
	if (!dt.Init(dd.GetD3DDevice(), windowW, windowH) ||
		!dtsmall.Init(dd.GetD3DDevice(), windowW, windowH)) {
		DEBUG("DirectXText生成失敗\n");
		return FALSE;
	}
	dt.Create(600, 600, L"Century Gothic", false);
	dtsmall.Create(20, 0, L"Century Gothic", false);

	ef = new EffectManager(&dd, (unsigned)windowW, (unsigned)windowH);


	// 画像ファイル読み込み
	char *filename[] ={
		"resource/background.jpg",
		"resource/effect1.png",
		"resource/effect2.png",
		"resource/effect3.png",
		"resource/effect4.png"
	};

	for (int i=0; i< (1+TEXTURECUNT); i++) {
		dd.AddTexture(i, filename[i]);
		CDDTexPro90 *tex = dd.GetTexClass(i);
		dd.SetPutRange(i, i, 0, 0, tex->GetWidth(), tex->GetHeight());
	}

	float scale = 1.0f;
	CDDTexPro90 *tex = dd.GetTexClass(0);
	if (tex->GetWidth()/tex->GetHeight() < windowW/windowH) {
		scale = (float)windowW/(float)tex->GetWidth();
	}
	else {
		scale = (float)windowH/(float)tex->GetHeight();
	}
	dd.SetPutStatus(0, 1.0f, scale, 0.0f);

	dd.SetBackColor(0xffffff);



	// ファイル読み込み
	if (!lottery.setRoomNumber("DEFINE/RoomNumber.txt")) {
		DEBUG("部屋番号の登録に失敗しました\n");
		return FALSE;
	}

	FILE *fp;
	fp = fopen("DEFINE/magnification.txt", "r");
	if (fp == NULL) {
		DEBUG("\"DEFINE/magnification.txt\"を開けませんでした");
		return FALSE;
	}

	char tmp[128];
	int i;
	int grade, mag;
	while (fgets(tmp, 128, fp) != NULL) {
		i=0, grade=0, mag=0;
		if (tmp[i] < '1' || tmp[i] > '5') {
			DEBUG("範囲外の学年の確率を指定しました");
			return FALSE;
		}

		grade = tmp[i] - '0';

		i++;
		while (tmp[i] == ' ') i++;	// 空白飛ばす

		while (tmp[i] >= '0' && tmp[i] <= '9') {
			mag *= 10;
			mag += tmp[i]-'0';
			i++;
		}

		// 倍率の登録をおこなう
		lottery.setMagnification(grade, mag);

	}
	

	ShowWindow(win.hWnd, SW_SHOW);


	return TRUE;
}


///////////////////////////////////////////////////////
// ロード済みデータの全開放
///////////////////////////////////////////////////////
BOOL CGame::Clear(void) {
	dd.Clear();

	return TRUE;
}


///////////////////////////////////////////////////////
// ルーレット実行関数
///////////////////////////////////////////////////////
BOOL CGame::RunRoulette() {
	// キーボード入力
	BYTE key[256];
	di.GetKeyboard(key);

	// ESCキーで終了
	if (key[DIK_ESCAPE]&0x80)
		return -1;

	// 仮想入力ハードウェア
	BOOL press[MAXKEYCNT];						// 押した瞬間にtrueになる配列
	ZeroMemory(&press, sizeof(press));

	// キーボードの処理
	static const int KEYID[MAXKEYCNT] ={		// キーのリスト
		DIK_SPACE, DIK_C, DIK_V, DIK_B, DIK_N, DIK_RETURN, DIK_1, DIK_2, DIK_3,DIK_4,DIK_5
	};

	for (int i=0; i<MAXKEYCNT; i++) {
		if ((key[KEYID[i]]&0x80)) {
			// キーボード入力があった場合
			if (!bOnKey[i]) {
				// まだ押されていなければ押された瞬間とする
				press[i] = TRUE;
				bOnKey[i] = TRUE;
			}
		}
		else {
			// 押されていなければフラグをリセット
			bOnKey[i] = FALSE;
		}
	}


	///////////////////////////////////////////////////
	// 入力処理
	///////////////////////////////////////////////////
	if (press[0]) {
		// スペースキーでルーレット停止
		iRouletteState &= 0b0000;
	}
	if (press[5]) {
		if (iRouletteState == 0b0000) {
			// ルーレットスタート
			if (SetNumber(0))
				iRouletteState |= 0b1111;
		}
	}
	else if (press[6]) {
		if (iRouletteState == 0b0000) {
			// ルーレットスタート
			if (SetNumber(1))
				iRouletteState |= 0b1111;
		}
	}
	else if (press[7]) {
		if (iRouletteState == 0b0000) {
			// ルーレットスタート　(女子くじのみ
			if (SetNumber(2))
				iRouletteState |= 0b1111;
		}
	}
	else if (press[8]) {
		if (iRouletteState == 0b0000) {
			// ルーレットスタート　(男子くじのみ
			if (SetNumber(3))
				iRouletteState |= 0b1111;
		}
	}
	else if (press[9]) {
		if (iRouletteState == 0b0000) {
			// ルーレットスタート　(男子くじのみ
			if (SetNumber(4))
				iRouletteState |= 0b1111;
		}
	}
	else if (press[10]) {
		if (iRouletteState == 0b0000) {
			// ルーレットスタート　(男子くじのみ
			if (SetNumber(5))
				iRouletteState |= 0b1111;
		}
	}


	if (press[1]) {
		iRouletteState &= 0b0111;
	}
	if (press[2]) {
		iRouletteState &= 0b1011;
	}
	if (press[3]) {
		iRouletteState &= 0b1101;
	}
	if (press[4]) {
		iRouletteState &= 0b1110;
	}


	ef->Update();


	////////////////////////////////////////////////////////////////////////////////////
	// デバイスロストチェック(フルスクリーン時にALT+TABを押した場合など)
	// ※復帰時は内部で管理しているテクスチャは自動的にリストアされるが、
	//   MANAGEDではない頂点バッファやテクスチャを使用している場合は、
	//   自分でロスト＆リストア処理を行う
	////////////////////////////////////////////////////////////////////////////////////
	if (!dd.CheckDevice()) {
		// ロスト中なら
		if (!bLostDevice) {
			// ロスト直後ならここで開放処理を行う
			DEBUG("デバイスがロストした\n");

			bLostDevice = TRUE;
		}

		// 描画せずに抜ける
		return 0;
	}

	if (bLostDevice) {
		// リストア直後ならここで再構築を行う
		DEBUG("リストアされた\n");

		bLostDevice = FALSE;
	}


	////////////////////////////////////////////////////////////////////////////////////
	// 描画処理
	////////////////////////////////////////////////////////////////////////////////////


	dd.DrawBegin();

	dd.Put2(0, 960/2, 720/2);	// 背景

	dtsmall.Draw(750, 700, 20, 0, 0x7fffffff, "(c)2018, Nanami Yamamoto");	// 署名

	ef->Draw();	// エフェクト

	dd.SetBlendOne(false);
	df.noStroke();
	df.fill(255, 255, 255, 130);
	df.rect(0, 200, 960, 260);	// ボックス

	// 当選番号の描画
	int width = 150;
	int textX = 600;
	int textY = 180;
	int num = iWiningRoom;
	for (int i=0; i<4; i++) {
		if ((iRouletteState >> i)&0b0001) {
			// ルーレット回転中
			dt.Draw(textX-width*i, textY, 300, 0, 0xff000000, "%d", rand()%10);
		}
		else {
			// ルーレット停止中
			if (iWiningRoom == 0) {
				// くじがなくなったとき
				dt.Draw(textX-width*i, textY, 300, 0, 0xff000000, "-");
			}
			else {
				dt.Draw(textX-width*i, textY, 300, 0, 0xff000000, "%d", num%10);
			}
		}
		num/=10;
	}

	// ルーレット停止中
	if (iRouletteState == 0) {
		if (iWiningRoomid != 0) {
			dt.Draw(textX+130, textY+50, 200, 0, 0xff000000, "(%d)", iWiningRoomid);
		}
	}



	dd.DrawEnd();


	// 継続
	return 0;
}


BOOL CGame::SetNumber(int mode) {
	// 当選番号取得
	
	const RoomNum *wining = lottery.getNumber(mode);

	iWiningRoom = wining->number;
	iWiningRoomid = wining->id;

	return TRUE;
}




/////////////////////////////////////////////////////////////////////////
// ゲームメインルーチン
/////////////////////////////////////////////////////////////////////////
BOOL CGame::Run(HINSTANCE hinst) {
	// ゲームメインループ
	MSG msg;
	BOOL bLoop=TRUE;
	while (bLoop) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message==WM_QUIT) {
				bLoop = FALSE;
				DEBUG("WM_QUIT\n");
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// メインゲーム処理分け
		switch (eState) {
			case INIT:
				// 初期化
				if (!Init(hinst)) {
					// 失敗
					eState = END;
				}
				else {
					// 成功
					eState = RUN;
				}
				break;

			case RUN:
				switch (RunRoulette()) {
					case 0:
						eState = RUN;
						break;
					case -1:
						eState = END;
						break;
				}
				break;

			case END:
				// 終了処理
				Clear();
				bLoop = FALSE;
				break;

			default:
				// 未定義のステート
				DEBUG("異常終了\n");
				return FALSE;
		}
		Sleep(0);
	}

	win.Delete();

	// プログラム正常終了
	return TRUE;
}
