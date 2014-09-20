/******************************************************************
功能：识别人眼状态想关的函数
说明：
******************************************************************/


/****************************** 判断眼睛状态 ************************************************
功能：通过模糊综合评价的思想判断眼睛的状态
输入：
	double MinEyeballRectShape：眼睛矩形区域的长宽比
	double MinEyeballBlackPixelRate：眼睛矩形区域黑像素点所占的比例
	double MinEyeballBeta：眼睛中心1/2区域黑色像素点占总黑像素点的比例
输出：
	返回人眼睁开闭合的状态0：睁开，1：闭合

说明：
	1. 三个输入参数的阈值是自己设定的
	2. 输出的结果参数的阈值需要调整
	3. 为了转硬件方便，加快运算速度，将浮点运算转为了整数运算。
Date: 2014.08.22
********************************************************************************************/
#include <cv.h>

int getEyeState(double MinEyeballRectShape, double MinEyeballBlackPixelRate, double MinEyeballBeta)
{
	int eyeState;
	int funcResult;
	int shapeFuzzyLv, pixelFuzzyLv, betaFuzzyLv;	// 三个参数对应的模糊级别的值

	// 判定眼睛矩形区域的长宽比的模糊级别
	shapeFuzzyLv = 0;
	if( (MinEyeballRectShape >= 0) && (MinEyeballRectShape <= 0.8) )
		shapeFuzzyLv = 0;
	else if( MinEyeballRectShape <= 1.2 )
		shapeFuzzyLv = 2;
	else if( MinEyeballRectShape <= 1.5 )
		shapeFuzzyLv = 6;
	else if( MinEyeballRectShape <= 2.5 )
		shapeFuzzyLv = 8;
	else if( MinEyeballRectShape <= 3 )
		shapeFuzzyLv = 6;
	else
		shapeFuzzyLv = 0;

	// 判定眼睛矩形区域黑像素点所占比例的模糊级别
	pixelFuzzyLv = 0;
	if( (MinEyeballBlackPixelRate >= 0) && (MinEyeballBlackPixelRate <= 0.4) )
		pixelFuzzyLv = 0;
	else if( MinEyeballBlackPixelRate <= 0.50 )
		pixelFuzzyLv = 2;
	else if( MinEyeballBlackPixelRate <= 0.60 )
		pixelFuzzyLv = 6;
	else if( MinEyeballBlackPixelRate <= 1 )
		pixelFuzzyLv = 8;

	// 判定眼睛中心1/2区域黑色像素点占总黑像素点的比例的模糊级别
	betaFuzzyLv = 0;
	if( (MinEyeballBeta >= 0) && (MinEyeballBeta <= 0.3) )
		betaFuzzyLv = 0;
	else if( MinEyeballBeta <= 0.45 )
		betaFuzzyLv = 2;
	else if( MinEyeballBeta <= 0.6 )
		betaFuzzyLv = 6;
	else if( MinEyeballBeta <= 1 )
		betaFuzzyLv = 8;

	// 模糊评价函数
	eyeState = 1;		// 默认是闭眼的
	funcResult = 2 * shapeFuzzyLv + 4 * pixelFuzzyLv + 4 * betaFuzzyLv;
	if( funcResult >= 58 )
		eyeState = 0;

	return eyeState;
}


int FuzPatternRecoEyeState(double MinEyeballRectShape, double MinEyeballBlackPixelRate, double MinEyeballBeta)
{
	int eyeState;
	double openEyeFunResult, closeEyeFunResult;
	double R, alpha, beta;	// 三个参数对应的模糊级别的值

	/*************** 计算睁眼时候的三个模糊函数值之和 ******************/
	////////////////R
	R = 0;
	if( (MinEyeballRectShape < 0.5) && (MinEyeballRectShape >= 3) )
		R = 0;
	else if( (MinEyeballRectShape >= 0.5) && (MinEyeballRectShape < 1.5) )
		R = MinEyeballRectShape - 0.5;
	else if( (MinEyeballRectShape >= 1.5) && (MinEyeballRectShape < 2.5) )
		R = 1;
	else if( MinEyeballRectShape < 3 )
		R = (3 - MinEyeballRectShape) / 0.5;

	////////////////alpha
	alpha = 0;
	if( MinEyeballBlackPixelRate < 0.4 )
		alpha = 0;
	else if( MinEyeballBlackPixelRate <= 0.6 )
		alpha = (MinEyeballBlackPixelRate - 0.4) / 0.2;
	else
		alpha = 1;

	////////////////beta
	beta = 0;
	if( MinEyeballBeta < 0.3 )
		beta = 0;
	else if( MinEyeballBeta <= 0.6 )
		beta = (MinEyeballBeta - 0.3) / 0.3;
	else
		beta = 1;

	// 三个睁眼模糊函数值之和
	openEyeFunResult = R + alpha + beta;

	/*************** 计算闭眼时候的三个模糊函数值之和 ******************/
	////////////////R
	R = 0;
	if( MinEyeballRectShape < 0.8 )
		R = 1;
	else if( MinEyeballRectShape <= 1.5 )
		R = (2.5 - MinEyeballRectShape) / 0.7;
	else if( MinEyeballRectShape < 2.5 )
		R = 0;
	else 
		R = 1;

	// 三个睁眼模糊函数值之和
	closeEyeFunResult = R + (1 - alpha) + (1 - beta);

	
	if( openEyeFunResult >= closeEyeFunResult )
		eyeState = 0;
	else
		eyeState = 1;
	
	printf("openEyeFunResult:%f\tcloseEyeFunResult:%f\n", openEyeFunResult, closeEyeFunResult);

	return eyeState;
}


/****************************** 判断眼睛状态 ************************************************
功能：统计lMinEyeballImg中的1/2区域内黑像素的比例
输入：
	int* vertProject：垂直投影结果
	CvRect* eyeRect：最小眼眶的区域
	int width：垂直投影数列的长度
	int eyeCol：最小眼眶中虹膜的中心所在的列数
	int MinEyeballBlackPixel：最小眼眶中的黑像素综总数
输出：
	double MinEyeballBeta：眼睛中心1/2区域黑色像素点占总黑像素点的比例

说明：
	1. 
Date: 2014.08.23
********************************************************************************************/

double calMiddleAreaBlackPixRate(int* vertProject, CvRect* eyeRect, int width, int height, int eyeCol, int MinEyeballBlackPixel)
{
	// 参数
	int temp, temp1, count, i;
	double MinEyeballBeta;
	temp1 = 0;				// 中间1/2区域的白像素个数
	temp = 0;				// 中间1/2区域的黑像素个数
	count = 0;				// 临时变量
	MinEyeballBeta = 0;	// 初始化中间1/2区域的黑像素的比例
	if( width >= 8 ){
		temp = eyeCol - eyeRect->x;// 最小眼眶的虹膜中心点所在的列
		// 以下if else 是保护性代码
		if( width / 2 > temp ){
			// 防止左边界越界的情况
			count = temp + width / 4;
			temp1 = temp - width / 4;
			if( temp1 < 0 )
				i = 0;
			else
				i = temp1;
		}
		else{
			// 防止右边界越界的情况
			i = temp - width / 4;
			temp1 = temp + width / 4;
			if( temp1 < width )
				count = temp1;
			else
				count = width;
		}
		temp1 = 0;
		temp = 0;
		for( ; i < count; i ++ )
			temp1 += *(vertProject + i);
		temp1 /= 255;
		temp = height * (width / 2) - temp1;								// 中间1/2区域的黑像素个数
		MinEyeballBeta = (double)temp / (double)MinEyeballBlackPixel;		// 中间1/2区域的黑像素占所有黑像素的比例
	}
	else{
		MinEyeballBeta = 0;
	}

	return MinEyeballBeta;
}