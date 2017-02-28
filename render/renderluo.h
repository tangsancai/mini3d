 #ifndef RENDERLUO_H
 #define RENDERLUO_H
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <math.h>
 #include <assert.h>
 #include <windows.h>
 #include <tchar.h>

 //状态变量
 const int RENDER_STATE_WIREFRAME=1;//渲染线框
 const int RENDER_STATE_TEXTURE=2;//渲染纹理 
 const int RENDER_STATE_COLOR=4; //渲染颜色
 const int RENDER_STATE_LIGHT_GLOBAL_AMBIENT=8;//全局环境光
 const int RENDER_STATE_LIGHT_DIFFUSE_REFLECTION=16;//漫反射光


 typedef unsigned int IUINT32;
 const float PI=3.1415926f;

 typedef struct
 { 
 	float m[4][4]; 
 } matrix_t;  
 typedef struct
 {
 	float x;
 	float y;
 	float z;
 	float w;
 } vector_t;
 typedef vector_t point_t;
 
 //=====================================================================
 // 向量运算 
 //=====================================================================
 //如果x超出了边界，就选边界；如果x没有超出边界，就选x；在判断颜色值的时候用到
 int CMID(int x,int min,int max);
 //z=x+y
 void vector_add(vector_t *z,const vector_t *x,const vector_t *y);
 //z=x-y
 void vector_sub(vector_t *z,const vector_t *x,const vector_t *y);
 //数量积
 float vector_dotproduct(const vector_t *x,const vector_t *y);
 //向量积 
 void vector_crossproduct(vector_t *z,const vector_t *x,const vector_t *y);
 //计算插值：t∈[0,1] 
 float interp(float x1,float y1,float t);
 //矢量插值：t∈[0,1]
 void vector_interp(vector_t *z,const vector_t *x,const vector_t *y,float t); 
 //矢量归一化 
 void vector_normalize(vector_t *v); 
 
 //=====================================================================
 // 矩阵运算 
 //=====================================================================
 //c=a+b
 void matrix_add(matrix_t *c,const matrix_t *a,const matrix_t *b);
 //c=a-b
 void matrix_sub(matrix_t *c,const matrix_t *a,const matrix_t *b);
 //c=a*b
 void matrix_mul(matrix_t *c,const matrix_t *a,const matrix_t *b);
 //c=a*i
 void matrix_scale(matrix_t *c,const matrix_t *a,float i);
 //y=x*m;向量和矩阵相乘
 void matrix_apply(vector_t *y,const vector_t *x,const matrix_t *m); 
 //重置为单位矩阵
 void matrix_set_identity(matrix_t *m); 
 //重置为零矩阵 
 void matrix_set_zero(matrix_t *m);
 //平移变换的平移矩阵 
 void matrix_set_translate(matrix_t *m,float x,float y,float z); 
 //缩放变换的缩放矩阵
 void matrix_set_scale(matrix_t *m,float x,float y,float z); 
 //旋转变换的旋转矩阵
 void matrix_set_rotate(matrix_t *m,float x,float y,float z,float theta); 
 //摄像头设置
 void matrix_set_lookat(matrix_t *m,const vector_t *eye,const vector_t *at,const vector_t *up);
 //透视投影矩阵设置
 void matrix_set_perspective(matrix_t *m,float fovy,float aspect,float zn,float zf);
 
 //=====================================================================
 // 坐标变换 
 //=====================================================================
 typedef struct
 {
 	matrix_t world;//世界坐标系变换 
 	matrix_t view;//照相机坐标变换 
 	matrix_t projection;//投影变换 
 	matrix_t transform;//transfrom=world*view*projection 
 	float w;//宽 
 	float h;//高 
 } transform_t; 
 //矩阵更新，计算：transfrom=world*view*projection 
 void transform_update(transform_t *ts);
 //初始化，设置屏幕宽高和变换矩阵 
 void transform_init(transform_t *ts, int width, int height); 
 //将点坐标进行坐标变换
 void transform_apply(const transform_t *ts, vector_t *y, const vector_t *x); 
 //检查齐次坐标同CVV的边界用于视锥体的裁剪
 int transform_check_cvv(const vector_t *v); 
 //归一化，得到屏幕坐标 
 void transform_homogenize(const transform_t *ts, vector_t *y, const vector_t *x);
 
 //=====================================================================
 // 几何计算：顶点、扫描线、边缘、矩形、步长计算 
 //=====================================================================
 typedef struct
 {
 	float r;
 	float g;
 	float b;
 } color_t;//RGB颜色 
 typedef struct
 {
 	float u;
 	float v;
 } texcoord_t;//纹理坐标
 typedef struct
 {
 	point_t pos;
 	texcoord_t tc;
 	color_t color;
 	float rhw;//rhw=1/w，用来做深度测试用的；经过projection乘法后，w与z是线性关系，所以用z做深度测试，可以用w缓存来代替，因为除法代价太大，所以事先保存成1/w  
 } vertex_t;//顶点 
 typedef struct
 {
 	vertex_t v;//保存Y=y与left、right的交点 
 	vertex_t v1;
 	vertex_t v2;
 } edge_t;//边 
 typedef struct
 {
 	float top;//方便水平扫描线识别扫描的垂直范围 
 	float bottom;
 	edge_t left;//左边的边 
 	edge_t right;//右边的边 
 } trapezoid_t;//梯形 
 typedef struct
 {
 	vertex_t v;//v中存放的是扫描线的浮点型，也即原来left与Y=y的交点，存放当前扫描线的点 
 	vertex_t step;//存储步长 
 	int x;//x,y中存放的是扫描线起点的整型，因为像素是整型的 
 	int y;
 	int w;//扫描线的宽 
 } scanline_t;//扫描线

 //除pos坐标外，全部除rhw，颜色、纹理渐变，若点不是在同一深度，就需要把z方向上的渐变也考虑进去，所以要同除w
 void vertex_rhw_init(vertex_t *v); 
 //顶点插值
 void vertex_interp(vertex_t *y,const vertex_t *x1,const vertex_t *x2,float t);
 //步长计算有用到 
 void vertex_division(vertex_t *y,const vertex_t *x1,const vertex_t *x2,float w);
 //顶点加法 
 void vertex_add(vertex_t *y,const vertex_t *x); 
 //按照Y坐标计算出左右两边纵坐标等于 y 的顶点 
 void trapezoid_edge_interp(trapezoid_t *trap,float y); 
 //根据左右两边的端点，初始化计算扫描线的起点和步长
 void trapezoid_init_scan_line(const trapezoid_t *trap,scanline_t *scanline,int y); 
 //根据三角形生成0~2个梯形，并且返回合法梯形的数量（我的注释：将三角形分割成可以扫描的水平三角形）
 int trapezoid_init_triangle(trapezoid_t *trap,const vertex_t *p1,const vertex_t *p2,const vertex_t *p3);
 
 //=====================================================================
 // 渲染设备
 //=====================================================================
 typedef struct
 {
 	transform_t transform;//坐标变换器 
 	int width;//窗口宽度 
 	int height;//窗口高度  
 	IUINT32 **framebuffer;//像素缓存：framebuffer[y]代表第y行，存储的是颜色，颜色是32位的！！！ 
	float **zbuffer;//深度缓存
	IUINT32 **texture;//纹理 
	int tex_width;//纹理宽度 
	int tex_height;//纹理高度 
	float max_u;//纹理最大宽度：tex_width - 1 
	float max_v;//纹理最大高度：纹理坐标：v*max_v，其中v∈[0,1] 
	int render_state;//渲染状态 
	IUINT32 background;//背景颜色 
	IUINT32 foreground;//线框颜色 
//	light_t light;//光照
 } device_t;

 //设备初始化，fb为外部帧缓存，非NULL将引用外部缓存帧缓存（每行对齐4字节） 
 void device_init(device_t *device,int width,int height,void *fb);
 //删除设备
 void device_destroy(device_t *device);
 //设置当前纹理 
 void device_set_texture(device_t *device,void *bits,long pitch,int w,int h); 
 //清空framebuffer和zbuffer
 void device_clear(device_t *device,int mode);
 //画点 
 void device_pixel(device_t *device,int x,int y,IUINT32 color);
 //画线段 
 void device_draw_line(device_t *device,int x1,int y1,int x2,int y2,IUINT32 c);
 //根据坐标读取纹理 
 IUINT32 device_texture_read(const device_t *device,float u,float v);
 
 //=====================================================================
 // 渲染实现 
 //=====================================================================
 //绘制扫描线(扫描线填充算法) 
 void device_draw_scanline(device_t *device,scanline_t *scanline);
 //主渲染函数 
 void device_render_trap(device_t *device,trapezoid_t *trap);
 //画原始三角形 
 void device_draw_primitive(device_t *device,const vertex_t *v1,const vertex_t *v2,const vertex_t *v3);

 //=====================================================================
 // 光照
 //=====================================================================
 typedef struct  
 {
	 color_t light;//光照颜色
	 point_t pos;//光照位置
 } light_t;
 //全局环境光
 void global_ambient_light(device_t *device,light_t *light);
 //设置光照颜色
 void set_light_color(light_t *light,float r,float g,float b);
 //设置光照位置
 void set_light_pos(light_t *light,int x,int y,int z);
 //光照
 void open_light(device_t *device,light_t *light);
 //加载图片
 void loadbmp(const char *szfilename,IUINT32 t[256][256]);

 //=====================================================================
 // Win32 窗口及图形绘制:为device提供一个DibSection的FB
 //=====================================================================
 extern int screen_w;
 extern int screen_h;
 extern int screen_exit;
 extern int screen_mx;
 extern int screen_my;
 extern int screen_mb;
 extern int screen_keys[512];//当前键盘按下状态
 extern unsigned char *screen_fb;//frame buffer
 extern long screen_pitch;
 //屏幕初始化
 int screen_init(int w,int h,const TCHAR *title);	
 //关闭屏幕
 int screen_close();								
 //处理消息
 void screen_dispatch();							
 //显示FrameBuffer
 void screen_update();							
 // win32 event handler
 static LRESULT screen_events(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam);	
 #ifdef _MSC_VER
	#pragma comment(lib, "gdi32.lib")
	#pragma comment(lib, "user32.lib")
 #endif
 
 #endif