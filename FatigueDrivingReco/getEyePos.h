#pragma once

#ifndef __GETEYEPOS_H__
#define __GETEYEPOS_H__

/******* 根据积分投影图搜索眼睛中心点的函数 *******/
int getEyePos(int* project, int size, int region);

/************************** 消除眉毛的函数 ****************************/
int removeEyebrow(int* horiProject, int width, int height, int threshold);

/********************* 计算自定义眼眶区域的函数 *************************/
void calEyeSocketRegion(CvRect* eyeRect, int width, int height, int EyeCol, int EyeRow);

/********************* 计算自定义眼眶区域的函数 *************************/
void getEyeMinRect(CvRect* eyeRect, int* horiProject, int* vertProject, int width, int height, int horiThreshold=5, int vertThreshold=3);



#endif