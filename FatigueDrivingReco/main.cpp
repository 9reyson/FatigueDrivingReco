/*************************************************************************
功能：检测人脸，检测人眼，识别人眼闭合状态，判断是否处于疲劳驾驶状态
改进：
		1. detectFace（）中用了直方图均衡化，到时看有没有必要
		2. 二值化的效果不太理想，到时用实际的驾驶图片测试再看看怎么改进。
		   二值化之前一定要做图像增强：非线性点运算或直方图均衡化。
		   在OSTU找到的最优阈值基础上减了一个常数，但减太多了，导致整张图片很灰暗的情况下二值化效果很差。
		3. detectFace子函数中有一个budge：返回的objects在子函数外被释放了！
说明：
Date：2014.08.14
**************************************************************************/

#include <highgui.h>
#include <cv.h>
#include <cxcore.h>
#include "histogram.h"
#include "memory.h"
#include "time.h"
#include "ostuThreshold.h"
#include "detectFace.h"
#include "histProject.h"
#include "linetrans.h"
#include "nonlinetrans.h"
#include "getEyePos.h"
#include "recoEyeState.h"
#include "recoFatigueState.h"

#define DETECTTIME	30								// 一次检测过程的时间长度，用检测次数衡量
#define FATIGUETHRESHOLD	180						// 判断是否疲劳驾驶的阈值

extern CvSeq* objectsTemp = NULL;					// 传递objects的值回来main（）

int main()
{
/*********************************** 主程序用到的参数 ***********************************/ 
	IplImage * srcImg = NULL;						// 存放从摄像头读取的每一帧彩色源图像
	IplImage * img = NULL;							// 存放从摄像头读取的每一帧灰度源图像
	CvCapture * capture;							// 指向CvCapture结构的指针
	CvMemStorage* storage = cvCreateMemStorage(0);	// 存放矩形框序列的内存空间
	CvSeq* objects = NULL;							// 存放检测到人脸的平均矩形框
	double scale_factor = 1.2;						// 搜索窗口的比例系数
	int min_neighbors = 3;							// 构成检测目标的相邻矩形的最小个数
	int flags = 0;									// 操作方式
	CvSize min_size = cvSize(40, 40);				// 检测窗口的最小尺寸
	int i, globalK;								
	int hist[256];									// 存放直方图的数组
	int pixelSum;
	int threshold;									// 存储二值化最优阈值
	clock_t start, stop;							// 计时参数
	IplImage* faceImg = NULL;						// 存储检测出的人脸图像
	int temp = 0;									// 临时用到的变量
	int temp1 = 0;									// 临时用到的变量
	int count = 0;									// 计数用的变量
	int flag = 0;									// 标记变量
	int * tempPtr = NULL;							// 临时指针
	CvRect* largestFaceRect;						// 存储检测到的最大的人脸矩形框
	int * horiProject = NULL;						// 水平方向的投影结果(数组指针)
	int * vertProject = NULL;						// 垂直方向的投影结果(数组指针)
	int * subhoriProject = NULL;					// 水平方向的投影结果(数组指针)
	int * subvertProject = NULL;					// 垂直方向的投影结果(数组指针)
	int WIDTH;										// 图像的宽度
	int HEIGHT;										// 图像的高度
	int rEyeCol = 0;								// 右眼所在的列数
	int lEyeCol = 0;								// 左眼所在的列数
	int lEyeRow = 0;								// 左眼所在的行数
	int rEyeRow = 0;								// 右眼所在的行数
	int eyeBrowThreshold;							// 区分眉毛与眼睛之间的阈值
	uchar* rowPtr = NULL;							// 指向图片每行的指针
	uchar* rowPtrTemp = NULL;						// 指向图片每行的指针, 中间变量
	IplImage* eyeImg = NULL;						// 存储眼睛的图像
	CvRect eyeRect;									// 存储裁剪后的人眼的矩形区域
	CvRect eyeRectTemp;								// 临时矩形区域
	IplImage* lEyeImg = NULL;						// 存储左眼的图像
	IplImage* rEyeImg = NULL;						// 存储右眼的图像
	IplImage* lEyeImgNoEyebrow = NULL;				// 存储去除眉毛之后的左眼图像
	IplImage* rEyeImgNoEyebrow = NULL;				// 存储去除眉毛之后的右眼图像
	IplImage* lEyeballImg = NULL;					// 存储最终分割的左眼框的图像
	IplImage* rEyeballImg = NULL;					// 存储最终分割的右眼框的图像
	IplImage* lMinEyeballImg = NULL;				// 存储最终分割的最小的左眼框的图像
	IplImage* rMinEyeballImg = NULL;				// 存储最终分割的最小的右眼框的图像
	int lMinEyeballBlackPixel;						// 存储最终分割的最小的左眼框的白色像素个数
	int rMinEyeballBlackPixel;						// 存储最终分割的最小的右眼框的白色像素个数
	double lMinEyeballBlackPixelRate;				// 存储最终分割的最小的左眼框的黑色像素占的比例
	double rMinEyeballBlackPixelRate;				// 存储最终分割的最小的右眼框的黑色像素占的比例
	double lMinEyeballRectShape;					// 存储最小左眼眶的矩形长宽比值
	double rMinEyeballRectShape;					// 存储最小右眼眶的矩形长宽比值
	double lMinEyeballBeta;							// 存储最小左眼眶的中间1/2区域的黑像素比值
	double rMinEyeballBeta;							// 存储最小右边眼眶的中间1/2区域的黑像素比值
	int lEyeState;									// 左眼睁（0）、闭（1）状态
	int rEyeState;									// 右眼睁（0）、闭（1）状态
	int eyeState;									// 眼睛综合睁（0）、闭（1）状态
	int eyeCloseNum = 0;							// 统计一次检测过程中闭眼的总数
	int eyeCloseDuration = 0;						// 统计一次检测过程中连续检测到闭眼状态的次数
	int maxEyeCloseDuration = 0;					// 一次检测过程中连续检测到闭眼状态的次数的最大值
	int failFaceNum = 0;							// 统计一次检测过程中未检测到人脸的总数
	int failFaceDuration = 0;						// 统计一次检测过程中连续未检测到人脸的次数
	int maxFailFaceDuration = 0;					// 一次检测过程中连续未检测到人脸的次数的最大值
	int fatigueState = 1;							// 驾驶员的驾驶状态：疲劳驾驶（1），正常驾驶（0）

	/********************* 创建显示窗口 **************************/
	cvNamedWindow("img", CV_WINDOW_AUTOSIZE);		// 显示灰度源图像
	cvNamedWindow("分割后的人脸", 1);				// 显示分割出大致眼眶区域的人脸
	cvNamedWindow("大致的左眼区域", 1);				// 显示大致的左眼区域
	cvNamedWindow("大致的右眼区域", 1);				// 显示大致的右眼区域
	cvNamedWindow("l_binary");						// 显示大致右眼区域的二值化图像
	cvNamedWindow("r_binary");						// 显示大致左眼区域的二值化图像
	cvNamedWindow("lEyeImgNoEyebrow", 1);			// 显示去除眉毛区域的左眼图像
	cvNamedWindow("rEyeImgNoEyebrow", 1);			// 显示去除眉毛区域的右眼图像
	cvNamedWindow("lEyeCenter", 1);					// 显示标出虹膜中心的左眼图像
	cvNamedWindow("rEyeCenter", 1);					// 显示标出虹膜中心的右眼图像
	cvNamedWindow("lEyeballImg", 1);				// 根据lEyeImgNoEyebrow大小的1/2区域重新划分的左眼图像
	cvNamedWindow("rEyeballImg", 1);				// 根据rEyeImgNoEyebrow大小的1/2区域重新划分的右眼图像
	cvNamedWindow("lkai", 1);						// 左眼进行开运算之后的图像
	cvNamedWindow("rkai", 1);						// 右眼进行开运算之后的图像
	cvNamedWindow("lMinEyeballImg", 1);				// 缩小至边界区域的左眼虹膜图像
	cvNamedWindow("rMinEyeballImg", 1);				// 缩小至边界区域的右眼眼虹膜图像
	
	
	capture = cvCreateCameraCapture(0);
	if( capture == NULL )
		return -1;

	for( globalK = 1; globalK <= DETECTTIME; globalK ++ ){
		start = clock();
		srcImg = cvQueryFrame(capture);
		img = cvCreateImage(cvGetSize(srcImg), IPL_DEPTH_8U, 1);
		cvCvtColor(srcImg, img, CV_BGR2GRAY);
		if( !img )
			continue;
		cvShowImage("img", img);
		cvWaitKey(20);

	/************************************* 检测人脸 ****************************************/
		cvClearMemStorage(storage);	// 将存储块的 top 置到存储块的头部，既清空存储块中的存储内容
		detectFace(
			img,					// 灰度图像
			objects,				// 输出参数：检测到人脸的矩形框
			storage,				// 存储矩形框的内存区域
			scale_factor,			// 搜索窗口的比例系数
			min_neighbors,			// 构成检测目标的相邻矩形的最小个数
			flags,					// 操作方式
			cvSize(20, 20)			// 检测窗口的最小尺寸
		);

		// 提取人脸区域
		if ( !objectsTemp->total ){
			printf("Failed to detect face!\n");		// 调试代码
			failFaceNum ++;							// 统计未检测到人脸的次数
			failFaceDuration ++;					// 统计连续未检测到人脸的次数

			// 检测过程中判断全是闭眼和检测不到人脸的情况，没有睁开眼的情况，导致maxEyeCloseDuration = 0;
			(eyeCloseDuration > maxEyeCloseDuration) ? maxEyeCloseDuration = eyeCloseDuration : maxEyeCloseDuration;
			eyeCloseDuration = 0;

			if( globalK == DETECTTIME ){
				// 当一次检测过程中，所有的过程都检测不到人脸，则要在此更新 maxFailFaceDuration 
				(failFaceDuration > maxFailFaceDuration) ? maxFailFaceDuration = failFaceDuration : maxFailFaceDuration;

				printf("\nFATIGUETHRESHOLD: %d\n", FATIGUETHRESHOLD);
				printf("eyeCloseNum: %d\tmaxEyeCloseDuration: %d\n", eyeCloseNum, maxEyeCloseDuration);
				printf("failFaceNum: %d\tmaxFailFaceDuration: %d\n", failFaceNum, maxFailFaceDuration);
				
				// 进行疲劳状态的判别
				fatigueState = recoFatigueState(FATIGUETHRESHOLD, eyeCloseNum, maxEyeCloseDuration, failFaceNum, maxFailFaceDuration);
				if( fatigueState == 1 )
					printf("驾驶员处于疲劳驾驶状态\n\n");
				else if( fatigueState == 0 )
					printf("驾驶员处于正常驾驶状态\n\n");

				// 进入下一次检测过程前，将变量清零
				globalK = 0;
				lEyeState = 1;
				rEyeState = 1;
				eyeState = 1;
				eyeCloseNum = 0;
				eyeCloseDuration = 0;
				maxEyeCloseDuration = 0;
				failFaceNum = 0;
				failFaceDuration = 0;
				maxFailFaceDuration = 0;
				fatigueState = 1;

				cvWaitKey(0);
			}

			continue;
		}
		else{
			// 统计连续未检测到人脸的次数中的最大数值
			(failFaceDuration > maxFailFaceDuration) ? maxFailFaceDuration = failFaceDuration : maxFailFaceDuration;
			failFaceDuration = 0;

			// 找到检测到的最大的人脸矩形区域
			temp = 0;
			for(i = 0; i < (objectsTemp ? objectsTemp->total : 0); i ++) {
				CvRect* rect = (CvRect*) cvGetSeqElem(objectsTemp, i);
				if ( (rect->height * rect->width) > temp ){
					largestFaceRect = rect;
					temp = rect->height * rect->width;
				}
			}

			// 根据人脸的先验知识分割出大致的人眼区域
			temp = largestFaceRect->width / 8;
			largestFaceRect->x = largestFaceRect->x + temp;
			largestFaceRect->width = largestFaceRect->width - 3*temp/2;
			largestFaceRect->height = largestFaceRect->height / 2;
			largestFaceRect->y = largestFaceRect->y + largestFaceRect->height / 2;
			largestFaceRect->height = largestFaceRect->height / 2;

			cvSetImageROI(img, *largestFaceRect);		// 设置ROI为检测到的最大的人脸区域
			faceImg = cvCreateImage(cvSize(largestFaceRect->width, largestFaceRect->height), IPL_DEPTH_8U, 1);
			cvCopy(img, faceImg, NULL);
			cvResetImageROI(img);						// 释放ROI
			cvShowImage("分割后的人脸", faceImg);

			eyeRectTemp = *largestFaceRect;
			// 根据人脸的先验知识分割出大致的左眼区域
			largestFaceRect->width /= 2;
			cvSetImageROI(img, *largestFaceRect);		// 设置ROI为检测到的最大的人脸区域
			lEyeImg = cvCreateImage(cvSize(largestFaceRect->width, largestFaceRect->height), IPL_DEPTH_8U, 1);
			cvCopy(img, lEyeImg, NULL);
			cvResetImageROI(img);						// 释放ROI
			cvShowImage("大致的左眼区域", lEyeImg);

			// 根据人脸的先验知识分割出大致的右眼区域
 			eyeRectTemp.x += eyeRectTemp.width / 2;
			eyeRectTemp.width /= 2;
			cvSetImageROI(img, eyeRectTemp);		// 设置ROI为检测到的最大的人脸区域
			rEyeImg = cvCreateImage(cvSize(eyeRectTemp.width, eyeRectTemp.height), IPL_DEPTH_8U, 1);
			cvCopy(img, rEyeImg, NULL);
			cvResetImageROI(img);						// 释放ROI
			cvShowImage("大致的右眼区域", rEyeImg);

		/********************************** 二值化处理 ***********************************/
			// 图像增强：直方图均衡化在detectFace中实现了一次；可尝试非线性点运算
			/*** 二值化左眼大致区域的图像 ***/
			//lineTrans(lEyeImg, lEyeImg, 1.5, 0);		// 线性点运算
			cvSmooth(lEyeImg, lEyeImg, CV_MEDIAN);		// 中值滤波 默认窗口大小为3*3
			nonlineTrans(lEyeImg, lEyeImg, 0.8);		// 非线性点运算
			memset(hist, 0, sizeof(hist));				// 初始化直方图的数组为0
			histogram(lEyeImg, hist);					// 计算图片直方图
			// 计算最佳阈值
			pixelSum = lEyeImg->width * lEyeImg->height;
			threshold = ostuThreshold(hist, pixelSum, 45);
			cvThreshold(lEyeImg, lEyeImg, threshold, 255, CV_THRESH_BINARY);// 对图像二值化
			// 显示二值化后的图像
			cvShowImage("l_binary",lEyeImg);

			/*** 二值化右眼大致区域的图像 ***/
			//lineTrans(rEyeImg, rEyeImg, 1.5, 0);		// 线性点运算
			cvSmooth(rEyeImg, rEyeImg, CV_MEDIAN);		// 中值滤波 默认窗口大小为3*3
			nonlineTrans(rEyeImg, rEyeImg, 0.8);		// 非线性点运算
			memset(hist, 0, sizeof(hist));				// 初始化直方图的数组为0
			histogram(rEyeImg, hist);					// 计算图片直方图
			// 计算最佳阈值
			pixelSum = rEyeImg->width * rEyeImg->height;
			threshold = ostuThreshold(hist, pixelSum, 45);
			cvThreshold(rEyeImg, rEyeImg, threshold, 255, CV_THRESH_BINARY);// 对图像二值化
			// 显示二值化后的图像
			cvShowImage("r_binary",rEyeImg);

		/***************************************** 检测人眼 ********************************************/
			/** 如果有明显的眉毛区域，则分割去除眉毛 **/

			// 分割左眼眉毛
			HEIGHT = lEyeImg->height;
			WIDTH = lEyeImg->width;
			// 分配内存
			horiProject = (int*)malloc(HEIGHT * sizeof(int));
			vertProject = (int*)malloc(WIDTH * sizeof(int));
			if( horiProject == NULL || vertProject == NULL ){
				printf("Failed to allocate memory\n");
				cvWaitKey(0);
				return -1;
			}
			// 内存置零
			for(i = 0; i < HEIGHT; i ++)
				*(horiProject + i) = 0;
			for(i = 0; i < WIDTH; i ++)
				*(vertProject + i) = 0;
			histProject(lEyeImg, horiProject, vertProject);				// 计算直方图投影
			lEyeRow = removeEyebrow(horiProject, WIDTH, HEIGHT, 10);	// 计算分割眉毛与眼框的位置

			// 分割右眼眉毛
			HEIGHT = rEyeImg->height;
			WIDTH = rEyeImg->width;
			// 分配内存
			horiProject = (int*)malloc(HEIGHT * sizeof(int));
			vertProject = (int*)malloc(WIDTH * sizeof(int));
			if( horiProject == NULL || vertProject == NULL ){
				printf("Failed to allocate memory\n");
				cvWaitKey(0);
				return -1;
			}
			// 内存置零
			for(i = 0; i < HEIGHT; i ++)
				*(horiProject + i) = 0;
			for(i = 0; i < WIDTH; i ++)
				*(vertProject + i) = 0;
			histProject(rEyeImg, horiProject, vertProject);				// 计算直方图投影
			rEyeRow = removeEyebrow(horiProject, WIDTH, HEIGHT, 10);	// 计算分割眉毛与眼框的位置

			// 显示去除眉毛后的人眼大致区域
			eyeRect = cvRect(0, lEyeRow, lEyeImg->width, (lEyeImg->height - lEyeRow));		// 去眉毛的眼眶区域在lEyeImg中的矩形框区域
			cvSetImageROI(lEyeImg, eyeRect);							// 设置ROI为去除眉毛的眼眶，在下面释放ROI
			lEyeImgNoEyebrow = cvCreateImage(cvSize(eyeRect.width, eyeRect.height), IPL_DEPTH_8U, 1);
			cvCopy(lEyeImg, lEyeImgNoEyebrow, NULL);
			cvShowImage("lEyeImgNoEyebrow", lEyeImgNoEyebrow);

			eyeRectTemp = cvRect(0, rEyeRow, rEyeImg->width, (rEyeImg->height - rEyeRow));	// 去眉毛的眼眶区域在rEyeImg中的矩形框区域
			cvSetImageROI(rEyeImg, eyeRectTemp);						// 设置ROI为去除眉毛的眼眶，在下面释放ROI
			rEyeImgNoEyebrow = cvCreateImage(cvSize(eyeRectTemp.width, eyeRectTemp.height), IPL_DEPTH_8U, 1);
			cvCopy(rEyeImg, rEyeImgNoEyebrow, NULL);
			cvShowImage("rEyeImgNoEyebrow", rEyeImgNoEyebrow);

			///////////////// 定位眼睛中心点在去除眉毛图像中的行列位置 ///////////////////
			HEIGHT = lEyeImgNoEyebrow->height;
			WIDTH = lEyeImgNoEyebrow->width;
			// 分配内存
			subhoriProject = (int*)malloc(HEIGHT * sizeof(int));
			subvertProject = (int*)malloc(WIDTH * sizeof(int));
			if( subhoriProject == NULL || subvertProject == NULL ){
				printf("Failed to allocate memory\n");
				cvWaitKey(0);
				return -1;
			}
			// 内存置零
			for(i = 0; i < HEIGHT; i ++)
				*(subhoriProject + i) = 0;
			for(i = 0; i < WIDTH; i ++)
				*(subvertProject + i) = 0;
	
			histProject(lEyeImgNoEyebrow, subhoriProject, subvertProject);	// 重新对分割出的左眼图像进行积分投影
			lEyeRow = getEyePos(subhoriProject, HEIGHT, HEIGHT/5);	// 定位左眼所在的行
			lEyeCol = getEyePos(subvertProject, WIDTH, WIDTH/5);	// 定位左眼所在的列


			HEIGHT = rEyeImgNoEyebrow->height;
			WIDTH = rEyeImgNoEyebrow->width;
			// 分配内存
			subhoriProject = (int*)malloc(HEIGHT * sizeof(int));
			subvertProject = (int*)malloc(WIDTH * sizeof(int));
			if( subhoriProject == NULL || subvertProject == NULL ){
				printf("Failed to allocate memory\n");
				cvWaitKey(0);
				return -1;
			}
			// 内存置零
			for(i = 0; i < HEIGHT; i ++)
				*(subhoriProject + i) = 0;
			for(i = 0; i < WIDTH; i ++)
				*(subvertProject + i) = 0;
			histProject(rEyeImgNoEyebrow, subhoriProject, subvertProject);	// 重新对分割出的右眼图像进行积分投影
			rEyeRow = getEyePos(subhoriProject, HEIGHT, HEIGHT/5);	// 定位右眼所在的行
			rEyeCol = getEyePos(subvertProject, WIDTH,  WIDTH/5);	// 定位右眼所在的列
			/*
			printf("************ image of eyes without eyebrow ***********\n");
			printf("Left eye: width: %d\theight: %d\n", lEyeImgNoEyebrow->width, lEyeImgNoEyebrow->height);
			printf("Right eye: width: %d\theight: %d\n", rEyeImgNoEyebrow->width, rEyeImgNoEyebrow->height);
			printf("Right eye: WIDTH: %d\tHEIGHT: %d\n", WIDTH, HEIGHT);
			printf("Centers positon of Eyes. lEyeRow: %d lEyeCol: %d\trEyeRow: %d rEyeCol: %d\n\n", lEyeRow, lEyeCol, rEyeRow, rEyeCol);
			*/
			// 标记眼睛的位置
			cvCircle(lEyeImgNoEyebrow, cvPoint(lEyeCol, lEyeRow), 3, CV_RGB(0,0,255), 1, 8, 0);
			cvCircle(rEyeImgNoEyebrow, cvPoint(rEyeCol, rEyeRow), 3, CV_RGB(0,0,255), 1, 8, 0);
			cvShowImage("lEyeCenter", lEyeImgNoEyebrow);
			cvShowImage("rEyeCenter", rEyeImgNoEyebrow);
	

		/********************************** 判断人眼睁闭状态 ***********************************/
	
			////////////////// 分割出以找到的中心为中心的大致眼眶 /////////////////
			// 左眼眶
			HEIGHT = lEyeImgNoEyebrow->height;
			WIDTH = lEyeImgNoEyebrow->width;
			// 计算大致眼眶的区域: eyeRect
			eyeRect = cvRect(0, 0, WIDTH, HEIGHT);
			calEyeSocketRegion(&eyeRect, WIDTH, HEIGHT, lEyeCol, lEyeRow);
			/*
			printf("************lEyeImgNoEyebrow************\n");
			printf("width: %d\theight: %d\n", WIDTH, HEIGHT);
			printf("**********lEyeballRect**********\n");
			printf("eyeRect.x = %d\teyeRect.width = %d\n", eyeRect.x, eyeRectTemp.width);
			printf("eyeRect.y = %d\teyeRect.height = %d\n\n", eyeRectTemp.y, eyeRectTemp.height);
			*/
			cvSetImageROI(lEyeImgNoEyebrow, eyeRect);		// 设置ROI为检测到眼眶区域
			lEyeballImg = cvCreateImage(cvGetSize(lEyeImgNoEyebrow), IPL_DEPTH_8U, 1);
			cvCopy(lEyeImgNoEyebrow, lEyeballImg, NULL);
			cvResetImageROI(lEyeImgNoEyebrow);
			cvShowImage("lEyeballImg", lEyeballImg);

			// 右眼眶
			HEIGHT = rEyeImgNoEyebrow->height;
			WIDTH = rEyeImgNoEyebrow->width;
			// 计算大致眼眶的区域: eyeRectTemp
			eyeRect = cvRect(0, 0, WIDTH, HEIGHT);
			calEyeSocketRegion(&eyeRect, WIDTH, HEIGHT, rEyeCol, rEyeRow);
			/*
			printf("************rEyeImgNoEyebrow************\n");
			printf("width: %d\theight: %d\n", WIDTH, HEIGHT);
			printf("**********rEyeballRect**********\n");
			printf("eyeRect.x = %d\teyeRect.width = %d\n", eyeRect.x, eyeRect.width);
			printf("eyeRect.y = %d\teyeRect.height = %d\n\n", eyeRect.y, eyeRect.height);
			*/
			cvSetImageROI(rEyeImgNoEyebrow, eyeRect);		// 设置ROI为检测到眼眶区域
			rEyeballImg = cvCreateImage(cvGetSize(rEyeImgNoEyebrow), IPL_DEPTH_8U, 1);
			cvCopy(rEyeImgNoEyebrow, rEyeballImg, NULL);
			cvResetImageROI(rEyeImgNoEyebrow);
			cvShowImage("rEyeballImg", rEyeballImg);

			/////////////////////////// 闭运算 ///////////////////////////
			cvErode(lEyeballImg, lEyeballImg, NULL, 2);		//腐蚀图像  
			cvDilate(lEyeballImg, lEyeballImg, NULL, 2);	//膨胀图像
			cvShowImage("lkai", lEyeballImg);

			cvErode(rEyeballImg, rEyeballImg, NULL, 1);		//腐蚀图像  
			cvDilate(rEyeballImg, rEyeballImg, NULL, 1);	//膨胀图像
			cvShowImage("rkai", rEyeballImg);

			/////////////////// 计算最小眼睛的矩形区域 ////////////////////
	
			///////////////////////////左眼
			HEIGHT = lEyeballImg->height;
			WIDTH = lEyeballImg->width;

			// 分配内存
			subhoriProject = (int*)malloc(HEIGHT * sizeof(int));
			subvertProject = (int*)malloc(WIDTH * sizeof(int));
			if( subhoriProject == NULL || subvertProject == NULL ){
				printf("Failed to allocate memory\n");
				cvWaitKey(0);
				return -1;
			}
			// 内存置零
			for(i = 0; i < HEIGHT; i ++)
				*(subhoriProject + i) = 0;
			for(i = 0; i < WIDTH; i ++)
				*(subvertProject + i) = 0;
			histProject(lEyeballImg, subhoriProject, subvertProject);
			// 计算左眼最小的矩形区域
			eyeRectTemp = cvRect(0, 0 , 1, 1);		// 初始化
			getEyeMinRect(&eyeRectTemp, subhoriProject, subvertProject, WIDTH, HEIGHT, 5, 3);
			/*
			printf("eyeRectTemp.y: %d\n", eyeRectTemp.y);
			printf("eyeRectTemp.height: %d\n", eyeRectTemp.height);
			printf("eyeRectTemp.x: %d\n", eyeRectTemp.x);
			printf("eyeRectTemp.width: %d\n", eyeRectTemp.width);
			*/
			// 计算最小左眼矩形的长宽比,  判断眼睛状态时用的到
			lMinEyeballRectShape = (double)eyeRectTemp.width / (double)eyeRectTemp.height;
			//printf("\nlMinEyeballRectShape: %f\n", lMinEyeballRectShape);

			cvSetImageROI(lEyeballImg, eyeRectTemp);		// 设置ROI为检测到最小面积的眼眶
			lMinEyeballImg = cvCreateImage(cvGetSize(lEyeballImg), IPL_DEPTH_8U, 1);
			cvCopy(lEyeballImg, lMinEyeballImg, NULL);
			cvResetImageROI(lEyeballImg);
			cvShowImage("lMinEyeballImg", lMinEyeballImg);

			////////////////////////  统计左眼黑像素个数  /////////////////////
			HEIGHT = lMinEyeballImg->height;
			WIDTH = lMinEyeballImg->width;

			// 分配内存
			subhoriProject = (int*)malloc(HEIGHT * sizeof(int));
			subvertProject = (int*)malloc(WIDTH * sizeof(int));
			if( subhoriProject == NULL || subvertProject == NULL ){
				printf("Failed to allocate memory\n");
				cvWaitKey(0);
				return -1;
			}
			// 内存置零
			for(i = 0; i < HEIGHT; i ++)
				*(subhoriProject + i) = 0;
			for(i = 0; i < WIDTH; i ++)
				*(subvertProject + i) = 0;

			histProject(lMinEyeballImg, subhoriProject, subvertProject);

			// 统计lEyeballImg中黑色像素的个数
			temp = 0;	// 白像素个数
			for( i = 0; i < WIDTH; i ++ )
				temp += *(subvertProject + i);
			temp /= 255;
			lMinEyeballBlackPixel = WIDTH * HEIGHT - temp;
			lMinEyeballBlackPixelRate = (double)lMinEyeballBlackPixel / (double)(WIDTH * HEIGHT);
			//printf("WIDTH * HEIGHT: %d\tlMinEyeballBlackSum;%d\n\n", WIDTH * HEIGHT, lMinEyeballBlackPixel);
			//printf("lMinEyeballBlackPixelRate;%f\n\n", lMinEyeballBlackPixelRate);

			// 统计lMinEyeballImg中的1/2区域内黑像素的比例
			lMinEyeballBeta = 0;
			lMinEyeballBeta = calMiddleAreaBlackPixRate(subvertProject, &eyeRectTemp, WIDTH, HEIGHT, lEyeCol, lMinEyeballBlackPixel);

			//printf("lMinEyeballBeta; %f\n\n", lMinEyeballBeta);



			////////////////////////////////////右眼
			HEIGHT = rEyeballImg->height;
			WIDTH = rEyeballImg->width;
			// 分配内存
			subhoriProject = (int*)malloc(HEIGHT * sizeof(int));
			subvertProject = (int*)malloc(WIDTH * sizeof(int));
			if( subhoriProject == NULL || subvertProject == NULL ){
				printf("Failed to allocate memory\n");
				cvWaitKey(0);
				return -1;
			}
			// 内存置零
			for(i = 0; i < HEIGHT; i ++)
				*(subhoriProject + i) = 0;
			for(i = 0; i < WIDTH; i ++)
				*(subvertProject + i) = 0;
			histProject(rEyeballImg, subhoriProject, subvertProject);

			// 计算右眼最小的矩形区域
			eyeRectTemp = cvRect(0, 0 , 1, 1);
			getEyeMinRect(&eyeRectTemp, subhoriProject, subvertProject, WIDTH, HEIGHT, 5, 3);

			// 计算最小右眼矩形的长宽比，判断眼睛状态时用的到
			rMinEyeballRectShape = (double)eyeRectTemp.width / (double)eyeRectTemp.height;
			//printf("\nrMinEyeballRectShape: %f\n", rMinEyeballRectShape);

			cvSetImageROI(rEyeballImg, eyeRectTemp);		// 设置ROI为检测到最小面积的眼眶
			rMinEyeballImg = cvCreateImage(cvGetSize(rEyeballImg), IPL_DEPTH_8U, 1);
			cvCopy(rEyeballImg, rMinEyeballImg, NULL);
			cvResetImageROI(rEyeballImg);
			cvShowImage("rMinEyeballImg", rMinEyeballImg);

			////////////////////////  统计右眼黑像素个数  /////////////////////
			HEIGHT = rMinEyeballImg->height;
			WIDTH = rMinEyeballImg->width;

			// 分配内存
			subhoriProject = (int*)malloc(HEIGHT * sizeof(int));
			subvertProject = (int*)malloc(WIDTH * sizeof(int));
			if( subhoriProject == NULL || subvertProject == NULL ){
				printf("Failed to allocate memory\n");
				cvWaitKey(0);
				return -1;
			}
			// 内存置零
			for(i = 0; i < HEIGHT; i ++)
				*(subhoriProject + i) = 0;
			for(i = 0; i < WIDTH; i ++)
				*(subvertProject + i) = 0;
			histProject(rMinEyeballImg, subhoriProject, subvertProject);// 计算直方图积分投影

			// 统计lEyeballImg中黑色像素的个数
			temp = 0;
			for( i = 0; i < WIDTH; i ++ )
				temp += *(subvertProject + i);
			temp /= 255;
			rMinEyeballBlackPixel = WIDTH * HEIGHT - temp;
			rMinEyeballBlackPixelRate = (double)rMinEyeballBlackPixel / (double)(WIDTH * HEIGHT);
			//printf("WIDTH * HEIGHT: %d\trMinEyeballBlackSum;%d\n\n", WIDTH * HEIGHT, rMinEyeballBlackPixel);
			//printf("rMinEyeballBlackPixelRate; %f\n\n", rMinEyeballBlackPixelRate);

			// 统计lMinEyeballImg中的1/2区域内黑像素的比例
			rMinEyeballBeta = 0;
			rMinEyeballBeta = calMiddleAreaBlackPixRate(subvertProject, &eyeRectTemp, WIDTH, HEIGHT, rEyeCol, rMinEyeballBlackPixel);

			//printf("temp:%d\trMinEyeballBeta; %f\n\n", temp, rMinEyeballBeta);

			// 判断眼睛睁闭情况
			lEyeState = 1;		// 左眼状态，默认闭眼
			rEyeState = 1;		// 右眼状态，默认闭眼
			eyeState = 1;		// 眼睛综合状态，默认闭眼
			if( lMinEyeballBlackPixel > 50)
				lEyeState = getEyeState(lMinEyeballRectShape, lMinEyeballBlackPixelRate, lMinEyeballBeta);
			else
				lEyeState = 1;

			if( rMinEyeballBlackPixel > 50)
				rEyeState = getEyeState(rMinEyeballRectShape, rMinEyeballBlackPixelRate, rMinEyeballBeta);
			else
				rEyeState = 1;
			(lEyeState + rEyeState) == 2 ? eyeState = 1 : eyeState=0;

			// 统计眼睛闭合的次数
			if( eyeState == 1 ){
				eyeCloseNum ++;					// 统计 eyeCloseNum 眼睛闭合次数
				eyeCloseDuration ++;
				if( globalK == DETECTTIME){
					// 检测过程中判断全是闭眼情况，没有睁眼和检测不到人脸的情况
					(eyeCloseDuration > maxEyeCloseDuration) ? maxEyeCloseDuration = eyeCloseDuration : maxEyeCloseDuration;
					eyeCloseDuration = 0;
				}
			}
			else{
				(eyeCloseDuration > maxEyeCloseDuration) ? maxEyeCloseDuration = eyeCloseDuration : maxEyeCloseDuration;
				eyeCloseDuration = 0;
			}
		} // 承接判断是否检测到人脸的if语句

	/*	
		printf("\n************** 眼睛状态 ***************\n");
		printf("lEyeState: %d\trEyeState: %d\n", lEyeState, rEyeState);
		printf("eyeState: %d\n\n\n\n", eyeState);
	*/

		// 计时：执行一次循环的时间
		stop = clock();
		//printf("run time: %f\n", (double)(stop - start) / CLOCKS_PER_SEC);

		printf("eyeState: %d\n", eyeState);

		// 调整循环变量，进入下一次检测过程
		if( globalK == DETECTTIME ){
			printf("\nFATIGUETHRESHOLD*****: %d\n", FATIGUETHRESHOLD);
			printf("eyeCloseNum: %d\tmaxEyeCloseDuration: %d\n", eyeCloseNum, maxEyeCloseDuration);
			printf("failFaceNum: %d\tmaxFailFaceDuration: %d\n", failFaceNum, maxFailFaceDuration);

			// 进行疲劳状态的判别
			fatigueState = recoFatigueState(FATIGUETHRESHOLD, eyeCloseNum, maxEyeCloseDuration, failFaceNum, maxFailFaceDuration);
			if( fatigueState == 1 )
				printf("驾驶员处于疲劳驾驶状态\n\n");
			else if( fatigueState == 0 )
				printf("驾驶员处于正常驾驶状态\n\n");

			// 进入下一次检测过程前，将变量清零
			globalK = 0;
			lEyeState = 1;
			rEyeState = 1;
			eyeState = 1;
			eyeCloseNum = 0;
			eyeCloseDuration = 0;
			maxEyeCloseDuration = 0;
			failFaceNum = 0;
			failFaceDuration = 0;
			maxFailFaceDuration = 0;
			fatigueState = 1;
			char c = cvWaitKey(0);
			if( c == 27 )
				break;
			else
				continue;
		}
	} // 承接检测过程的 for 循环

	// 释放内存
	cvDestroyWindow("分割后的人脸");
	cvDestroyWindow("大致的左眼区域");
	cvDestroyWindow("大致的右眼区域");
	cvDestroyWindow("l_binary");
	cvDestroyWindow("r_binary");
	cvDestroyWindow("lEyeImgNoEyebrow");
	cvDestroyWindow("rEyeImgNoEyebrow");
	cvDestroyWindow("lEyeCenter");
	cvDestroyWindow("rEyeCenter");	
	cvDestroyWindow("lEyeballImg");
	cvDestroyWindow("rEyeballImg");
	cvDestroyWindow("lkai");
	cvDestroyWindow("rkai");
	cvDestroyWindow("lMinEyeballImg");
	cvDestroyWindow("rMinEyeballImg");
	cvReleaseMemStorage(&storage);
	cvReleaseImage(&eyeImg);
	free(horiProject);
	free(vertProject);
	free(subhoriProject);
	free(subvertProject);

	return 0;
}