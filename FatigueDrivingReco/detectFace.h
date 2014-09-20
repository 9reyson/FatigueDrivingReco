#pragma once

#ifndef __DETECTFACE_H__
#define __DETECTFACE_H__

#include"cv.h"

void detectFace(
	IplImage* srcImg,						// 灰度图像
	CvSeq* objects,						// 输出参数：检测到人脸的矩形框
	CvMemStorage* storage,				// 存储矩形框的内存区域
	double scale_factor = 1.1,			// 搜索窗口的比例系数
	int min_neighbors = 3,				// 构成检测目标的相邻矩形的最小个数
	int flags = 0,						// 操作方式
	CvSize min_size = cvSize(20, 20)	// 检测窗口的最小尺寸
);

#endif