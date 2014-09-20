/***************************************
功能：计算图像的直方图
输入：图像指针，存放直方图的数组首地址
Date: 2014.08.14
****************************************/

#include"cv.h"

void histogram(IplImage * img, int * hist)
{
	int width = img->width;
	int height = img->height;
	int i, j;
	uchar* imageData = (uchar*)img->imageData;

	//统计灰度级包含的像素个数
	for(i = 0; i < height; i++){
		for(j = 0;j <width;j++){
			hist[imageData[i * img->widthStep + j]]++;
		}
	}
}