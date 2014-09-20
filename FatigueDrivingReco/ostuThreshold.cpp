/******************************************************
功能：用Ostu最大类间方差法计算二值化阈值
输入：
	hist：图像的直方图数组
	pixelSum：图像的像素总和
	CONST: 一个常数；为了适应各种特殊的要求，可实现在找到的最优分割阈值的基础上减去该常数
输出：
	threshold：最优阈值
Date: 2014.08.14
******************************************************/
#pragma once

#include <stdio.h>

int ostuThreshold(int * hist, int pixelSum, const int CONST)
{
	float pixelPro[256];
	int i, j, threshold = 0;

	//计算每个像素在整幅图像中的比例
	for(i = 0; i < 256; i++){
		*(pixelPro+i) = (float)(*(hist+i)) / (float)(pixelSum);
	}

	//经典ostu算法,得到前景和背景的分割
	//遍历灰度级[0,255],计算出方差最大的灰度值,为最佳阈值
	float w0, w1, u0tmp, u1tmp, u0, u1, u,deltaTmp, deltaMax = 0;
	for(i = 0; i < 256; i++){
		w0 = w1 = u0tmp = u1tmp = u0 = u1 = u = deltaTmp = 0;

		for(j = 0; j < 256; j++){
			if(j <= i){			//背景部分
				//以i为阈值分类，第一类总的概率
				w0 += *(pixelPro+j);		
				u0tmp += j * (*(pixelPro+j));
			}
			else				//前景部分
			{
				//以i为阈值分类，第二类总的概率
				w1 += *(pixelPro+j);		
				u1tmp += j * (*(pixelPro+j));
			}
		}

		u0 = u0tmp / w0;		//第一类的平均灰度
		u1 = u1tmp / w1;		//第二类的平均灰度
		u = u0tmp + u1tmp;		//整幅图像的平均灰度
		//计算类间方差
		deltaTmp = w0 * (u0 - u)*(u0 - u) + w1 * (u1 - u)*(u1 - u);
		//找出最大类间方差以及对应的阈值
		if(deltaTmp > deltaMax){	
			deltaMax = deltaTmp;
			threshold = i;
		}
	}
	printf("Ostu Threshold: %d\n", threshold);
	printf("real Threshold: %d\n", threshold - CONST);
	//返回最佳阈值;
	return (threshold - CONST);
}