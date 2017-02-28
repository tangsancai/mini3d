#include"renderluo.h"

//=====================================================================
// 坐标变换
//=====================================================================

void transform_update(transform_t *ts)
{
	matrix_t m;
	matrix_mul(&m,&ts->world,&ts->view);
	matrix_mul(&ts->transform,&m,&ts->projection);
} 

//初始化 
void transform_init(transform_t *ts,int width,int height)
{
	float aspect=(float)width/(float)height;
	matrix_set_identity(&ts->world);
	matrix_set_identity(&ts->view);
	matrix_set_perspective(&ts->projection,PI*0.5f,aspect,1.0,500.0f);
	ts->w=(float)width;
	ts->h=(float)height;
	transform_update(ts);	
}

//将点坐标x，坐标变换到CVV中，方便裁剪（方便判断是否在CVV中） 
void transform_apply(const transform_t *ts,vector_t *y,const vector_t *x)
{
	matrix_apply(y,x,&ts->transform);//变换后的坐标存在y中 
}

//检查齐次坐标同CVV的边界用于视锥体的裁剪
int transform_check_cvv(const vector_t *v) 
{
	float w=v->w;//规范化后w应该是1 
	int check=0;
	if (v->z<0.0f) 
		check|=1;//把check与1做按位或操作 
	if (v->z>w) 
		check|=2;
	if (v->x<-w) 
		check|=4;
	if (v->x>w) 
		check|=8;
	if (v->y<-w) 
		check|=16;
	if (v->y>w) 
		check|=32;
	return check;//这样一个check就可以判断出6位的情况了 
}

//归一化得到屏幕坐标
void transform_homogenize(const transform_t *ts,vector_t *y,const vector_t *x) 
{
	float rhw=1.0f/x->w;
	y->x=(x->x*rhw+1.0f)*ts->w*0.5f;
	y->y=(1.0f-x->y*rhw)*ts->h*0.5f;
	y->z=x->z*rhw;
	y->w=1.0f;
} 