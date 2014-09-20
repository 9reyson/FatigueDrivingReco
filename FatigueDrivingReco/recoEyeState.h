#pragma once

#ifndef __RECOEYESTATE_H__
#define __RECOEYESTATE_H__

// 根据模糊综合评价思想评价眼睛的睁开闭合状态
int getEyeState(double MinEyeballRectShape, double MinEyeballBlackPixelRate, double MinEyeballBeta);

// 根据模糊模式识别评价眼睛的睁开闭合状态
int FuzPatternRecoEyeState(double MinEyeballRectShape, double MinEyeballBlackPixelRate, double MinEyeballBeta);

// 统计lMinEyeballImg中的1/2区域内黑像素的比例
double calMiddleAreaBlackPixRate(int* vertProject, CvRect* eyeRect, int width, int height, int eyeCol, int MinEyeballBlackPixel);


#endif