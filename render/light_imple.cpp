#include "renderluo.h"

void set_light_color(light_t *light,float r,float g,float b)
{
	light->light.r=r;
	light->light.g=g;
	light->light.b=b;
}

void set_light_pos(light_t *light,float x,float y,float z)
{
	light->pos.x=x;
	light->pos.y=y;
	light->pos.z=z;
	light->pos.w=1;
}

void global_ambient_light(device_t *device,light_t *light)
{
	int width=device->width;
	int height=device->height;
	for(int i=0;i<height;i++)
	{
		for(int j=0;j<width;j++)
		{
			int R=(int)(light->light.r*255.0f);
			int G=(int)(light->light.g*255.0f); 
			int B=(int)(light->light.b*255.0f);
			R = CMID(R,0,255);//·ÀÖ¹³ö½ç 
			G = CMID(G,0,255);
			B = CMID(B,0,255);
			device->framebuffer[i][j]=(R<<16)|(G<<8)|B;
		}
	}
}
/*
void diffuse_reflection_light(device_t *device,light_t *light)
{
	device->light.light.r=light->light.r*0.5;
	device->light.light.g=light->light.g*0.5;
	device->light.light.b=light->light.b*0.5;
}*/

void open_light(device_t *device,light_t *light)
{
	if(device->render_state&RENDER_STATE_LIGHT_GLOBAL_AMBIENT)
	{
		global_ambient_light(device,light);
	}/*
	if(device->render_state&RENDER_STATE_LIGHT_DIFFUSE_REFLECTION)
	{
		diffuse_reflection_light(device,light);
	}*/
}