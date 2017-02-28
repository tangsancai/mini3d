#include"renderluo.h"

//=====================================================================
// 几何计算
//=====================================================================

void vertex_rhw_init(vertex_t *v)//若点不在同一深度，颜色/纹理的渐变就需要把z方向给考虑进去 
{
	float rhw=1.0f/v->pos.w;
	v->rhw=rhw;
	v->tc.u*=rhw;//纹理坐标同除点的w 
	v->tc.v*=rhw;
	v->color.r*=rhw;//RGB颜色坐标同除点的w 
	v->color.g*=rhw;
	v->color.b*=rhw;
}

void vertex_interp(vertex_t *y,const vertex_t *x1,const vertex_t *x2,float t)//顶点插值 
{
	vector_interp(&y->pos,&x1->pos,&x2->pos,t);
	y->tc.u=interp(x1->tc.u,x2->tc.u,t);
	y->tc.v=interp(x1->tc.v,x2->tc.v,t);
	y->color.r=interp(x1->color.r,x2->color.r,t);
	y->color.g=interp(x1->color.g,x2->color.g,t);
	y->color.b=interp(x1->color.b,x2->color.b,t);
	y->rhw=interp(x1->rhw,x2->rhw,t); 
}

void vertex_add(vertex_t *y,const vertex_t *x)//顶点加法 
{
	y->pos.x+=x->pos.x;
	y->pos.y+=x->pos.y;
	y->pos.z+=x->pos.z;
	y->pos.w+=x->pos.w;
	y->rhw+=x->rhw;
	y->tc.u+=x->tc.u;
	y->tc.v+=x->tc.v;
	y->color.r+=x->color.r;
	y->color.g+=x->color.g;
	y->color.b+=x->color.b;
}

//顶点除width，用以得到相应点的纹理与颜色 
void vertex_division(vertex_t *y,const vertex_t *x1,const vertex_t *x2,float w)//传进来的w为扫描线的宽度 
{
	float inv=1.0f/w;
	y->pos.x=(x2->pos.x-x1->pos.x)*inv;
	y->pos.y=(x2->pos.y-x1->pos.y)*inv;
	y->pos.z=(x2->pos.z-x1->pos.z)*inv;
	y->pos.w=(x2->pos.w-x1->pos.w)*inv;
	
	y->tc.u=(x2->tc.u-x1->tc.u)*inv;
	y->tc.v=(x2->tc.v-x1->tc.v)*inv;
	
	y->color.r=(x2->color.r-x1->color.r)*inv;
	y->color.g=(x2->color.g-x1->color.g)*inv;
	y->color.b=(x2->color.b-x1->color.b)*inv;
	
	y->rhw=(x2->rhw-x1->rhw)*inv;
}

//根据左右两边的端点，初始化计算扫描线的起点和步长
void trapezoid_init_scan_line(const trapezoid_t *trap,scanline_t *scanline,int y) 
{
	float width=trap->right.v.pos.x-trap->left.v.pos.x;//v中存储的是Y=y与left、right相交的点，当前扫描线的点 
	scanline->x=(int)(trap->left.v.pos.x+0.5f);//像素是整点，加0.5为四舍五入，得到像素点 
	scanline->w=(int)(trap->right.v.pos.x+0.5f)-scanline->x;//扫描线的宽 
	scanline->y=y;//y传进来就是整型，无需四舍五入 
	scanline->v=trap->left.v;//此处保存的是扫描线起点的浮点型；x,y中保存的是扫描线的整型 
	if(trap->left.v.pos.x>=trap->right.v.pos.x)//如果左边顶点与右边相同，则此扫描线的宽为0 
	{
		scanline->w=0;	
	} 
	vertex_division(&scanline->step,&trap->left.v,&trap->right.v,width);//计算步长，计算出来x=1，y=0，纹理坐标也是如此，RGB色彩沿扫描线均匀分布，也可计算步长 
}

//按照Y坐标计算出左右两边纵坐标等于 y 的顶点 
void trapezoid_edge_interp(trapezoid_t *trap,float y) 
{
	float s1=trap->left.v2.pos.y-trap->left.v1.pos.y;
	float s2=trap->right.v2.pos.y-trap->right.v1.pos.y;
	float t1=(y-trap->left.v1.pos.y)/s1;
	float t2=(y-trap->right.v1.pos.y)/s2;
	vertex_interp(&trap->left.v,&trap->left.v1,&trap->left.v2,t1);//trap->left.v保存的是左边上，Y=y的点 
	vertex_interp(&trap->right.v,&trap->right.v1,&trap->right.v2,t2);
}

//将三角形分割成可以扫描的水平三角形 
int trapezoid_init_triangle(trapezoid_t *trap,const vertex_t *p1,const vertex_t *p2,const vertex_t *p3)
{							//传过来的trap是数组 
	const vertex_t *p;
	float k;
	float x;

	//设置p1为最低点，p2为次低点,p3为最高点 
	if(p1->pos.y>p2->pos.y) 
	{
		p=p1;
	 	p1=p2;
	 	p2=p;
	}
	if(p1->pos.y>p3->pos.y)
	{
		p=p1;
		p1=p3;
		p3=p;
	}
	if(p2->pos.y>p3->pos.y)
	{
		p=p2;
		p2=p3;
		p3=p;
	}
	
	//不考虑有点重合的情况 
	//因为扫描线扫的是水平的，所以无需考虑点在一条垂直线上的情况，top和bottom记录的是扫描线需要扫描的垂直范围 
	//如果三点在一条水平线或垂直线上，返回梯形个数 0 
	if(p1->pos.y==p2->pos.y&&p1->pos.y==p3->pos.y)
	{
		return 0;
	}
	if(p1->pos.x==p2->pos.x&&p1->pos.x==p3->pos.x)
	{
		return 0;
	}
	
	//如果有两个点在一条水平线上，返回梯形个数 1 
	if(p1->pos.y==p2->pos.y)//triangle down 
	{	
		if(p1->pos.x>p2->pos.x)//为区分左边，右边 
		{
			p=p1;
			p1=p2;
			p2=p;
		} 
		trap[0].top=p1->pos.y;
		trap[0].bottom=p3->pos.y;
		trap[0].left.v1=*p1;
		trap[0].left.v2=*p3;
		trap[0].right.v1=*p2;
		trap[0].right.v2=*p3;
		return (trap[0].top<trap[0].bottom)?1:0; 
	}
	if(p2->pos.y==p3->pos.y)//triangle up 
	{	
		if (p2->pos.x>p3->pos.x) 
		{
			p=p2;
		 	p2=p3;
		  	p3=p;
		}
		trap[0].top=p1->pos.y;
		trap[0].bottom=p3->pos.y;
		trap[0].left.v1=*p1;
		trap[0].left.v2=*p2;
		trap[0].right.v1=*p1;
		trap[0].right.v2=*p3;
		return (trap[0].top<trap[0].bottom)?1:0;
	}
	
	//没有点在同一水平线上，就按中间点（按y分）划分为2个三角形 
	trap[0].top=p1->pos.y;
	trap[0].bottom=p2->pos.y;
	trap[1].top=p2->pos.y;
	trap[1].bottom=p3->pos.y;

	k=(p3->pos.y-p1->pos.y)/(p2->pos.y-p1->pos.y);
	x=p1->pos.x+(p2->pos.x-p1->pos.x)*k;//中间点为p2，判断三角形的形态，用以区别左边、右边 

	if(x<=p3->pos.x)//triangle left 
	{		
		trap[0].left.v1=*p1;
		trap[0].left.v2=*p2;
		trap[0].right.v1=*p1;
		trap[0].right.v2=*p3;
		trap[1].left.v1=*p2;
		trap[1].left.v2=*p3;
		trap[1].right.v1=*p1;
		trap[1].right.v2=*p3;
	}	
	else//triangle right 
	{					
		trap[0].left.v1=*p1;
		trap[0].left.v2=*p3;
		trap[0].right.v1=*p1;
		trap[0].right.v2=*p2;
		trap[1].left.v1=*p1;
		trap[1].left.v2=*p3;
		trap[1].right.v1=*p2;
		trap[1].right.v2=*p3;
	}
	return 2;
}

