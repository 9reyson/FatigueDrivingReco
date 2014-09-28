/*************************************************
功能：特定功能函数——根据眼睛闭合状态和是否检测到人脸
					  判断驾驶状态：正常？疲劳？
输入：
	int eyeCloseNum：检测过程中眼睛闭状态的总次数
	int maxEyeCloseDuration：检测过程中眼睛连续闭合的最大次数
	int failFaceNum：检测过程中未检测到人脸的总次数
	int maxFailFaceDuration：检测过程中连续未检测到人脸的最大次数
说明：
Data：2014.09.22
**************************************************/
#include <stdio.h>

int eyeCloseNumTab[] = {2,2,4,6,9,14,20,29,39,50,61,72,80,86,91,94,96,98,98,99,99,100,100,100,100,100,100,100,100,100, 100};
int eyeCloseDurationTab[] = {2, 4, 9, 18, 32, 50, 68, 82, 91, 95, 98, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};
int failFaceDurationTab[] = {2, 6, 14, 29, 50, 71, 86, 94, 98, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};

int recoFatigueState(int thresh, int eyeCloseNum, int maxEyeCloseDuration, int failFaceNum, int maxFailFaceDuration)
{
	int eyeCloseValue;				// 眼睛闭合次数的贡献值
	int eyeCloseDurationValue;		// 眼睛连续闭合次数的贡献值
	int failFaceValue;				// 未检测到人脸的总次数的贡献值
	int failFaceDurationValue;		// 连续未检测到人脸的贡献值
	int compreValue;				// 综合贡献值

	// 查表计算四个指标的贡献值
	eyeCloseValue = eyeCloseNumTab[eyeCloseNum];
	eyeCloseDurationValue = eyeCloseDurationTab[maxEyeCloseDuration];
	failFaceValue = eyeCloseNumTab[failFaceNum];
	failFaceDurationValue = failFaceDurationTab[maxFailFaceDuration];
	// 综合贡献值
	compreValue = eyeCloseValue + eyeCloseDurationValue + failFaceValue + failFaceDurationValue;

	printf("\neyeCloseValue: %d\n", eyeCloseValue);
	printf("eyeCloseDurationValue: %d\n", eyeCloseDurationValue);
	printf("failFaceValue: %d\n", failFaceValue);
	printf("failFaceDurationValue: %d\n", failFaceDurationValue);
	printf("compreValue: %d\n\n", compreValue);

	return (compreValue >= thresh) ? 1 : 0;
}
