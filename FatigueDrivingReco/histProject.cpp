/**************************************************
功能：计算图像直方图在水平方向和垂直方向的投影
输入：
	srcImg：源图像
输出：
	horiProj: 水平方向的投影结果；1 * height数组的指针，输入前记得初始化
	vertProj：垂直方向的投影结果；1 * width数组的指针，输入前记得初始化
**************************************************/

#include "cv.h"

void histProject(IplImage * srcImg, int* horiProj, int* vertProj)
{
	// 程序用到的参数
	int i, j;
	uchar* ptr = NULL;					// 指向图像当前行首地址的指针
	uchar* temp = NULL;
	int HEIGHT = srcImg->height;
	int WIDTH = srcImg->width;

	
	for(i = 0; i < HEIGHT; i ++){
		ptr = (uchar*) (srcImg->imageData + i * srcImg->widthStep);
		for(j = 0; j < WIDTH; j ++){
			temp = ptr + j;				// 减少计算量
			*(horiProj + i) += *temp;	// 计算水平方向的投影
			*(vertProj + j) += *temp;	// 计算垂直方向的投影
		}
	}

}