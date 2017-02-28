#include "renderluo.h"

//=====================================================================
// 渲染实现
//=====================================================================

//绘制扫描线(扫描线填充算法) 
void device_draw_scanline(device_t *device,scanline_t *scanline)
{
	IUINT32 *framebuffer=device->framebuffer[scanline->y];
	float *zbuffer=device->zbuffer[scanline->y];
	
	int width=device->width;
	int w=scanline->w;
	int x=scanline->x;
	int render_state=device->render_state;
	for(;w>0;x++,w--)
	{
		if(x>=0&&x<width)
		{
			float rhw=scanline->v.rhw;//v.rhw=1/v.w 
			if(rhw>=zbuffer[x])//zbuffer中存储的也是rhw（1/w）;w与z经过projection矩阵后成线性关系 
			{
				float w=1.0f/rhw;
				zbuffer[x]=rhw;//如果z更小，更接近观察者，更新深度缓存的相应位置	
				if(render_state&RENDER_STATE_COLOR)//按位且，扫描线绘制使用颜色 
				{
					float r=scanline->v.color.r*w;
					float g=scanline->v.color.g*w;
					float b=scanline->v.color.b*w;
					int R=(int)(r*255.0f);//当初存的是r∈[0,1]
					int G=(int)(g*255.0f); 
					int B=(int)(b*255.0f);
					R = CMID(R,0,255);//防止出界
					G = CMID(G,0,255);
					B = CMID(B,0,255);
					framebuffer[x]=(R<<16)|(G<<8)|(B);//颜色的存储是：0xffffff，前ff是R，中ff为G，后ff为B
				}
				if(render_state&RENDER_STATE_TEXTURE)//扫描线绘制使用纹理
				{
					float u=scanline->v.tc.u*w;//在vertex_rhw_init中全除了个w，这里要变回来
					float v=scanline->v.tc.v*w;//IUINT32 *framebuffer=device->framebuffer[scanline->y];也是一点一点取的纹理，
					IUINT32 cc=device_texture_read(device,u,v); 
					framebuffer[x]=cc;//纹理也是现实在颜色缓存（帧缓存）中的
				} /*
				if(render_state&RENDER_STATE_LIGHT_DIFFUSE_REFLECTION)
				{
					int R=(int)(device->light.light.r*255.0f);//当初存的是r∈[0,1]
					int G=(int)(device->light.light.g*255.0f); 
					int B=(int)(device->light.light.b*255.0f);
					R = CMID(R,0,255);//防止出界
					G = CMID(G,0,255);
					B = CMID(B,0,255);
					framebuffer[x]+=(R<<16)|(G<<8)|(B);
				}*/
			}
		}
		vertex_add(&scanline->v,&scanline->step);//移动扫描线上需要绘制的点
		if(x>=width)
		{
			break;
		} 
	}
}

//主渲染函数 
void device_render_trap(device_t *device,trapezoid_t *trap)
{
	scanline_t scanline;
	int top=(int)(trap->top+0.5f);//需要绘制的水平三角形的顶部
	int bottom=(int)(trap->bottom+0.5f);
	for(int j=top;j<bottom;j++)
	{
		if(j>=0&&j<device->height)//三角形在视野范围之内
		{
			trapezoid_edge_interp(trap,(float)j+0.5f);//获取扫描线与顶点的两个交点 
			trapezoid_init_scan_line(trap,&scanline,j);//根据两个端点初始化计算扫描线的起点和步长
			device_draw_scanline(device,&scanline);//画扫描线 
		} 
		if(j>=device->height)//超出视野外的部分无需绘制 
		{
			break;
		}
	} 
}

//画原始三角形 
void device_draw_primitive(device_t *device,const vertex_t *v1,const vertex_t *v2,const vertex_t *v3)
{
	point_t p1;
	point_t p2;
	point_t p3;
	point_t c1;
	point_t c2;
	point_t c3;
	int render_state=device->render_state;
	
	transform_apply(&device->transform,&c1,&v1->pos);//将v1->pos transform变换后存在c1中
	transform_apply(&device->transform,&c2,&v2->pos);//c2就保证在cvv中了 
	transform_apply(&device->transform,&c3,&v3->pos); 
	
	//此处只是粗糙的判断：若有一个点没在cvv中就不画这个三角形
	if(transform_check_cvv(&c1)!=0||transform_check_cvv(&c2)!=0||transform_check_cvv(&c3)!=0)
	{
		return;
	} 
	//归一化
	transform_homogenize(&device->transform,&p1,&c1);
	transform_homogenize(&device->transform,&p2,&c2);
	transform_homogenize(&device->transform,&p3,&c3);
	
	if(render_state&(RENDER_STATE_TEXTURE|RENDER_STATE_COLOR ))//纹理或颜色的状态变量开启，则绘制三角形 
	{
		vertex_t t1=*v1;
		vertex_t t2=*v2;
		vertex_t t3=*v3;
		
		trapezoid_t traps[2];
		int n;
		
		t1.pos=p1;
		t2.pos=p2;
		t3.pos=p3;
		t1.pos.w=c1.w;
		t2.pos.w=c2.w;
		t3.pos.w=c3.w;//w还是坐标变换后的w，保存的是深度信息	
		
		vertex_rhw_init(&t1);//除点位置坐标外，其他同除以w，因为在计算与边交点的过程中，会把w置为1，此处将其他信息先除w 
		vertex_rhw_init(&t2);
		vertex_rhw_init(&t3);
		
		n=trapezoid_init_triangle(traps,&t1,&t2,&t3);//将原三角形拆分成可以用扫描线绘制的三角形 
		if(n>=1)
		{
			device_render_trap(device,&traps[0]);
		}	
		if(n>=2)
		{
			device_render_trap(device,&traps[1]);//绘制 
		}
	} 
	if(render_state&RENDER_STATE_WIREFRAME)//开启线框的状态量 
	{
		device_draw_line(device,(int)p1.x,(int)p1.y,(int)p2.x,(int)p2.y,device->foreground);
		device_draw_line(device,(int)p1.x,(int)p1.y,(int)p3.x,(int)p3.y,device->foreground);
		device_draw_line(device,(int)p3.x,(int)p3.y,(int)p2.x,(int)p2.y,device->foreground);	
	}
}