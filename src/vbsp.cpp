
#include <cstdlib>
#include <fstream>
#include <pthread.h>
using namespace std;

#include <gtc/type_ptr.hpp>
#include <gtc/matrix_transform.hpp>

#include "engine.h"
#include "log.h"
#include "system/FileSystem.h"
#include "system/config.h"
#include "game/IGame.h"
#include "resource/ResourceManager.h"
#include "graphics/platform_gl.h"
#include "graphics/gl_utils.h"
#include "graphics/texture.h"
#include "graphics/glsl_prog.h"
#include "renderer/camera.h"
#include "renderer/font.h"
#include "graphics/fbo.h"

#include "vbsp.h"
#include "renderer/render_vars.h"
#include "renderer/vbsp_mesh.h"
#include "renderer/progs_manager.h"
#include "renderer/vbsp_material.h"
#include "file_format/mdl.h"
#include "world.h"

#include "physics.h"

GLfloat quadVerts[] =
{
	 1,-1,
	 1, 1,
	-1, 1,
	-1,-1
};

IGame *CreateGame(){
	return new vbspGame();
}

GLuint scrWidth = 800;
GLuint scrHeight = 480;
float aspect = 1;

glslProgsManager g_progsManager;
MaterialManager g_MaterialManager;

ResourceManager g_ResMan;

glslProg progQuadTexture;
GLint u_quad_transform;

map_t bspMap;
model_t model;

Texture defaultTex;
Texture whiteTex;
Material *testMat;
Material *loadingMat = 0;
bool loaded=0;

Camera camera;
glm::vec3 velocity(0);
glm::vec3 lastRot(0);

glm::vec3 startpos;
glm::vec3 startrot;

glm::mat4 modelMtx;
glm::mat4 mapMtx;
glm::mat4 mvpMtx;

int movemask;
float speed = 10;
bool noclip = 1;

char load_name[256] = "test1";
int load_mode = 0;//0-model, 1-map, 2-material
int texture_mip = 0;
int model_lod = 0;
float screen_scale = 1;
float zfar = 500;

bool flashlight = 0;
bool log_render = 0;//1;
bool no_cull = 0;
bool lock_frustum = 0;

void DrawQuad();
void CreateDefaultTextures();
void DrawTexQuad();
void DrawUI();
void DrawLoading();
void InputInit();
void Update(double deltaTime);

Font testFont;
FrameBufferObject screenFBO;

eGameState gameState = eGame;

pthread_mutex_t graphicsMutex;
pthread_mutex_t loadingMutex;
pthread_t loadingThread;

void SetupCamera(glm::vec3 pos, glm::vec3 rot)
{
	camera.pos = pos;
	camera.rot = rot;
	camera.rot.y = camera.rot.y-90;
	lastRot = camera.rot;
}

void *LoadMapThread(void *arg){
	if(!bspMap.Load(load_name)){
		EngineError("Error loading map\n");
	}
	if(glm::abs(startpos.x)>0.01){
		Log("startpos.x %f\n",startpos.x);
		SetupCamera(glm::mat3(mapMtx)*startpos, startrot);
	}
	CheckGLError("LoadMap", __FILE__, __LINE__);

	g_physics.Init();
	bspMap.InitPhysics();
	g_physics.InitPlayer(camera.pos);
	//g_physics.SetDebugDrawMode(1);
	
	pthread_mutex_lock(&loadingMutex);
	gameState = eGame;
	pthread_mutex_unlock(&loadingMutex);
	
	return 0;
}

void LoadMapAsync(){
	gameState = eLoading;
	
	pthread_create(&loadingThread, NULL, LoadMapThread, 0);
}

glm::vec3 atovec3(const char *str);
void ReadConfig()
{
#if 0
	char path[256];
	GetFilePath("vbsp.txt",path);
	ifstream cfg(path);
	if(!cfg)
	{
		Log("Config file not found!\n");
		return;
	}
	cfg >> load_mode;
	cfg >> texture_mip;
	cfg >> model_lod;
	cfg >> speed;
	cfg >> screen_scale;
	cfg >> load_name;
	cfg>>startpos.x;
	cfg>>startpos.y;
	cfg>>startpos.z;
	cfg>>startrot.x;
	cfg>>startrot.y;
	cfg>>startrot.z;
	//SetupCamera
	cfg.close();
#endif

	ConfigFile cfg;
	if(!cfg.Load("vbsp.txt"))
		return;
	load_mode = atoi(cfg.values["load_mode"].c_str());
	texture_mip = atoi(cfg.values["texture_mip"].c_str());
	model_lod = atoi(cfg.values["model_lod"].c_str());
	speed = atof(cfg.values["speed"].c_str());
	screen_scale = atof(cfg.values["screen_scale"].c_str());
	zfar = atof(cfg.values["zfar"].c_str());
	strcpy(load_name, cfg.values["load_name"].c_str());
	if(cfg.values.find("startpos")!=cfg.values.end())
		startpos = atovec3(cfg.values["startpos"].c_str());
	if(cfg.values.find("startpos")!=cfg.values.end())
		startrot = atovec3(cfg.values["startrot"].c_str());
}

void LoadShaders()
{
	double startTime = GetTime();
	glUseProgram(0);

	progQuadTexture.CreateFromFile("quad", "texture");
	u_quad_transform = progQuadTexture.GetUniformLoc("u_transform");
	progQuadTexture.Use();
	glUniform4f(u_quad_transform, 0,0,1,1);

	g_progsManager.Init(&camera);
	glUseProgram(0);
	LOG("Shaders loaded (%f s)\n",GetTime()-startTime);
	CheckGLError("LoadShaders", __FILE__, __LINE__);
}

void CreateDefaultTextures()
{
	GLubyte pixels[]={
		127,127,255,255,127,127,255,255,
		127,127,255,255,127,127,255,255,
		255,255,127,127,255,255,127,127,
		255,255,127,127,255,255,127,127,
		127,127,255,255,127,127,255,255,
		127,127,255,255,127,127,255,255,
		255,255,127,127,255,255,127,127,
		255,255,127,127,255,255,127,127};
	defaultTex.Create(8,8);
	defaultTex.SetFilter(GL_LINEAR, GL_LINEAR);
	defaultTex.SetWrap(GL_REPEAT);
	defaultTex.Upload(0, GL_LUMINANCE, pixels);

	g_MaterialManager.AddMaterial("*", new Material(eProgType_LightmappedGeneric, &defaultTex));//eProgType_Unlit
	g_MaterialManager.AddMaterial("*m", new Material(eProgType_VertexlitGeneric, &defaultTex));

	GLubyte whiteData[] = {255,63};
	whiteTex.Create(1,1);
	whiteTex.Upload(0, GL_LUMINANCE_ALPHA, whiteData);

	CheckGLError("CreateDefaultTextures", __FILE__, __LINE__);
}

double deltaTime=0;
double oldTime=0;
int fps=0;
int frame=0;

void vbspGame::Created()
{
	double startTime = GetTime();
	ReadConfig();

	pthread_mutex_init(&loadingMutex, NULL);
	pthread_mutex_init(&graphicsMutex, NULL);

	Log( "%s\n", glGetString(GL_VENDOR));
	Log( "%s\n", glGetString(GL_RENDERER));
	Log( "%s\n", glGetString(GL_VERSION));

	int dbits=0;
	glGetIntegerv(GL_DEPTH_BITS,&dbits);
	Log("depth bits %d\n",dbits);

	int rfmt=0;
	int rtp=0;
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT,&rfmt);
	glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE,&rtp);
	Log("Color read format %X, type %X\n",rfmt,rtp);

	g_ResMan.Init();

	LoadShaders();

	//texture
	testFont = Font();
	testFont.LoadBMFont("sansation",&g_ResMan);

	CreateDefaultTextures();

	//startup loading
	DrawLoading();
	EngineSwapBuffers();
	//

	screenFBO.Create();
	screenFBO.CreateTexture(256, 256, GL_LINEAR);//In changed()
	screenFBO.AddDepthBuffer();
	CheckGLError("LoadShaders", __FILE__, __LINE__);
	
	mapMtx = glm::scale( glm::rotate( glm::mat4(1),glm::radians(90.0f), glm::vec3(-1.0f, 0, 0)),glm::vec3(0.026f));

	//TODO make menu or console for this
	if(load_mode == 0){
		if(!model.Load(load_name, model_lod)){
			Log( "Error loading model\n");
		}
		SetupCamera(glm::vec3(0,0,3), glm::vec3(0,90,0));
	}else if(load_mode == 1){
#if 0
		LoadMapAsync();
#else
		if(!bspMap.Load(load_name)){
			Log( "Error loading map\n");
			exit(-1);
		}
		if(glm::abs(startpos.x)>0.01){
			Log("startpos.x %f\n",startpos.x);
			SetupCamera(glm::mat3(mapMtx)*startpos, startrot);
		}
		CheckGLError("LoadMap", __FILE__, __LINE__);

		g_physics.Init();
		bspMap.InitPhysics();
		g_physics.InitPlayer(camera.pos);
		//g_physics.SetDebugDrawMode(1);
#endif
	}else if(load_mode == 2){
		testMat = g_MaterialManager.GetMaterial(load_name);
		SetupCamera(glm::vec3(0,1,1), glm::vec3(0,90,0));
	}

	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D,0);
	glBindFramebuffer(GL_FRAMEBUFFER,0);
	glClearColor(0.2,0.2,0.2,1);
	//glClearColor(1,1,1,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
#ifndef ANDROID
	//gles2 behaviour
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	//TODO: Check extension
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif

	CheckGLError("Created", __FILE__, __LINE__);
	Log("Created() %f s\n",GetTime()-startTime);

	oldTime = GetTime();

	if(gameState == eGame)
		EnableCursor(false);
	
	loaded=true;
}

void vbspGame::Changed(int width, int height)
{
	if(width<=0 || height<=0)
		return;
	scrWidth = width;
	scrHeight = height;
	LOG( "Changed %dx%d\n", width, height);
	glViewport(0,0,width,height);

	aspect = (float)scrWidth/scrHeight;
	//camera.UpdateProj(75.0f, aspect, 0.2f, 1000.0f);
	camera.projMtx = glm::perspectiveFov(glm::radians(75.0f),(float)scrWidth,(float)scrHeight,0.2f,zfar);
	
	InputInit();
	
	if(!loaded)
		DrawLoading();
	
	screenFBO.Resize(width*screen_scale,height*screen_scale);
#ifdef ANDROID
	LoadShaders();
#endif
}

#include "button.h"
extern int buttonsCount;
extern Button buttons[];

double t=0;

void vbspGame::Draw()
{
	double startTime = GetTime();
	deltaTime = (startTime-oldTime);
	oldTime = startTime;

	t+=deltaTime;
	frame++;
	if(t>=1.0){
		t -= 1;
		fps=frame;
		frame=0;
	}
	Update(deltaTime);

	pthread_mutex_lock(&graphicsMutex);
	//
	glUseProgram(0);
	glBindTexture(GL_TEXTURE_2D,0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);
	//

	glBindFramebuffer(GL_FRAMEBUFFER,0);
	glViewport(0,0,scrWidth,scrHeight);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	CheckGLError("Clear", __FILE__, __LINE__);
	glEnableVertexAttribArray(0);

	if(gameState==eLoading){
		DrawLoading();
	}else{
	
	g_progsManager.StartFrame();

	//g_progsManager.SetMtx(mapMtx);
	//mvpMtx = camera.projMtx*camera.viewMtx*modelMtx;
	//modelProg.Use();
	//glUniformMatrix4fv(modelProg.u_mvpMtx, 1, false, glm::value_ptr(mvpMtx));
	//glUniformMatrix4fv(modelProg.u_modelMtx, 1, false, glm::value_ptr(modelMtx));
	//CheckGLError("Draw set uniforms", __FILE__, __LINE__);

	if(load_mode == 1){
		screenFBO.Bind();
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		bspMap.Draw();
		CheckGLError("DrawMap", __FILE__, __LINE__);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, scrWidth, scrHeight);
		progQuadTexture.Use();
		screenFBO.BindTexture();
		glUniform4f(u_quad_transform,0,0,1,1);
		DrawQuad();

		glDisable(GL_DEPTH_TEST);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		g_progsManager.SelectProg(eProgType_Color,0);
		g_physics.Draw();
	}
	else if(load_mode == 0)
	{
		model.Draw();
	}
	else if(load_mode == 2)
	{
		g_progsManager.SelectProg(eProgType_Unlit,0);
		g_progsManager.SetMtx(mapMtx);
		DrawTexQuad();
	}

	glDisable(GL_DEPTH_TEST);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	DrawUI();
	glEnable(GL_DEPTH_TEST);
	}

	glDisableVertexAttribArray(0);
	glUseProgram(0);
	pthread_mutex_unlock(&graphicsMutex);

	if(log_render)
		LOG("Render end\n");
	log_render = false;
	CheckGLError("Draw", __FILE__, __LINE__);
}

void Update(double deltaTime)
{
	if(gameState==eLoading)
		return;

	if(buttons[1].pressed){
		if(gameState == eGame){
			EnableCursor(true);
			gameState = ePause;
		}else{
			EnableCursor(false);
			gameState = eGame;
		}
		buttons[1].pressed = false;
	}

	if(gameState == eGame){
#ifndef ANDROID
		float rotSpeed = deltaTime*90.0;
		if(movemask & 64)
			camera.rot.x-=rotSpeed;
		else if(movemask & 128)
			camera.rot.x+=rotSpeed;

		if(movemask & 256)
			camera.rot.y-=rotSpeed;
		else if(movemask & 512)
			camera.rot.y+=rotSpeed;
#endif
		if(buttons[2].pressed){
			flashlight=!flashlight;
			buttons[2].pressed = false;
		}
		if(buttons[4].pressed){
			draw_prop_bboxes=!draw_prop_bboxes;
			buttons[4].pressed = false;
		}
		if(buttons[5].pressed){
			lock_frustum=!lock_frustum;
			buttons[5].pressed = false;
		}
		if(buttons[6].pressed){
			g_physics.Jump();
			buttons[6].pressed = false;
		}
		if(buttons[3].pressed){
			noclip=!noclip;
			g_physics.SetPlayerPos(camera.pos-glm::vec3(0,0.7,0));
			buttons[3].pressed = false;
		}

		if(!noclip && load_mode==1){
			//if(abs(velocity.x)+abs(velocity.z)>0.001)
			{
				glm::mat4 mtx = glm::rotate( glm::mat4(1), glm::radians(camera.rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
				glm::vec3 v = glm::mat3(mtx)*velocity*speed;
				g_physics.Move(v.x, v.z);
			}
		}else{
			camera.pos += glm::mat3(glm::inverse(camera.viewMtx))*velocity*speed*(float)deltaTime;
		}
	
		modelMtx = glm::rotate(glm::mat4(1),glm::radians(/*modelrot*/0.0f),glm::vec3(0, 1.0f, 0));
		modelMtx = glm::rotate( modelMtx, model.rootbonerot.x, glm::vec3(-1.0f, 0, 0));
		//modelMtx = glm::rotate( modelMtx, model.rootbonerot.z, glm::vec3(0, 0, 1.0));
		modelMtx = glm::scale( modelMtx, glm::vec3(0.026f));

		if(load_mode==1)
			g_physics.Update(deltaTime);

		camera.UpdateView();
		if(!lock_frustum)
			camera.UpdateFrustum();
	}
}

void UpdateRefract()
{
#if 0
//1
	if(!framePixels)
		framePixels = new GLubyte[scrWidth*scrHeight*4];
	glReadPixels(0, 0, scrWidth, scrHeight, frameReadFormat, GL_UNSIGNED_BYTE, framePixels);
	glActiveTexture(GL_TEXTURE3);
	if(!frameTex.id)
		frameTex.Create(scrWidth,scrHeight);
	frameTex.Upload(0, frameReadFormat, framePixels);
	CheckGLError("UpdateRefract", __FILE__, __LINE__);
//2
	glActiveTexture(GL_TEXTURE3);
	if(!frameTex.id)
		frameTex.Create(scrWidth,scrHeight);
	glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,0,0,scrWidth,scrHeight,0);
	CheckGLError("UpdateRefract", __FILE__, __LINE__);
#endif
}

void Button::SetUniform(int loc)
{
	glUniform4f(loc, x*2.0f-(1-w), -(y*2.0f-(1-h)), w, h);
}

void DrawLoading()
{
	if(!loadingMat)
		loadingMat = g_MaterialManager.GetMaterial("console/startup_loading");
	progQuadTexture.Use();
	loadingMat->Bind();
	glUniform4f(u_quad_transform,0.7,-0.7,0.2,-0.2/aspect);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnableVertexAttribArray(0);
	DrawQuad();
	
	whiteTex.Bind();
	glUniform4f(u_quad_transform,0.1,-0.2,0.4*t,-0.2/aspect);
	/*
	glm::mat4 mtx = glm::mat4(1.0);
	mtx = glm::translate(mtx,glm::vec3(0.5f*aspect,0.5f,0));
	mtx = glm::rotate(mtx,t*6.28,glm::vec3(0,0,1));

	SetModelMtx(mtx);
	DrawRect(-0.1/aspect,0.9,0.2/aspect,0.2,false,&testTex);
	SetModelMtx(glm::mat4(1.0));

	DrawText("Loading...",0.4,0.8,0.5);
	*/
}

extern Texture g_lightmaps[8];
#define DRAW_LM_IN_UI 0
void DrawUI()
{
#ifndef WIN32
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	whiteTex.Bind();
	//g_renderer.UseProgram(&progQuad);
	progQuadTexture.Use();
	for(int i=0;i<buttonsCount;i++)
	{
		buttons[i].SetUniform(u_quad_transform);
		DrawQuad();
	}

	glDisable(GL_BLEND);
#endif
#if DRAW_LM_IN_UI
	progQuadTexture.Use();
	for(int i = 0; i<8; i++)
	{
		if(!g_lightmaps[i].id)
			break;
		g_lightmaps[i].Bind();
		glUniform4f(u_quad_transform,-0.8+0.4*i,-0.5,0.2,0.2*aspect);
		DrawQuad();
	}
#endif

	if(load_mode == 1)
	{
		float textSize = 0.5f;
		glDisable(GL_BLEND);
		//glEnable(GL_BLEND);
		//glBlendFunc(1, 1);
		glDisable(GL_CULL_FACE);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(2);
		glslProg *p = g_progsManager.GetProg(eProgType_Unlit,0);
		p->Use();
		glm::mat4 mtx = glm::scale(glm::mat4(1.0),glm::vec3(1.0f,aspect,1.0f));
		glUniformMatrix4fv(p->u_mvpMtx,1,GL_FALSE,glm::value_ptr(mtx));
		char str[128];
		snprintf(str,128,"cur: leaf %d, cluster %d, area %d\nrender: leafs, %d mats %d props %d", (int)(cameraLeaf-bspMap.leafs),cameraLeaf->cluster, cameraLeaf->area, renderLeafs, renderMats, renderProps);
		testFont.Print(str,-1.0,-0.9/aspect,textSize);

		snprintf(str,128,"fps %d delta time %f\n", fps, deltaTime);
		testFont.Print(str,-1.0,0.9/aspect,textSize/aspect);

		if(gameState==ePause){
			testFont.Print("Paused",-0.3,0.0,2.0);
		}

		glDisableVertexAttribArray(2);
		glEnable(GL_CULL_FACE);
		//glDisable(GL_BLEND);
	}

	glBindTexture(GL_TEXTURE_2D,0);
	CheckGLError("DrawUI", __FILE__, __LINE__);
	/*
	vboQuad.Bind();
#if FBO
	g_renderer.UseProgram(&progQuadTexture);
	glUniform4f(u_quad_transformTex,0,0,1,1);
	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,8,0);
//	glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,8,(void*)12);
	fbo.BindTexture();
	glDrawArrays(GL_TRIANGLE_FAN,0,4);
#endif
#ifndef WIN32
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

	glBindTexture(GL_TEXTURE_2D,0);
	g_renderer.UseProgram(&progQuadColor);
	glUniform4f(u_colorQuad,0.5,0.5,0.5,0.2);
#ifdef BULLET
	buttons[0].SetUniform(u_quad_transformCol);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);
#endif
	buttons[1].SetUniform(u_quad_transformCol);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);
	buttons[2].SetUniform(u_quad_transformCol);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);

	glDisable(GL_BLEND);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

#endif
*/
	/*if(camera.frustum.Contains(glm::vec4(0,0,0,1)))
	{
		progQuadTexture.Use();
		g_lightmaps[0].Bind();
		glUniform4f(u_quad_transform,-0.8,-0.5,0.2,0.2*aspect);
		DrawQuad();
	}*/
}

void DrawQuad()
{
	glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,8,&quadVerts);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);
}

void DrawTexQuad()
{
	//glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	/*if(mat->transparent)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(0);
	}*/

	testMat->Bind();

	GLfloat tempvert[]={-20,0,0,0,0, 20,0,0,1,0, -20,0,40,0,1, 20,0,40,1,1};
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,20,tempvert);
	glVertexAttrib3f(1, 0.5,0.5,1);
	glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,20,tempvert+3);
	glDrawArrays(GL_TRIANGLE_STRIP,0,4);

	/*if(mat->transparent)
	{
		glDisable(GL_BLEND);
		glDepthMask(1);
	}*/
	//glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

float boxShapeVerts[] =
{
	-1,-1,-1,
	-1,-1, 1,
	-1, 1,-1,
	-1, 1, 1,
	 1,-1,-1,
	 1,-1, 1,
	 1, 1,-1,
	 1, 1, 1
};

GLubyte boxShapeInds[] =
{
	0,1, 0,2, 0,4,
	5,4, 5,1, 5,7,
	3,2, 3,1, 3,7,
	6,2, 6,7, 6,4
};//24

void DrawBoxLines(glm::mat4 mtx, glm::vec3 mins, glm::vec3 maxs, glm::vec4 color)
{
	glm::vec3 size = (maxs-mins)*0.5f;
	glm::vec3 pos = (maxs+mins)*0.5f;
	mtx *= glm::translate(glm::mat4(1),pos);
	mtx *= glm::scale(glm::mat4(1),size);

	g_progsManager.SetMtx(mtx);
	//g_progsManager.SelectProg(eProgType_Color,0);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12, boxShapeVerts);
	/*glDepthFunc(GL_GREATER);
	glm::vec4 hcol = color*0.5f;
	glUniform4fv(u_color,1, (float*)&hcol);
	glDrawElements(GL_LINES,24,GL_UNSIGNED_BYTE,boxShapeInds);
	glDepthFunc(GL_LESS);
	glUniform4fv(u_color,1,(float*)&color);*/
	g_progsManager.SetColor(color);
	if(log_render)
		LOG("glDrawElements %d %p\n", 24, boxShapeInds);
	glDrawElements(GL_LINES,24,GL_UNSIGNED_BYTE,boxShapeInds);
}

void DrawBoxLines(glm::mat4 mtx, BoundingBox bbox, glm::vec4 color)
{
	DrawBoxLines(mtx, bbox.min,bbox.max,color);
}
