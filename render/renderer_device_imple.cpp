#include "renderluo.h"

//=====================================================================
// 渲染设备
//=====================================================================

//设备初始化，fb为外部帧缓存，非NULL将引用外部缓存帧缓存（每行对齐4字节）
void device_init(device_t *device,int width,int height,void *fb)
{
	//不是说void*因为没有指定对象类型，所以无法解释指向对象大小吗？
	//此处sizeof(void*)的大小与编译器的目标平台有关，如果目标平台是32位的，那么sizeof(void*)的大小为4字节
	//指针的本质就是内存地址，因此指针大小与内存空间，即编址位数有关。 
	char *framebuf;
	char *zbuf;
	 
	int n=sizeof(void*)*(height*2+1024)+width*height*8;//width*height*8：屏幕像素个数，每个像素8位，256色； 
	char *ptr=(char*)malloc(n+64);
	assert(ptr);//assert的作用是如果表达式为False，那么先向stderr打印一条出错消息，然后通过abort来终止程序
	
	device->framebuffer=(IUINT32**)ptr; //ptr=0;
	device->zbuffer=(float**)(ptr+sizeof(void*)*height);//ptr=0;
	ptr+=sizeof(void*)*height*2;
	device->texture=(IUINT32**)ptr;
	ptr+=sizeof(void*)*1024;
	framebuf=(char*)ptr;
	zbuf=(char*)ptr+width*height*4;
	ptr+=width*height*8;
	if(fb!=NULL)//如果有外部帧缓存，就是用外部帧缓存
	{
		framebuf=(char*)fb;
	} 
	for(int j=0;j<height;j++)
 	{
		device->framebuffer[j]=(IUINT32*)(framebuf+width*4*j);//将帧缓存地址赋给framebuffer
		device->zbuffer[j]=(float*)(zbuf+width*4*j);//将深度缓存地址赋给zbuffer
	}
	device->texture[0]=(IUINT32*)ptr;
	device->texture[1]=(IUINT32*)(ptr+16);
	memset(device->texture[0],0,64);
	device->tex_width=2;
	device->tex_height=2;
	device->max_u=1.0f;
	device->max_v=1.0f;
	device->width=width;
	device->height=height;
	device->background=0xc0c0c0;
	device->foreground=0;
	transform_init(&device->transform,width,height);
	device->render_state=RENDER_STATE_WIREFRAME; 
}

//清空framebuffer和zbuffer
void device_clear(device_t *device,int mode)
{
	int height=device->height;
	int y;
	for(y=0;y<device->height;y++)
 	{
 		//使用清空颜色使用的是不同灰度（但颜色都是灰色，灰色的强度不同而已） 
		IUINT32 cc=(height-1-y)*230/(height-1);//cc清空使用的颜色；
		//rgb，256色，8位表示一种色（r/g/b）；左移16位，左移8位，左移0位，三种颜色合并得出正确的清空颜色
		//cc用于存储颜色，共32位，前8位空，后24位分别存储R、G、B三色。此处获得一个8位数字后向左移16、8位即RGB数值相同，cc为一种灰度颜色，去不同的cc，也就是灰度大小不同 
		cc=(cc<<16)|(cc<<8)|cc;
		if(mode==0)
		{
			cc=device->background;
		} 
		IUINT32 *dst=device->framebuffer[y];
		for(int x=device->width;x>0;dst++,x--) 
		{
			dst[0]=cc;
		}
	}
	for(y=0;y<device->height;y++)
 	{
		float *dst=device->zbuffer[y];
		for(int x=device->width;x>0;dst++,x--)
		{
			dst[0]=0.0f;
		} 
	}
}

//设置当前纹理
void device_set_texture(device_t *device,void *bits,long pitch,int w,int h) 
{
	//bits为原纹理起始地址
	//pitch为一行纹理行首尾之间差量
	//pitch必须为long型，32位中为4字节，64位中为8字节，与内存地址位数相同
	//需要处理成宽高为w，h的纹理 
	char *ptr=(char*)bits;
	assert(w<=1024&&h<=1024);
	for(int j=0;j<h;ptr+=pitch,j++)//重新计算每行纹理，不论读进来的纹理大小是怎样
	{
		device->texture[j]=(IUINT32*)ptr;//纹理的存储地址 
	}
	device->tex_width=w;
	device->tex_height=h;
	device->max_u=(float)(w-1);
	device->max_v=(float)(h-1);
}

//删除设备 
void device_destroy(device_t *device) 
{
	if(device->framebuffer)
	{
		free(device->framebuffer);
	} 
	device->framebuffer=NULL;
	device->zbuffer=NULL;
	device->texture=NULL;
}

//画点 
void device_pixel(device_t *device,int x,int y,IUINT32 color) 
{
	if(((IUINT32)x)<=(IUINT32)device->width&&((IUINT32)y)<=(IUINT32)device->height) 
	{
		device->framebuffer[y][x]=color;
	}
}

//画线段 
void device_draw_line(device_t *device,int x1,int y1,int x2,int y2,IUINT32 c) 
{
	int x;
	int y;
	int rem=0;
	if(x1==x2&&y1==y2)//只是一个点 
 	{
		device_pixel(device,x1,y1,c);
	}	
	else if(x1==x2)//在一条直线上 
	{
		int inc=(y1<=y2)?1:-1;
		for(y=y1;y!=y2;y+= inc)
		{
			device_pixel(device,x1,y,c);
		} 
		device_pixel(device,x2,y2,c);
	}	
	else if(y1==y2) 
	{
		int inc=(x1<=x2)?1:-1;
		for (x=x1;x!=x2;x+=inc) 
		{
			device_pixel(device,x,y1,c);
		}
		device_pixel(device,x2,y2,c);
	}	
	else//没在一条直线上，中点画线算法 
	{
		int dx=(x1<x2)?x2-x1:x1-x2;
		int dy=(y1<y2)?y2-y1:y1-y2;
		if(dx>=dy)//dx>dy主方向为x 
		{
			if(x2<x1)
			{
				x=x1;
	  	 		x1=x2;
	  	 		x2=x;
	  	 		y=y1;
   			 	y1=y2;
			 	y2=y;
			}
			for(x=x1,y=y1;x<=x2;x++)
		 	{
				device_pixel(device,x,y,c);
				rem+=dy;
				if(rem>=dx)//斜率超过1，y需要+1（或-1，视y1与y2的大小而定） 
				{
					rem-=dx;
					y+=(y2>=y1)?1:-1;
					device_pixel(device,x,y,c);
				}
			}
			device_pixel(device,x2,y2,c);
		}	
		else//dy>dx主方向为y 
		{
			if(y2<y1) 
			{
				x=x1;
				x1=x2;
				x2=x;
				y=y1;
				y1=y2;
				y2=y;
			} 
			for(x=x1,y=y1;y<=y2;y++)
		 	{
				device_pixel(device,x,y,c);
				rem+=dx;
				if(rem>=dy) 
				{
					rem-=dy;
					x+=(x2>=x1)?1:-1;
					device_pixel(device,x,y,c);
				}
			}
			device_pixel(device,x2,y2,c);
		}
	}
}

//根据坐标读取纹理 
IUINT32 device_texture_read(const device_t *device,float u,float v) 
{
	int x;
	int y;
	u=u*device->max_u;//max_u是指纹理的最大宽度，u不是整数坐标，而是0~1的数值 
	v=v*device->max_v;
	x=(int)(u+0.5f);//四舍五入 
	y=(int)(v+0.5f);
	x=CMID(x,0,device->tex_width-1);//如果x超出了边界，就选边界；如果x没有超出边界，就选x 
	y=CMID(y,0,device->tex_height-1);
	return device->texture[y][x];//y第几行，x第几列 
}