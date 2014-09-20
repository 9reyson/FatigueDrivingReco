/**************************************************
功能：检测图片中的人脸区域

输入：
	IplImage* srcImg,					// 灰度图像
	CvMemStorage* storage,				// 存储矩形框的内存区域
	double scale_factor = 1.1,			// 搜索窗口的比例系数
	int min_neighbors = 3,				// 构成检测目标的相邻矩形的最小个数
	int flags = 0,						// 操作方式
	CvSize min_size = cvSize(20, 20)	// 检测窗口的最小尺寸

输出参数：	
	CvSeq* objects				// 检测到人脸的矩形框

说明：	1. 识别的准确率和速度关键在于cvHaarDetectObject（）函数的参数的调整
		2. 如果实际用于汽车内检测效果不佳时，可考虑自己搜集汽车室内图片然后训练分类器
		3. 实际用于疲劳驾驶检测时，由于人脸位于图片的中央而且占的面积很大，可以将min_size和scale_factor调大一些，加快速度
		4. 内含直方图均衡化
**************************************************/

#include "cv.h"
#include "stdlib.h"
#include "highgui.h"

extern CvSeq* objectsTemp;		// 传递objects的值会main（）
void detectFace(
	IplImage* srcImg,					// 灰度图像
	CvSeq* objects,						// 输出参数：检测到人脸的矩形框
	CvMemStorage* storage,				// 存储矩形框的内存区域
	double scale_factor = 1.1,			// 搜索窗口的比例系数
	int min_neighbors = 3,				// 构成检测目标的相邻矩形的最小个数
	int flags = 0,						// 操作方式
	CvSize min_size = cvSize(20, 20)	// 检测窗口的最小尺寸
)
{
	// 程序用到的参数
	const char* cascadeName = "haarcascade_frontalface_alt2.xml"; // 级联分类器的xml文件名

	// 读取级联分类器xml文件
	CvHaarClassifierCascade* cascade = (CvHaarClassifierCascade*)cvLoad(cascadeName, 0, 0, 0);
	if( !cascade ) { 
        fprintf( stderr, "ERROR: Could not load classifier cascade\n" ); 
		cvWaitKey(0);
		exit(-1);
    }

	// 直方图均衡
	//cvEqualizeHist(srcImg ,srcImg); 

	// 检测人脸
	cvClearMemStorage(storage); 
    objects = cvHaarDetectObjects(	
		srcImg, 
		cascade, 
		storage, 
		scale_factor, 
		min_neighbors, 
		flags, 				/*CV_HAAR_DO_CANNY_PRUNING*/
		min_size
	);
	objectsTemp = objects;	// 为了将objects的值传递回main函数

	// 释放cascade的内存
	cvReleaseHaarClassifierCascade(&cascade);

}