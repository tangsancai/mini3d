#include "renderluo.h"

//=====================================================================
// 向量和矩阵计算
//=====================================================================

int CMID(int x,int min,int max) 
{ 
	return (x<min)?min:((x>max)?max:x); 
}

void vector_add(vector_t *z,const vector_t *x,const vector_t *y)
{
	z->x=x->x+y->x;
 	z->y=x->y+y->y;
 	z->z=x->z+y->z;
 	z->w=1.0f;
}

void vector_sub(vector_t *z,const vector_t *x,const vector_t *y)
{
	z->x=x->x-y->x;
 	z->y=x->y-y->y;
 	z->z=x->z-y->z;
 	z->w=1.0f;
}

float vector_dotproduct(const vector_t *x,const vector_t *y)
{
	return x->x*y->x+x->y*y->y+x->z*y->z;	//变换矩阵是n+1维的，向量多上一维是为了适应变换矩阵 
}

void vector_crossproduct(vector_t *z,const vector_t *x,const vector_t *y)
{
	z->x=x->y*y->z - x->z*y->y;
	z->y=x->z*y->x - x->x*y->z;
	z->z=x->x*y->y - x->y*y->x;
	z->w=1.0f;
}

float interp(float x,float y,float t)
{
	return x+(y-x)*t;
}

void vector_interp(vector_t *z,const vector_t *x,const vector_t *y,float t)
{
	z->x=interp(x->x,y->x,t);
	z->y=interp(x->y,y->y,t);
	z->z=interp(x->z,y->z,t);
	z->w=1.0f;
}

void vector_normalize(vector_t *v)
{
	float length=(float)sqrt(v->x*v->x+v->y*v->y+v->z*v->z);
	if(length!=0)
	{
		v->x=v->x/length;
		v->y=v->y/length;
		v->z=v->z/length;	
	}
}

void matrix_add(matrix_t *c,const matrix_t *a,const matrix_t *b)
{
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			c->m[i][j]=a->m[i][j]+b->m[i][j];
		}
	}
}

void matrix_sub(matrix_t *c,const matrix_t *a,const matrix_t *b)
{
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			c->m[i][j]=a->m[i][j]-b->m[i][j];
		}
	}
}

void matrix_mul(matrix_t *c,const matrix_t *a,const matrix_t *b)
{
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			c->m[i][j]= (a->m[i][0]*b->m[0][j])+
						(a->m[i][1]*b->m[1][j])+
						(a->m[i][2]*b->m[2][j])+
						(a->m[i][3]*b->m[3][j]);
		}
	}
}

void matrix_scale(matrix_t *c,const matrix_t *a,float f)
{
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			c->m[i][j]=a->m[i][j]*f;
		}
	}
}

//向量乘上一个矩阵；在上层调用中乘的是一个tranfrom矩阵，将坐标转化到cvv中 
void matrix_apply(vector_t *y,const vector_t *x,const matrix_t *m)
{
	y->x=x->x*m->m[0][0]+x->y*m->m[1][0]+x->z*m->m[2][0]+x->w*m->m[3][0]; 
	y->y=x->x*m->m[0][1]+x->y*m->m[1][1]+x->z*m->m[2][1]+x->w*m->m[3][1];
	y->z=x->x*m->m[0][2]+x->y*m->m[1][2]+x->z*m->m[2][2]+x->w*m->m[3][2];
	y->w=x->x*m->m[0][3]+x->y*m->m[1][3]+x->z*m->m[2][3]+x->w*m->m[3][3];
} 

void matrix_set_identity(matrix_t *m)
{
	//m矩阵为模型视图矩阵 
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			m->m[i][j]=0;
		}
	}
	m->m[0][0]=1.0f;
	m->m[1][1]=1.0f;
	m->m[2][2]=1.0f;
	m->m[3][3]=1.0f;
}

void matrix_set_zero(matrix_t *m)
{
	//m矩阵为模型视图矩阵 
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			m->m[i][j]=0;
		}
	}
}

//设置平移
void matrix_set_translate(matrix_t *m,float x,float y,float z)
{
	//m矩阵为模型视图矩阵 
	matrix_set_identity(m);					//前3列为旋转缩放向量 
	m->m[3][0]=x;							//最后一列为平移向量 
	m->m[3][1]=y;							//使用齐次坐标（3维上升为4维用以合并旋转缩放矩阵和平移向量） 
	m->m[3][2]=z;
}

//设置缩放 
void matrix_set_scale(matrix_t *m,float x,float y,float z)
{
	//m矩阵为模型视图矩阵 
	matrix_set_identity(m);
	m->m[0][0]=x;
	m->m[1][1]=y;
	m->m[2][2]=z;
} 

//设置旋转 
void matrix_set_rotate(matrix_t *m,float x,float y,float z,float theta)
{
	//m矩阵为模型视图矩阵 
	float qsin=(float)sin(theta*0.5f);
	float qcos=(float)cos(theta*0.5f);
	vector_t vec={x,y,z,1.0f};//(0,0,0)->(x,y,z)为旋转轴 
	float w=qcos;
	vector_normalize(&vec);
	x=vec.x*qsin;
	y=vec.y*qsin;
	z=vec.z*qsin;
	//因为旋转轴有点就在原点，因此无需平移了，直接旋转两次与z轴重合，旋转theta角，再反向乘上逆矩阵求回来 
	m->m[0][0]=1-2*y*y-2*z*z;//只验证了该点  
	m->m[1][0]=2*x*y-2*w*z;
	m->m[2][0]=2*x*z+2*w*y;
	m->m[0][1]=2*x*y+2*w*z;
	m->m[1][1]=1-2*x*x-2*z*z;
	m->m[2][1]=2*y*z-2*w*x;
	m->m[0][2]=2*x*z-2*w*y;
	m->m[1][2]=2*y*z+2*w*x;
	m->m[2][2]=1-2*x*x-2*y*y;
	m->m[0][3]=m->m[1][3]=m->m[2][3]=0.0f;
	m->m[3][0]=m->m[3][1]=m->m[3][2]=0.0f;	
	m->m[3][3]=1.0f;
}

//设置观察坐标系 
void matrix_set_lookat(matrix_t *m,const vector_t *eye,const vector_t *at,const vector_t *up)
{
	//eye为相机在世界坐标的位置；
	//at为相机尽头对准的物体在世界坐标的位置；
	//up为相机向上的方向在世界坐标中的位置  
	//m为模型视图矩阵，用以将物体中的物体坐标转化到世界坐标，再由世界坐标转化到眼坐标。
	//此处仿的是opengl，没有单独的模型矩阵或视图矩阵 
	vector_t xaxis;
	vector_t yaxis;
	vector_t zaxis;
	//建立眼（观察）坐标，得出眼坐标的坐标轴单位向量 
	vector_sub(&zaxis,at,eye);//设置眼坐标的z轴（面向物体方向为z轴，深度） 
	vector_normalize(&zaxis);
	vector_crossproduct(&xaxis,up,&zaxis);//通过与规定的相机向上的向量叉乘，得出x轴向量 
	vector_normalize(&xaxis);
	vector_crossproduct(&yaxis,&zaxis,&xaxis);//x轴与z轴得出向上的y轴向量 

	m->m[0][0]=xaxis.x;
	m->m[1][0]=xaxis.y;
	m->m[2][0]=xaxis.z;
	m->m[3][0]=-vector_dotproduct(&xaxis, eye);

	m->m[0][1]=yaxis.x;
	m->m[1][1]=yaxis.y;
	m->m[2][1]=yaxis.z;
	m->m[3][1]=-vector_dotproduct(&yaxis, eye);

	m->m[0][2]=zaxis.x;
	m->m[1][2]=zaxis.y;
	m->m[2][2]=zaxis.z;
	m->m[3][2]=-vector_dotproduct(&zaxis, eye);
	
	m->m[0][3]=m->m[1][3]=m->m[2][3]=0.0f;
	m->m[3][3]=1.0f;
}

//设置透视投影的投射矩阵，此处CVV中z坐标的取值为[0,1] 
void matrix_set_perspective(matrix_t *m,float fovy,float aspect,float zn,float zf) 
{
	//fovy为视角，aspect为高宽比，zn为近平面，zf为远平面 
	//这里将近平面作为投射平面 
	float fax=1.0f/(float)tan(fovy*0.5f);
	matrix_set_zero(m);
	m->m[0][0]=(float)(fax/aspect);
	m->m[1][1]=(float)(fax);
	m->m[2][2]=zf/(zf-zn);
	m->m[3][2]=-zn*zf/(zf-zn);
	m->m[2][3]=1;
}