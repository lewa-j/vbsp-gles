
#pragma once

struct Button
{
	float x,y, w,h;
	int type;
	bool pressed;

	Button():pressed(false){}

	Button(float nx, float ny, float nw, float nh, bool adjust = false);
	
	void SetUniform(int loc);
	
	bool Hit(float tx, float ty);

};
