
#include <vec3.hpp>
#include "log.h"
#include "renderer/camera.h"
#include "button.h"
#include "vbsp.h"

#include "renderer/render_vars.h"

extern glm::vec3 velocity;
extern Camera camera;
extern float speed;
extern glm::vec3 lastRot;

void LoadShaders();

extern float aspect;
int buttonsCount = 7;
Button buttons[7];
Button::Button(float nx, float ny, float nw, float nh, bool adjust):
			x(nx),y(ny),w(nw),h(nh),pressed(false)
{
	if(adjust)
	{
		if(aspect>1)
			h*=aspect;
		else
			w/=aspect;
	}
	LOG("Created button %f %f %f %f\n",x,y,w,h);
}

bool Button::Hit(float tx, float ty)
{
	if(tx>x+w||tx<x)
		return false;

	if(ty>y+h||ty<y)
		return false;

	pressed = true;
	return true;
}

void InputInit()
{
//#ifdef ANDROID
	buttons[0] = Button(0.0,0.5,0.5,0.5);//move
	buttons[1] = Button(0.8,0.1,0.2,0.2,true);//pause
	buttons[2] = Button(0.8,0.7, 0.1,0.1,true);//flashlight
	buttons[3] = Button(0.8,0.85,0.1,0.1,true);//noclip
	buttons[6] = Button(0.8,0.55,0.1,0.1,true);//jump
//#endif
}

//#ifdef ANDROID
extern unsigned int scrWidth;
extern unsigned int scrHeight;

float starttx = 0;
float startty = 0;
int move = false;
float cameraSpeed = 120;

void vbspGame::OnTouch(float tx, float ty, int ta, int tf)
{
#ifdef ANDROID
	float x = tx/scrWidth;//480;
	float y = (ty-76)/scrHeight;//800;

	//LOG("touch action %d at %fx%f\n",ta,x,y);
	if(ta==0)
	{
		starttx = x;
		startty = y;

		for(int i=0; i<buttonsCount; i++)
		{
			buttons[i].Hit(x,y);
		}
	}
	else if(ta==1)
	{
		if(!buttons[0].pressed)
		{
			lastRot.x-=(startty-y)*cameraSpeed;
			lastRot.y+=(starttx-x)*cameraSpeed;
		}
		velocity = glm::vec3(0);
		for(int i=0; i<buttonsCount; i++)
		{
			buttons[i].pressed = false;
		}
	}
	else if(ta==2)
	{
		if(gameState==eGame){
		if(buttons[0].pressed)
		{
			velocity.x = -(starttx-x);
			velocity.z = -(startty-y);
		}
		else
		{
			camera.rot.x = lastRot.x-(startty-y)*cameraSpeed;
			camera.rot.y = lastRot.y+(starttx-x)*cameraSpeed;
		}
		}
	}
#endif
}

void vbspGame::OnMouseMove(float mx, float my)
{
	if(gameState == eGame)
	{
		float dx = mx-starttx;
		float dy = my-startty;
		starttx=mx;
		startty=my;
		camera.rot.x += dy*0.1;
		camera.rot.y -= dx*0.1;
	}
}

#ifdef ANDROID
#include <android/keycodes.h>
#include <android/input.h>
void vbspGame::OnKey(int key, int scancode, int action, int mods)
{
	LOG("OnKey(%d,%d)\n",key,action);
	if(key==AKEYCODE_W)
	{
		if(action==AKEY_EVENT_ACTION_DOWN)
		{
			velocity.z = -1;
		}
		else if(action==AKEY_EVENT_ACTION_UP)
		{
			velocity.z = 0;
		}
	}
	if(key==AKEYCODE_S)
	{
		if(action==AKEY_EVENT_ACTION_DOWN)
		{
			velocity.z = 1;
		}
		else if(action==AKEY_EVENT_ACTION_UP)
		{
			velocity.z = 0;
		}
	}
	if(key==AKEYCODE_SHIFT_LEFT)
	{
		if(action==AKEY_EVENT_ACTION_DOWN)
		{
			velocity.y = 1;
		}
		else if(action==AKEY_EVENT_ACTION_UP)
		{
			velocity.y = 0;
		}
	}
	if(key==AKEYCODE_CTRL_LEFT)
	{
		if(action==AKEY_EVENT_ACTION_DOWN)
		{
			velocity.y = -1;
		}
		else if(action==AKEY_EVENT_ACTION_UP)
		{
			velocity.y = 0;
		}
	}
	
	if(action==AKEY_EVENT_ACTION_DOWN)
	{	
		if(key==AKEYCODE_F)
		{
			flashlight = !flashlight;
		}
	}
}
#else

#include <GLFW/glfw3.h>
extern int movemask;

void vbspGame::OnKey(int key, int scancode, int action, int mods)
{
	if(key==GLFW_KEY_W)
	{
		if(action==GLFW_PRESS)
		{
			velocity.z = -1;
		}
		else if(action==GLFW_RELEASE)
		{
			velocity.z = 0;
		}
	}
	if(key==GLFW_KEY_S)
	{
		if(action==GLFW_PRESS)
		{
			velocity.z = 1;
		}
		else if(action==GLFW_RELEASE)
		{
			velocity.z = 0;
		}
	}
	if(key==GLFW_KEY_A)
	{
		if(action==GLFW_PRESS)
		{
			velocity.x = -1;
		}
		else if(action==GLFW_RELEASE)
		{
			velocity.x = 0;
		}
	}
	if(key==GLFW_KEY_D)
	{
		if(action==GLFW_PRESS)
		{
			velocity.x = 1;
		}
		else if(action==GLFW_RELEASE)
		{
			velocity.x = 0;
		}
	}
	if(key==GLFW_KEY_LEFT_SHIFT)
	{
		if(action==GLFW_PRESS)
		{
			velocity.y = 1;
		}
		else if(action==GLFW_RELEASE)
		{
			velocity.y = 0;
		}
	}
	if(key==GLFW_KEY_LEFT_CONTROL)
	{
		if(action==GLFW_PRESS)
		{
			velocity.y = -1;
		}
		else if(action==GLFW_RELEASE)
		{
			velocity.y = 0;
		}
	}
	if(key==GLFW_KEY_UP)
	{
		if(action==GLFW_PRESS)
		{
			movemask |= 64;
		}
		else if(action==GLFW_RELEASE)
		{
			movemask &= 1023-64;
		}
	}
	if(key==GLFW_KEY_DOWN)
	{
		if(action==GLFW_PRESS)
		{
			movemask |= 128;
		}
		else if(action==GLFW_RELEASE)
		{
			movemask &= 1023-128;
		}
	}
	if(key==GLFW_KEY_RIGHT)
	{
		if(action==GLFW_PRESS)
		{
			movemask |= 256;
		}
		else if(action==GLFW_RELEASE)
		{
			movemask &= 1023-256;
		}
	}
	if(key==GLFW_KEY_LEFT)
	{
		if(action==GLFW_PRESS)
		{
			movemask |= 512;
		}
		else if(action==GLFW_RELEASE)
		{
			movemask &= 1023-512;
		}
	}

	if(key==GLFW_KEY_Q)
	{
		if(action==GLFW_PRESS)
		{
			speed-=0.5f;
		}
	}
	if(key==GLFW_KEY_E)
	{
		if(action==GLFW_PRESS)
		{
			speed+=0.5f;
		}
	}

	if(action==GLFW_PRESS)
	{
		if(key==GLFW_KEY_C)
		{
			Log("Current position is %f %f %f\n", camera.pos.x, camera.pos.y, camera.pos.z);
			Log("Current rotation is %f %f %f\n", camera.rot.x, camera.rot.y, camera.rot.z);
		}
		if(key==GLFW_KEY_B)
		{
			LoadShaders();
		}
		if(key==GLFW_KEY_1)
		{
			draw_verts = !draw_verts;
		}
		if(key==GLFW_KEY_2)
		{
			draw_lightmaps = !draw_lightmaps;
		}
		if(key==GLFW_KEY_3)
		{
			draw_entities = !draw_entities;
		}
		if(key==GLFW_KEY_4)
		{
			no_cull = !no_cull;
		}
		if(key==GLFW_KEY_5)
		{
			lock_frustum=!lock_frustum;
		}
		if(key==GLFW_KEY_6)
		{
			no_vis = !no_vis;
		}
		if(key==GLFW_KEY_7)
		{
			log_render = true;
		}
		if(key==GLFW_KEY_8)
			draw_prop_bboxes=!draw_prop_bboxes;

		if(key==GLFW_KEY_F)
		{
			buttons[2].pressed = true;
		}
		if(key==GLFW_KEY_SPACE)
			buttons[6].pressed = true;
		if(key==GLFW_KEY_V)
			buttons[3].pressed = true;
		if(key==GLFW_KEY_ESCAPE)
			buttons[1].pressed = true;
	}
}
#endif
