
#pragma once

#include "game/IGame.h"

class vbspGame: public IGame{
	void Created();
	void Changed(int w, int h);
	void Draw();
	const char *GetGamedir(){
		return "vbsp";
	}
	void OnKey(int key, int scancode, int action, int mods);
	void OnTouch(float tx, float ty, int ta, int tf);
	void OnMouseMove(float x, float y);
};

enum eGameState
{
	eMenu,
	eGame,
	ePause,
	eLoading
};
extern eGameState gameState;

#include <pthread.h>
extern pthread_mutex_t graphicsMutex;

