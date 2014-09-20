/************************************************************
功能：找出数列中限定区域内的最低点的位置
输入：
	int* project:  数列
	int size: 数列的长度
	int region: 中间区域的范围
输出：
	返回int结果位置, 错误时返回-1
说明：
	1. 没有考虑相同投影值但是不同index的情况怎么处理？？
	2. 可考虑加入原图像参数，消除头发大片黑图像的影响！！
Date: 2014.08.19
************************************************************/

#include <cv.h>
#include <stdlib.h>


typedef struct
{
	int data;
	int index; 
}projectArr;

// qsort的函数参数
int cmpInc( const void *a ,const void *b)
{
	return (*(projectArr *)a).data - (*(projectArr *)b).data;
}


int getEyePos(int* project, int size, int region)
{
	// 参数
	projectArr* projectStruct = NULL;
	projectArr* projectTemp = NULL;
	int i, j, pos, sizeTemp, temp;

	// 分配projectStruct内存空间
	projectStruct = (projectArr*)malloc(size * sizeof(projectArr));
	projectTemp = (projectArr*)malloc(sizeof(projectArr));

	// 初始化内存空间
	for(i = 0; i < size; i ++){
		(projectStruct + i)->data  = *(project + i);
		(projectStruct + i)->index  = i;
	}

	// 对project从小到大快速排序
	//qsort(projectStruct, size, sizeof(*project), cmpInc);
	for(i = 0; i <= size - 2; i ++){
		for( j = 0; j < size - i - 1; j ++ ){
			if( (projectStruct + j)->data > (projectStruct + j + 1)->data ){
				*projectTemp = *(projectStruct + j);
				*(projectStruct + j) = *(projectStruct + j + 1);
				*(projectStruct + j + 1) = *projectTemp;
			}
		}
	}

	// 寻找中间区域的最小值及其位置
	sizeTemp = size / 2;
	temp = 0;
	for( i = 0; i < size; i ++ ){
		temp = (projectStruct+i)->index;
		if( (temp > sizeTemp - region) && (temp < sizeTemp + region) ){
			pos = (projectStruct + i)->index;
			// 防止指针越界访问位置元素出现负数
			if( pos < 0)
				return -1;
			break;
		}
		else{
			// 防止整个数列不存在符合条件的元素
			if( i == size - 1 )
				return -1;
		}
	}

	//free(projectStruct);
	free(projectTemp);
	return pos;
}


/************************************************************
功能：搜索积分投影图中的最低点，从而消除眉毛的函数
输入：
	int* horiProject: 数列的指针
	int width:  数列的宽度
	int height: 数列的高度
	int threshold：分割眉毛的阈值，最多
输出：
	返回找到的最低点行位置，结果为int类型，即眉毛与眼睛的分割线
说明：
		1. 消除眉毛时可以调整eyeBrowThreshold来调整去除的效果
		2. 同时可以调整连续大于阈值的次数count来调整效果。
Date: 2014.08.23
************************************************************/

int removeEyebrow(int* horiProject, int width, int height, int threshold)
{
	//  参数
	int temp, temp1, count, flag, i;
	int eyeRow;
	int eyeBrowThreshold;

	// 定位人眼位置
	eyeBrowThreshold = (width - threshold) * 255;			// 为了防止无法区分眼睛和眉毛的情况，可适当降低阈值

	// 消除眉毛区域
	temp = 100000000;
	temp1 = 0;
	count = 0;
	flag = 0;										// 表示当前搜索的位置在第一个最低谷之前
	eyeRow = 0;
	for(i = 0; i < height; i = i + 3){
		count ++;
		temp1 = *(horiProject + i) + *(horiProject + i + 1) + *(horiProject + i + 2);
		if( (temp1 < temp) & (flag == 0) ){
			temp = temp1;
			eyeRow = i;
			count = 0;
		}
		if (count >= 3 || i >= height - 2){
			flag = 1;
			break;
		}
	}

	// 搜索第一个大于眼睛与眉毛分割阈值的点
	count = 0;
	for( i = eyeRow; i < height; i ++ ){
		if( *(horiProject + i) > eyeBrowThreshold){
			eyeRow = i;
			count ++;
			if( count >= 3 ){				// count: 统计共有多少连续的行的投影值大于阈值；
				eyeRow = i;
				break;
			}
		}
		else
			count = 0;
	}

	// 防止没有眉毛错删眼睛的情况，可根据实验结果调整参数！
	if( eyeRow >= height / 2 )
		eyeRow = 0;

	return eyeRow;
}

/************************************************************
功能：特定功能函数：根据人眼的中心大致计算眼眶的区域
输入：
	CvRect* eyeRect: 眼眶矩形区域的指针
	int width:  数列的宽度
	int height: 数列的高度
	int EyeCol：虹膜中心所在的列位置
	int EyeRow：虹膜中心所在的行位置
输出：
	以指针的方式返回眼眶的大致区域，eyeRect
说明：
		
Date: 2014.08.23
************************************************************/

void calEyeSocketRegion(CvRect* eyeRect, int width, int height, int EyeCol, int EyeRow)
{
	// 参数
	int temp, temp1;
	temp = EyeCol - width / 4;
	temp1 = EyeRow - height / 4;
	if( (temp < 0) && (temp1 < 0) ){
		eyeRect->x = 0;
		eyeRect->width = width / 2 + temp;
		eyeRect->y = 0;
		eyeRect->height = height / 2 + temp1;
	}
	else if( (temp < 0) && (temp1 > 0) ){
		eyeRect->x = 0;
		eyeRect->width = width / 2 + temp;
		eyeRect->y = temp1;
		eyeRect->height = height / 2;
	}
	else if( (temp > 0) && (temp1 < 0) ){
		eyeRect->x = temp;
		eyeRect->width = width / 2;
		eyeRect->y = 0;
		eyeRect->height = height / 2 + temp1;
	}
	else if( (temp > 0) && (temp1 > 0) ){
		eyeRect->x = temp;
		eyeRect->width = width / 2;
		eyeRect->y = temp1;
		eyeRect->height = height / 2;
	}
}

/************************************************************
功能：特定功能函数：计算人眼最小的矩形区域
输入：
	CvRect* eyeRect: 人眼最小的矩形区域的指针
	int* horiProject
	int* vertProject
	int width:  数列的宽度
	int height: 数列的高度
	int horiThreshold：水平方向的阈值
	int vertThreshold：垂直方向的阈值
输出：
	通过指针返回CvRect* eyeRect: 人眼最小的矩形区域的指针
说明：
		1. 
		2. 
Date: 2014.08.23
************************************************************/

void getEyeMinRect(CvRect* eyeRect, int* horiProject, int* vertProject, int width, int height, int horiThreshold=5, int vertThreshold=3)
{
	// 参数
	int temp, temp1, i;

	temp1 = (width - horiThreshold) * 255;
	for(i = 0; i < height; i ++){
		if( *(horiProject + i) < temp1 ){
			eyeRect->y = i;
			break;
		}
	}
	temp = i;				// 记录eyeRectTemp.y的位置
	printf("eyeRectTemp->y: %d\n", eyeRect->y);

	if( temp != height ){	
	// temp != HEIGHT: 防止没有符合*(subhoriProject + i) < temp1条件的位置；如果temp != HEIGHT则一定有满足条件的位置存在
		for(i = height-1; i >= 0; i --){
			if( *(horiProject + i) < temp1 ){
				temp = i;
				break;
			}
		}
		if( temp == eyeRect->y )
			eyeRect->height = 1;
		else
			eyeRect->height = temp - eyeRect->y;
	}
	else{
		eyeRect->height = 1;
	}
	printf("eyeRectTemp.height: %d\n", eyeRect->height);
	
	temp1 = (height - vertThreshold) * 255;
	for( i = 0; i < width; i ++ ){
		if( *(vertProject + i) < temp1 ){
			eyeRect->x = i;
			break;
		}
	}
	temp = i;			// 记录eyeRectTemp.x的位置
	printf("eyeRectTemp.x: %d\n", eyeRect->x);

	if( temp != width ){
		for(i = width-1; i >= 0; i --){
			if( *(vertProject + i) < temp1 ){
				temp = i;
				break;
			}
		}
		// 防止宽度为0，显示图像时出错！
		if( temp == eyeRect->x )
			eyeRect->width = 1;
		else
			eyeRect->width = temp - eyeRect->x;
	}
	else{
		eyeRect->width = 1;
	}
	printf("eyeRectTemp.width: %d\n", eyeRect->width);
}