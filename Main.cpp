#include <Windows.h>
#include "CGame.h"

#define DEBUGMODE
#include "libfiles/DEBUG.H"

///////////////////////////////////////////////////////////////////////////////////////////////////
// main関数のWindows版
///////////////////////////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain( HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdParam,int nCmdShow )
{
	// デバッグライブラリ初期化
	INITDEBUG();
	CLEARDEBUG();

	

	// ゲームメイン
	CGame *game = new CGame();
	game->Run( hInstance );
	delete game;

	

	return 0;
}
