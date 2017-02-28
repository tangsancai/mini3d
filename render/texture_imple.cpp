#include "renderluo.h"

char* gltloadbmp(const char *szfilename,int &iwidth,int &iheight)  
{  
    //打开文件   
    FILE *pfile=fopen(szfilename,"rb");  
    if(pfile==0)  
    {  
        exit(0);  
    }  
    //读取图像大小 bmp图像宽高放在前两个字节？   
    fseek(pfile,0x0012,SEEK_SET);//pfile指向偏移0x0012位，不成功则指向SEEK_SET即文件开头   
    fread(iwidth,sizeof(iwidth),1,pfile);//最多读1个元素，每个元素sizeof(iwidth)个字节，iwidth用于接收数据的内存地址   
    fread(iheight,sizeof(iheight),1,pfile);   
    //计算像素长度  
    int pixellength=iwidth*3;//3:BGR（blue、green、red）   
    while(pixellength%4!=0)//读取的时候是按int读取的，也就是4字节   
    {  
        pixellength++;  
    }  
    pixellength=pixellength*(iheight);  
    //读取像素数据  
    char *pixeldata=(char*)malloc(pixellength);  
    if(pixeldata==0)  
    {  
        exit(0);  
    }   
    fseek(pfile,54,SEEK_SET);  
    fread(pixeldata,pixellength,1,pfile);  
    //关闭文件  
    fclose(pfile);
	
    return pixeldata;  
}  