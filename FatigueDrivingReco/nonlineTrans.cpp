/********************************************************
功能：对图像进行线性点运算，实现图像增强
输入：
		IplImage* srcImg: 源灰度图像
		float a：乘系数a
输出：
		IplImage* dstImg：输出经过线性变换后的图像
********************************************************/

#include "cv.h"
#include "highgui.h"

#include "cv.h"

void nonlineTrans(IplImage* srcImg, IplImage* dstImg, float a)
{
	int i, j;
	uchar* ptr = NULL;					// 指向图像当前行首地址的指针
	uchar* pixel = NULL;				// 指向像素点的指针
	float temp;

	dstImg = cvCreateImage(cvGetSize(srcImg), IPL_DEPTH_8U, 1);
	cvCopy(srcImg, dstImg, NULL);
	int HEIGHT = dstImg->height;
	int WIDTH = dstImg->width;
	
	for(i = 0; i < HEIGHT; i ++){
		ptr = (uchar*) (srcImg->imageData + i * srcImg->widthStep);
		for(j = 0; j < WIDTH; j ++){
			pixel = ptr + j;

			// 非线性变换
			temp = *pixel + (a * (*pixel) * (255 - *pixel)) / 255;

			// 判断范围
			if ( temp > 255 )
				*pixel = 255;
			else if (temp < 0)
				*pixel = 0;
			else
				*pixel = (uchar)(temp + 0.5);// 四舍五入
		}
	}
}