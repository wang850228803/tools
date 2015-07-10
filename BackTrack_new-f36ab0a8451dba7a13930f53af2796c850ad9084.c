#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define DEFAULT_BITCOUNT_PER_PIXEL 32
#define DEFAULT_BYTE_PER_PIXEL (DEFAULT_BITCOUNT_PER_PIXEL/8)
#define DEFAULT_DISTANCE 500           //The real distance of the guideline
#define MARK_LENGTH 59                 //The length of 0.5 mark
#define RATIO_FOR_ZONE_ONE 225/1000    //The caution triangle shall be 22.5% of screen height for zone 1
#define RATIO_FOR_ZONE_TWO 1875/10000  //The caution triangle shall be 18.75% of screen height for zone 2
#define RATIO_FOR_ZONE_THREE 15/100    //The caution triangle shall be 15% of screen height for zone 3
#define RATIO_FOR_ZONE_FOUR 1125/10000 //The caution triangle shall be 11.25% of screen height for zone 4
#define SIN60_REVERSE 1000/1732        //The value of 1/sin(PI/3)
#define PAS_PARAM_1 5                  //The parameters for drawing PAS
#define PAS_PARAM_2 9
#define PAS_PARAM_3 36
#define PAS_PARAM_4 5
#define PAS_PARAM_5 15
#define PAS_PARAM_6 22
#define RCTA_PARAM_1 60                //The parameters for drawing RCTA
#define RCTA_PARAM_2 30
#define RCTA_PARAM_3 20
#define RCTA_PARAM_4 75
#define RCTA_PARAM_5 10

typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;
typedef unsigned char BYTE;

typedef struct tagBITMAPFILEHEADER { // bmfh
	WORD bfType;
	DWORD bfSize;
	WORD bfReserved1;
	WORD bfReserved2;
	DWORD bfOffBits;
}__attribute__ ((packed)) BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER { // bmih
	DWORD biSize;
	LONG biWidth;
	LONG biHeight;
	WORD biPlanes;
	WORD biBitCount;
	DWORD biCompression;
	DWORD biSizeImage;
	LONG biXPelsPerMeter;
	LONG biYPelsPerMeter;
	DWORD biClrUsed;
	DWORD biClrImportant;
}__attribute__ ((packed)) BITMAPINFOHEADER;

typedef struct tagCPoint {
	int x;
	int y;
} CPoint;

typedef struct tagVEHICLEDATA {
	float m_fAngle; //front wheel angle(弧度)
	float m_LRearAxle;//轴距
	float m_LRearWheel;//后轮距
	int lwidth;	//屏幕宽度
	int lheight;	//屏幕高度
	double a1;	//摄像头纵向视角的一半
	double a2;	//摄像头横向视角的一半
	double b1;	//摄像头中心线与水平线的夹角
	double h;	//摄像头高度
	double c1;	//摄像头与保险杠的纵向距离
	double x1;	//横向调整参数
	double y11;	//纵向调整参数
}VEHICLEDATA;

typedef struct tagTRACKVIEW {
	int m_iTlong;
	float m_step;
	float Ny;	//绘制的最近点
	float Fy;	//绘制的最远点
	float m_fLX[40];	//x point of left rear wheel back track
	float m_fLY[40];	//y point of left rear wheel back track
	float m_fRX[40];	//x point of right rear wheel back track
	float m_fRY[40];	//y point of left rear wheel back track
	int m_iLU[40];		//x point of left rear wheel back track in image
	int m_iLV[40];		//y point of left rear wheel back track in image
	int m_iRU[40];		//x point of right rear wheel back track in image
	int m_iRV[40];		//y point of left rear wheel back track in image
}TRACKVIEW;

VEHICLEDATA vehicleData;
TRACKVIEW viewPara;
BITMAPINFOHEADER tgtHead;

//给定一个图像位图数据、宽、高、颜色表指针及每像素所占的位数等信息,将其写到指定文件中
int saveBmp(char *bmpName, unsigned char *imgBuf, int width, int height,
		int biBitCount) {

	//待存储图像数据每行字节数为4的倍数
	int lineByte = (width * biBitCount / 8 + 3) / 4 * 4;

	FILE *fp;
	BITMAPFILEHEADER fileHead;
	BITMAPINFOHEADER head;

	//如果位图数据指针为0，则没有数据传入，函数返回
	if (!imgBuf)
		return 0;

	fp = fopen(bmpName, "wb");
	if (fp == 0)
		return 0;

	//申请位图文件头结构变量，填写文件头信息
	fileHead.bfType = 0x4D42; //bmp类型

	//bfSize是图像文件4个组成部分之和
	fileHead.bfSize = 54 + lineByte * height;

	fileHead.bfReserved1 = 0;

	fileHead.bfReserved2 = 0;

	//bfOffBits是图像文件前3个部分所需空间之和
	fileHead.bfOffBits = 54;

	fwrite(&fileHead, sizeof(BITMAPFILEHEADER), 1, fp);

	//申请位图信息头结构变量，填写信息头信息
	head.biBitCount = biBitCount;
	head.biClrImportant = 0;
	head.biClrUsed = 0;
	head.biCompression = 0;
	head.biHeight = height;
	head.biPlanes = 1;
	head.biSize = 40;
	head.biSizeImage = lineByte * height;
	head.biWidth = width;
	head.biXPelsPerMeter = 0;
	head.biYPelsPerMeter = 0;

	//写位图信息头进内存
	fwrite(&head, sizeof(BITMAPINFOHEADER), 1, fp);

	//写位图数据进文件
	fwrite(imgBuf, height * lineByte, 1, fp);

	fclose(fp);

	return 1;
}

void GetTfromView(int lWidth, int lHeight, float c1) {
	// guideline 绘制的最近点和最远点（以摄像头纵向距离为起点）
	float t;

	viewPara.Ny = c1;
	viewPara.Fy = DEFAULT_DISTANCE + c1;

	// computer jump step
	viewPara.m_step = (viewPara.Fy - viewPara.Ny) / 20.0;
	viewPara.m_iTlong = 0;
	for (t = viewPara.Ny; t < viewPara.Fy; t = t + viewPara.m_step) {
		viewPara.m_iTlong++;
	}
}

void ComputerXY(void) {
	float a, b;
	int l;

	if (sin(vehicleData.m_fAngle) != 0) {

		a = vehicleData.m_LRearAxle / (float) (tan(vehicleData.m_fAngle));
		b = (float) (sin(vehicleData.m_fAngle)) / vehicleData.m_LRearAxle;
	}

	for (l = 0; l < viewPara.m_iTlong + 1; l++) {
		if (sin(vehicleData.m_fAngle) != 0) {
			// left rear wheel back track
			viewPara.m_fLX[l] = -(a - vehicleData.m_LRearWheel / 2)
					* (float) (cos(b * (viewPara.Ny + viewPara.m_step * l))) + a;
			viewPara.m_fLY[l] = (a - vehicleData.m_LRearWheel / 2)
					* (float) (sin(b * (viewPara.Ny + viewPara.m_step * l)));

			// right rear  wheel back track
			viewPara.m_fRX[l] = -(a + vehicleData.m_LRearWheel / 2)
					* (float) (cos(b * (viewPara.Ny + viewPara.m_step * l))) + a;
			viewPara.m_fRY[l] = (a + vehicleData.m_LRearWheel / 2)
					* (float) (sin(b * (viewPara.Ny + viewPara.m_step * l)));
		} else {
			viewPara.m_fLX[l] = vehicleData.m_LRearWheel / 2;
			viewPara.m_fLY[l] = viewPara.Ny + viewPara.m_step * l;

			viewPara.m_fRX[l] = -vehicleData.m_LRearWheel / 2;
			viewPara.m_fRY[l] = viewPara.Ny + viewPara.m_step * l;
		}

	}
}

CPoint GetUVfromXY(double x, double y) {

	CPoint p;
	p.x = x * vehicleData.lwidth / 2 / (y * (float) tan(vehicleData.a2) + vehicleData.lwidth / vehicleData.x1) + vehicleData.lwidth / 2;
	p.y = vehicleData.y11 * vehicleData.lheight * (float) sin(vehicleData.a1 + vehicleData.b1 - atan(vehicleData.h / y)) / 2
			/ (float) sin(vehicleData.a1) / (float) cos(vehicleData.b1 - atan(vehicleData.h / y));

	return p;
}

void ComputerUV(void) {
	CPoint p;

	int l;

	for (l = 0; l < viewPara.m_iTlong + 1; l++) {
		// left rear wheel back track in image
		p = GetUVfromXY(viewPara.m_fLX[l], viewPara.m_fLY[l]);
		viewPara.m_iLU[l] = p.x;
		viewPara.m_iLV[l] = p.y;

		//right rear wheel back track in image
		p = GetUVfromXY(viewPara.m_fRX[l], viewPara.m_fRY[l]);
		viewPara.m_iRU[l] = p.x;
		viewPara.m_iRV[l] = p.y;
	}
}

void VehicleDataInit(float angle) {
	//除了最后两个参数，其它参数都可以在gis40文档中找到,以下使用的是默认值
	vehicleData.m_fAngle = angle;    	//front wheel angle
	vehicleData.m_LRearAxle = 294.6;   	//轴距
	vehicleData.m_LRearWheel = 272.0;    	//后轮距
	vehicleData.lwidth = DEFAULT_WIDTH;    	//屏幕宽度
	vehicleData.lheight = DEFAULT_HEIGHT;   //屏幕高度
	vehicleData.a1 = atan(50/63.04);        //摄像头纵向视角的一半
	vehicleData.a2 = atan(50/47.28);        //摄像头横向视角的一半
	vehicleData.b1 = 37*M_PI/180;           //摄像头中心线与水平线的夹角
	vehicleData.h = 109.0;                  //摄像头高度
	vehicleData.c1 = 13;                    //摄像头与保险杠的纵向距离
	vehicleData.x1 = 7;
	vehicleData.y11 = 1;

	GetTfromView(vehicleData.lwidth, vehicleData.lheight, vehicleData.c1);

	ComputerXY();
	ComputerUV();

	tgtHead.biWidth = vehicleData.lwidth;
	tgtHead.biHeight = vehicleData.lheight;
	tgtHead.biBitCount = DEFAULT_BITCOUNT_PER_PIXEL;
}

void SetColor(unsigned char *lpDIBBits, int u, int v, int color) {

	long lLineBytes = (vehicleData.lwidth * DEFAULT_BYTE_PER_PIXEL + 3) / DEFAULT_BYTE_PER_PIXEL * DEFAULT_BYTE_PER_PIXEL;
	if (color == 0) {
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + u * DEFAULT_BYTE_PER_PIXEL) = 37;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v - 1) + u * DEFAULT_BYTE_PER_PIXEL) = 37;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v + 1) + u * DEFAULT_BYTE_PER_PIXEL) = 37;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u - 1) * DEFAULT_BYTE_PER_PIXEL) = 37;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u + 1) * DEFAULT_BYTE_PER_PIXEL) = 37;

		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + u * DEFAULT_BYTE_PER_PIXEL + 1) = 193;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v - 1) + u * DEFAULT_BYTE_PER_PIXEL + 1) = 193;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v + 1) + u * DEFAULT_BYTE_PER_PIXEL + 1) = 193;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u - 1) * DEFAULT_BYTE_PER_PIXEL + 1) = 193;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u + 1) * DEFAULT_BYTE_PER_PIXEL + 1) = 193;

		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + u * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v - 1) + u * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v + 1) + u * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u - 1) * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u + 1) * DEFAULT_BYTE_PER_PIXEL + 2) = 255;

		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + u * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v - 1) + u * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v + 1) + u * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u - 1) * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u + 1) * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
	} else {
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + u * DEFAULT_BYTE_PER_PIXEL) = 0;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v - 1) + u * DEFAULT_BYTE_PER_PIXEL) = 0;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v + 1) + u * DEFAULT_BYTE_PER_PIXEL) = 0;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u - 1) * DEFAULT_BYTE_PER_PIXEL) = 0;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u + 1) * DEFAULT_BYTE_PER_PIXEL) = 0;

		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + u * DEFAULT_BYTE_PER_PIXEL + 1) = 0;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v - 1) + u * DEFAULT_BYTE_PER_PIXEL + 1) = 0;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v + 1) + u * DEFAULT_BYTE_PER_PIXEL + 1) = 0;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u - 1) * DEFAULT_BYTE_PER_PIXEL + 1) = 0;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u + 1) * DEFAULT_BYTE_PER_PIXEL + 1) = 0;

		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + u * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v - 1) + u * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v + 1) + u * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u - 1) * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u + 1) * DEFAULT_BYTE_PER_PIXEL + 2) = 255;

		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + u * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v - 1) + u * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v + 1) + u * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u - 1) * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - v) + (u + 1) * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
	}
}


void DrawBackTrack(unsigned char *lpDIBBits, int lWidth, int lHeight) {
	int t, tj;
	float k;
	int U, V;

	for (t = 0; t < viewPara.m_iTlong; t++) {
		// draw left wheel back track
		k = (float) (viewPara.m_iLU[t] - viewPara.m_iLU[t + 1])
				/ (float) (viewPara.m_iLV[t] - viewPara.m_iLV[t + 1]);
		if (k > -1 && k < 1) {
			if (viewPara.m_iLV[t] < viewPara.m_iLV[t + 1]) {
				for (tj = viewPara.m_iLV[t]; tj < viewPara.m_iLV[t + 1]; tj++) {
					U = (int) (viewPara.m_iLU[t] - k * (viewPara.m_iLV[t] - tj));
					if (U < lWidth - 1 && tj < lHeight - 1 && tj > 1 && U > 1) {
						if (t > 1)
							SetColor(lpDIBBits, U, tj, 0);
						else
							SetColor(lpDIBBits, U, tj, 1);
					}

				}

			} else {
				for (tj = viewPara.m_iLV[t]; tj > viewPara.m_iLV[t + 1]; tj--) {
					U = (int) (viewPara.m_iLU[t] - k * (viewPara.m_iLV[t] - tj));
					if (U < lWidth - 1 && tj < lHeight - 1 && tj > 1 && U > 1) {
						if (t > 1)
							SetColor(lpDIBBits, U, tj, 0);
						else
							SetColor(lpDIBBits, U, tj, 1);
					}
				}
			}
		} else {
			k = (float) (viewPara.m_iLV[t] - viewPara.m_iLV[t + 1])
					/ (float) (viewPara.m_iLU[t] - viewPara.m_iLU[t + 1]);
			if (viewPara.m_iLU[t] < viewPara.m_iLU[t + 1]) {
				for (tj = viewPara.m_iLU[t]; tj < viewPara.m_iLU[t + 1]; tj++) {
					V = (int) (viewPara.m_iLV[t] - k * (viewPara.m_iLU[t] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1) {
						if (t > 1)
							SetColor(lpDIBBits, tj, V, 0);
						else
							SetColor(lpDIBBits, tj, V, 1);
					}

				}

			} else {
				for (tj = viewPara.m_iLU[t]; tj > viewPara.m_iLU[t + 1]; tj--) {
					V = (int) (viewPara.m_iLV[t] - k * (viewPara.m_iLU[t] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1) {
						if (t > 1)
							SetColor(lpDIBBits, tj, V, 0);
						else
							SetColor(lpDIBBits, tj, V, 1);
					}
				}
			}
		}

		// draw right wheel back track
		k = (float) (viewPara.m_iRU[t] - viewPara.m_iRU[t + 1])
				/ (float) (viewPara.m_iRV[t] - viewPara.m_iRV[t + 1]);
		if (k > -1 && k < 1) {
			if (viewPara.m_iRV[t] < viewPara.m_iRV[t + 1]) {
				for (tj = viewPara.m_iRV[t]; tj < viewPara.m_iRV[t + 1]; tj++) {
					U = (int) (viewPara.m_iRU[t] - k * (viewPara.m_iRV[t] - tj));
					if (U < lWidth - 1 && tj < lHeight - 1 && tj > 1 && U > 1) {
						if (t > 1)
							SetColor(lpDIBBits, U, tj, 0);
						else
							SetColor(lpDIBBits, U, tj, 1);
					}
				}

			} else {
				for (tj = viewPara.m_iRV[t]; tj > viewPara.m_iRV[t + 1]; tj--) {
					U = (int) (viewPara.m_iRU[t] - k * (viewPara.m_iRV[t] - tj));
					if (U < lWidth - 1 && tj < lHeight - 1 && tj > 1 && U > 1) {
						if (t > 1)
							SetColor(lpDIBBits, U, tj, 0);
						else
							SetColor(lpDIBBits, U, tj, 1);
					}
				}
			}
		} else {
			k = (float) (viewPara.m_iRV[t] - viewPara.m_iRV[t + 1])
					/ (float) (viewPara.m_iRU[t] - viewPara.m_iRU[t + 1]);
			if (viewPara.m_iRU[t] < viewPara.m_iRU[t + 1]) {
				for (tj = viewPara.m_iRU[t]; tj < viewPara.m_iRU[t + 1]; tj++) {
					V = (int) (viewPara.m_iRV[t] - k * (viewPara.m_iRU[t] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1) {
						if (t > 1)
							SetColor(lpDIBBits, tj, V, 0);
						else
							SetColor(lpDIBBits, tj, V, 1);
					}
				}

			} else {
				for (tj = viewPara.m_iRU[t]; tj > viewPara.m_iRU[t + 1]; tj--) {
					V = (int) (viewPara.m_iRV[t] - k * (viewPara.m_iRU[t] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1) {
						if (t > 1)
							SetColor(lpDIBBits, tj, V, 0);
						else
							SetColor(lpDIBBits, tj, V, 1);
					}
				}
			}
		}

		//draw line between left and right wheel back track
		if ((t + 1) % 4 == 0) {
			k = (float) (viewPara.m_iLV[t + 1] - viewPara.m_iRV[t + 1])
					/ (float) (viewPara.m_iLU[t + 1] - viewPara.m_iRU[t + 1]);
			if (viewPara.m_iLU[t + 1] < viewPara.m_iRU[t + 1]) {
				for (tj = viewPara.m_iLU[t + 1]; tj < viewPara.m_iRU[t + 1]; tj++) {
					V = (int) (viewPara.m_iLV[t + 1] - k * (viewPara.m_iLU[t + 1] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1) {
						if (t > 1)
							SetColor(lpDIBBits, tj, V, 0);
						else
							SetColor(lpDIBBits, tj, V, 1);
					}
				}

			} else if (viewPara.m_iRU[t + 1] < viewPara.m_iLU[t + 1]) {
				for (tj = viewPara.m_iLU[t + 1]; tj > viewPara.m_iRU[t + 1]; tj--) {
					V = (int) (viewPara.m_iLV[t + 1] - k * (viewPara.m_iLU[t + 1] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1) {
						if (t > 1)
							SetColor(lpDIBBits, tj, V, 0);
						else
							SetColor(lpDIBBits, tj, V, 1);
					}
				}
			}
		}

		//draw 0.5m mark
		if (t == 2) {
			k = (float) (viewPara.m_iLV[t] - viewPara.m_iRV[t]) / (float) (viewPara.m_iLU[t] - viewPara.m_iRU[t]);
			if (viewPara.m_iLU[t] < viewPara.m_iRU[t]) {
				for (tj = viewPara.m_iLU[t]; tj < viewPara.m_iLU[t] + MARK_LENGTH -1; tj++) {
					V = (int) (viewPara.m_iLV[t] - k * (viewPara.m_iLU[t] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1)
						SetColor(lpDIBBits, tj, V, 1);
				}
				for (tj = viewPara.m_iRU[t] - MARK_LENGTH; tj < viewPara.m_iRU[t] + 1; tj++) {
					V = (int) (viewPara.m_iRV[t] - k * (viewPara.m_iRU[t] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1)
						SetColor(lpDIBBits, tj, V, 1);
				}

			} else if (viewPara.m_iRU[t + 1] < viewPara.m_iLU[t + 1]) {
				for (tj = viewPara.m_iRU[t]; tj < viewPara.m_iRU[t] + MARK_LENGTH -1; tj++) {
					V = (int) (viewPara.m_iRV[t] - k * (viewPara.m_iRU[t] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1)
						SetColor(lpDIBBits, tj, V, 1);
				}
				for (tj = viewPara.m_iLU[t] - MARK_LENGTH; tj < viewPara.m_iLU[t] + 1; tj++) {
					V = (int) (viewPara.m_iLV[t] - k * (viewPara.m_iLU[t] - tj));
					if (tj < lWidth - 1 && V < lHeight - 1 && tj > 1 && V > 1)
						SetColor(lpDIBBits, tj, V, 1);
				}
			}
		}

	}
}

void setSinglePix(unsigned char *lpDIBBits, int i, int j, int color) {
	long lLineBytes = (vehicleData.lwidth * DEFAULT_BYTE_PER_PIXEL + 3) / DEFAULT_BYTE_PER_PIXEL * DEFAULT_BYTE_PER_PIXEL;
		if (color == 0) {
			*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - j) + i * DEFAULT_BYTE_PER_PIXEL) = 37;
			*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - j) + i * DEFAULT_BYTE_PER_PIXEL + 1) = 193;
			*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - j) + i * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
			*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - j) + i * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		} else {
			*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - j) + i * DEFAULT_BYTE_PER_PIXEL) = 0;
			*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - j) + i * DEFAULT_BYTE_PER_PIXEL + 1) = 0;
			*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - j) + i * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
			*((unsigned char *) lpDIBBits + lLineBytes * (vehicleData.lheight - j) + i * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		}
}

//绘制三角警示符号Park Assist Symbols
//五个参数分别为存储图片像素信息的数组，三角中心的横坐标，三角中心的纵坐标，符号大小（1到4）,符号颜色（0,1）
void drawPAS(unsigned char *lpDIBBits, int px, int py, int size, int color) {
	int h, r2;
	int i, j;
	int tempy;
	float k;
	CPoint p1, p2, p3, pr;

	switch (size){
		case 1: h = DEFAULT_HEIGHT * RATIO_FOR_ZONE_ONE;
			break;
		case 2: h = DEFAULT_HEIGHT * RATIO_FOR_ZONE_TWO;
			break;
		case 3: h = DEFAULT_HEIGHT * RATIO_FOR_ZONE_THREE;
			break;
		case 4: h = DEFAULT_HEIGHT * RATIO_FOR_ZONE_FOUR;
			break;
		default:break;
	}

	p1.x = px;
	p1.y = py + h * 2 / 3;
	p2.x = px - h * SIN60_REVERSE;
	p2.y = py - h / 3;
	p3.x = px + h * SIN60_REVERSE;
	p3.y = py - h / 3;

//画三角形
	k = (float) (p1.y - p2.y) / (p1.x - p2.x);
	for (i = p2.x + PAS_PARAM_1; i< p1.x - PAS_PARAM_1 + 1; i++) {
		j = p2.y + k * (i - p2.x);
		SetColor(lpDIBBits, i, j, color);
	}

	pr.x = p1.x;
	pr.y = p1.y - PAS_PARAM_1 * 4 * SIN60_REVERSE;
	r2 = (p1.x - PAS_PARAM_1 - pr.x) * (p1.x - PAS_PARAM_1 - pr.x) + (j - pr.y) * (j - pr.y);
	for(i = p1.x - PAS_PARAM_1; i < p1.x + PAS_PARAM_1; i++) {
		j = (int)sqrt(r2 - (i - pr.x) * (i - pr.x)) + pr.y;
		SetColor(lpDIBBits, i, j, color);
	}

	k = (float) (p3.y - p1.y) / (p3.x - p1.x);
	for (i = p1.x + PAS_PARAM_1; i< p3.x - PAS_PARAM_1 + 1; i++) {
		j = p3.y + k * (i - p3.x);
		SetColor(lpDIBBits, i, j, color);
	}

	tempy = j;
	pr.x = p3.x - PAS_PARAM_2;
	pr.y = p3.y + PAS_PARAM_2 * SIN60_REVERSE;
	r2 = PAS_PARAM_2 * PAS_PARAM_2 * SIN60_REVERSE * SIN60_REVERSE;
	for (i = p3.y; i < tempy; i++) {
		j = (int)sqrt(r2 - (i - pr.y) * (i - pr.y)) + pr.x;
		SetColor(lpDIBBits, j, i, color);
	}
	pr.x = p2.x + PAS_PARAM_2;
	for (i = p3.y; i < tempy; i++) {
		j = pr.x - (int)sqrt(r2 - (i - pr.y) * (i - pr.y)) ;
		SetColor(lpDIBBits, j, i, color);
	}

	for (i = p2.x + PAS_PARAM_2; i < p3.x - PAS_PARAM_2 + 1; i++)
		SetColor(lpDIBBits, i, p2.y, color);
//画叹号
	for (i = px - h / PAS_PARAM_3; i < px + h / PAS_PARAM_3 + 1; i++)
		for (j = py - h / PAS_PARAM_4; j < py - h / PAS_PARAM_4 + 2 * h / PAS_PARAM_3 + 1; j++)
			if ((i - px) * (i - px) + (j - (py - h / PAS_PARAM_4 + h / PAS_PARAM_3)) * (j - (py - h / PAS_PARAM_4 + h / PAS_PARAM_3)) <= h * h / PAS_PARAM_3 /PAS_PARAM_3) {
				setSinglePix(lpDIBBits, i, j, color);
			}

	for (j = py - h / PAS_PARAM_5; j < py + h / 3; j++)
		for(i = px - (j - (py - h / PAS_PARAM_5))/PAS_PARAM_6; i < px + (j - (py - h / PAS_PARAM_5))/PAS_PARAM_6 + 1; i++)
			SetColor(lpDIBBits, i, j, color);
}

//Draw Rear Cross Traffic Alert Symbols,
//四个参数为存储图片像素信息的数组，三角中心的横坐标，三角中心的纵坐标，是左边还是右边(0,1)
void drawRCTA(unsigned char *lpDIBBits, int px, int py, int lr) {
	int i, j;

	drawPAS(lpDIBBits, px, py, 4, 1);

	//画箭头
	if (lr == 0) {
		for (i = px - RCTA_PARAM_1; i < px - RCTA_PARAM_2; i++)
			for (j = py; j < py + RCTA_PARAM_3; j++)
				setSinglePix(lpDIBBits, i, j, 1);

		for (i = px - RCTA_PARAM_4; i < px - RCTA_PARAM_1; i++)
			for (j = py + RCTA_PARAM_5 - i + (px - RCTA_PARAM_4); j < py + RCTA_PARAM_5 + 1 + i - (px - RCTA_PARAM_4); j++)
				setSinglePix(lpDIBBits, i, j, 1);
	} else {
		for (i = px + RCTA_PARAM_1; i > px + RCTA_PARAM_2; i--)
			for (j = py; j < py + RCTA_PARAM_3; j++)
				setSinglePix(lpDIBBits, i, j, 1);

		for (i = px + RCTA_PARAM_4; i > px + RCTA_PARAM_1; i--)
			for (j = py + RCTA_PARAM_5 + i - (px + RCTA_PARAM_4); j < py + RCTA_PARAM_5 + 1 - i + (px + RCTA_PARAM_4); j++)
				setSinglePix(lpDIBBits, i, j, 1);
	}
}


int load_guideline(float angle_rad, unsigned char* addr) {

	VehicleDataInit(angle_rad);

	memset(addr,0x0,DEFAULT_WIDTH*DEFAULT_HEIGHT*DEFAULT_BYTE_PER_PIXEL);

	DrawBackTrack(addr, tgtHead.biWidth, tgtHead.biHeight);//绘制轨迹线

	return EXIT_SUCCESS;
}

int main(void) {
	char writePath[] = "/home/wanghui/Desktop/rs.BMP";//输出的图片路径
	unsigned char pBmpBuf[(DEFAULT_WIDTH * 4 + 3)/4*4 * DEFAULT_HEIGHT];//存储图片像素信息的数组

	VehicleDataInit(M_PI*45/180);//初始化汽车各项参数（前轮转角为-M_PI/6（弧度））

	//red:0xff,green:0xff,blue:0xff => white
	memset(pBmpBuf,0xFF,sizeof(pBmpBuf));//纯白色的图片
        
        //SetAlpha(pBmpBuf);//将透明度都设为0
	DrawBackTrack(pBmpBuf, tgtHead.biWidth, tgtHead.biHeight);//绘制轨迹线
	drawPAS(pBmpBuf, 400, 240, 4, 1);//绘制三角警示符号Park Assist Symbols，五个参数分别为存储图片像素信息的数组，三角中心的横坐标，三角中心的纵坐标，符号大小（1到4）,符号颜色（0,1）

	//drawRCTA(pBmpBuf, 400, 240, 1);//Draw Rear Cross Traffic Alert Symbols, 四个参数为存储图片像素信息的数组，三角中心的横坐标，三角中心的纵坐标，是左边还是右边(0,1)

	saveBmp(writePath, pBmpBuf, tgtHead.biWidth, tgtHead.biHeight, tgtHead.biBitCount);//存储图片

	return EXIT_SUCCESS;
}
