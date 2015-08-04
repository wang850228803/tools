#include <stdio.h>
#include <string.h>

#define SCREEN_COLS             700  /* how many columns does terminal have */
#define HORIZONTAL_DISTANCE     4   /* horizontal distance between two characters (列距) */
#define VERTICAL_DISTANCE       1   /* vertical distance  between two rows of characters (行距) */
#define BRUSH_CHAR              ('*')
#define BLANK_CHAR              (' ')

/* ASCII_TAB字模中字体的高度和宽度 */
#define FONT_ROWS               7
#define FONT_COLS               5

#define DEFAULT_BYTE_PER_PIXEL 4
#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 480

/* 屏幕每行最多可以显示的字符个数 */
#define CHAR_PER_LINE           (SCREEN_COLS/(FONT_COLS + HORIZONTAL_DISTANCE))
// ASCII_TAB[] contains all ASCII characters from sp (32) to z (122)

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

BITMAPINFOHEADER tgtHead;

int xCount = 0;
int yCount = 0;

static const unsigned char ASCII_TAB[][5]=  //5*7
{
    { 0x00, 0x00, 0x00, 0x00, 0x00 },   // sp
    { 0x00, 0x00, 0x2f, 0x00, 0x00 },   // !
    { 0x00, 0x07, 0x00, 0x07, 0x00 },   // "
    { 0x14, 0x7f, 0x14, 0x7f, 0x14 },   // #
    { 0x24, 0x2a, 0x7f, 0x2a, 0x12 },   // $
    { 0xc4, 0xc8, 0x10, 0x26, 0x46 },   // %
    { 0x36, 0x49, 0x55, 0x22, 0x50 },   // &
    { 0x00, 0x05, 0x03, 0x00, 0x00 },   // '
    { 0x00, 0x1c, 0x22, 0x41, 0x00 },   // (
    { 0x00, 0x41, 0x22, 0x1c, 0x00 },   // )
    { 0x14, 0x08, 0x3E, 0x08, 0x14 },   // *
    { 0x08, 0x08, 0x3E, 0x08, 0x08 },   // +
    { 0x00, 0x00, 0x50, 0x30, 0x00 },   // ,
    { 0x10, 0x10, 0x10, 0x10, 0x10 },   // -
    { 0x00, 0x60, 0x60, 0x00, 0x00 },   // .
    { 0x20, 0x10, 0x08, 0x04, 0x02 },   // /

    { 0x3E, 0x51, 0x49, 0x45, 0x3E },   // 0
    { 0x00, 0x42, 0x7F, 0x40, 0x00 },   // 1
    { 0x42, 0x61, 0x51, 0x49, 0x46 },   // 2
    { 0x21, 0x41, 0x45, 0x4B, 0x31 },   // 3
    { 0x18, 0x14, 0x12, 0x7F, 0x10 },   // 4
    { 0x27, 0x45, 0x45, 0x45, 0x39 },   // 5
    { 0x3C, 0x4A, 0x49, 0x49, 0x30 },   // 6
    { 0x01, 0x71, 0x09, 0x05, 0x03 },   // 7
    { 0x36, 0x49, 0x49, 0x49, 0x36 },   // 8
    { 0x06, 0x49, 0x49, 0x29, 0x1E },   // 9

    { 0x00, 0x36, 0x36, 0x00, 0x00 },   // :
    { 0x00, 0x56, 0x36, 0x00, 0x00 },   // ;
    { 0x08, 0x14, 0x22, 0x41, 0x00 },   // <
    { 0x14, 0x14, 0x14, 0x14, 0x14 },   // =
    { 0x00, 0x41, 0x22, 0x14, 0x08 },   // >
    { 0x02, 0x01, 0x51, 0x09, 0x06 },   // ?
    { 0x32, 0x49, 0x59, 0x51, 0x3E },   // @

    { 0x7E, 0x11, 0x11, 0x11, 0x7E },   // A
    { 0x7F, 0x49, 0x49, 0x49, 0x36 },   // B
    { 0x3E, 0x41, 0x41, 0x41, 0x22 },   // C
    { 0x7F, 0x41, 0x41, 0x22, 0x1C },   // D
    { 0x7F, 0x49, 0x49, 0x49, 0x41 },   // E
    { 0x7F, 0x09, 0x09, 0x09, 0x01 },   // F
    { 0x3E, 0x41, 0x49, 0x49, 0x7A },   // G
    { 0x7F, 0x08, 0x08, 0x08, 0x7F },   // H
    { 0x00, 0x41, 0x7F, 0x41, 0x00 },   // I
    { 0x20, 0x40, 0x41, 0x3F, 0x01 },   // J
    { 0x7F, 0x08, 0x14, 0x22, 0x41 },   // K
    { 0x7F, 0x40, 0x40, 0x40, 0x40 },   // L
    { 0x7F, 0x02, 0x0C, 0x02, 0x7F },   // M
    { 0x7F, 0x04, 0x08, 0x10, 0x7F },   // N
    { 0x3E, 0x41, 0x41, 0x41, 0x3E },   // O
    { 0x7F, 0x09, 0x09, 0x09, 0x06 },   // P
    { 0x3E, 0x41, 0x51, 0x21, 0x5E },   // Q
    { 0x7F, 0x09, 0x19, 0x29, 0x46 },   // R
    { 0x46, 0x49, 0x49, 0x49, 0x31 },   // S
    { 0x01, 0x01, 0x7F, 0x01, 0x01 },   // T
    { 0x3F, 0x40, 0x40, 0x40, 0x3F },   // U
    { 0x1F, 0x20, 0x40, 0x20, 0x1F },   // V
    { 0x3F, 0x40, 0x38, 0x40, 0x3F },   // W
    { 0x63, 0x14, 0x08, 0x14, 0x63 },   // X
    { 0x07, 0x08, 0x70, 0x08, 0x07 },   // Y
    { 0x61, 0x51, 0x49, 0x45, 0x43 },   // Z

    { 0x00, 0x7F, 0x41, 0x41, 0x00 },   // [
    { 0x55, 0x2A, 0x55, 0x2A, 0x55 },   // '\'
    { 0x00, 0x41, 0x41, 0x7F, 0x00 },   // ]
    { 0x04, 0x02, 0x01, 0x02, 0x04 },   // ^
    { 0x40, 0x40, 0x40, 0x40, 0x40 },   // _
    { 0x00, 0x01, 0x02, 0x04, 0x00 },   // '

    { 0x20, 0x54, 0x54, 0x54, 0x78 },   // a
    { 0x7F, 0x48, 0x44, 0x44, 0x38 },   // b
    { 0x38, 0x44, 0x44, 0x44, 0x20 },   // c
    { 0x38, 0x44, 0x44, 0x48, 0x7F },   // d
    { 0x38, 0x54, 0x54, 0x54, 0x18 },   // e
    { 0x08, 0x7E, 0x09, 0x01, 0x02 },   // f
    { 0x0C, 0x52, 0x52, 0x52, 0x3E },   // g
    { 0x7F, 0x08, 0x04, 0x04, 0x78 },   // h
    { 0x00, 0x44, 0x7D, 0x40, 0x00 },   // i
    { 0x20, 0x40, 0x44, 0x3D, 0x00 },   // j
    { 0x7F, 0x10, 0x28, 0x44, 0x00 },   // k
    { 0x00, 0x41, 0x7F, 0x40, 0x00 },   // l
    { 0x7C, 0x04, 0x18, 0x04, 0x78 },   // m
    { 0x7C, 0x08, 0x04, 0x04, 0x78 },   // n
    { 0x38, 0x44, 0x44, 0x44, 0x38 },   // o
    { 0x7C, 0x14, 0x14, 0x14, 0x08 },   // p
    { 0x08, 0x14, 0x14, 0x18, 0x7C },   // q
    { 0x7C, 0x08, 0x04, 0x04, 0x08 },   // r
    { 0x48, 0x54, 0x54, 0x54, 0x20 },   // s
    { 0x04, 0x3F, 0x44, 0x40, 0x20 },   // t
    { 0x3C, 0x40, 0x40, 0x20, 0x7C },   // u
    { 0x1C, 0x20, 0x40, 0x20, 0x1C },   // v
    { 0x3C, 0x40, 0x30, 0x40, 0x3C },   // w
    { 0x44, 0x28, 0x10, 0x28, 0x44 },   // x
    { 0x0C, 0x50, 0x50, 0x50, 0x3C },   // y
    { 0x44, 0x64, 0x54, 0x4C, 0x44 }    // z
};

static char get_char_xy(char ch, int x, int y)
{
    if (ch < ' ' || ch > 'z')
        ch = ' ';
    ch -= ' ';
    return (ASCII_TAB[ch][x] & (1<<y)) ? BRUSH_CHAR : BLANK_CHAR;
}

void setSinglePix(unsigned char *lpDIBBits, int i, int j, int color) {
	long lLineBytes = (DEFAULT_WIDTH * DEFAULT_BYTE_PER_PIXEL + 3) / DEFAULT_BYTE_PER_PIXEL * DEFAULT_BYTE_PER_PIXEL;
		if (color == 0) { //黄色
			*((unsigned char *) lpDIBBits + lLineBytes * (DEFAULT_HEIGHT -1- j) + i * DEFAULT_BYTE_PER_PIXEL) = 37;
			*((unsigned char *) lpDIBBits + lLineBytes * (DEFAULT_HEIGHT -1- j) + i * DEFAULT_BYTE_PER_PIXEL + 1) = 193;
			*((unsigned char *) lpDIBBits + lLineBytes * (DEFAULT_HEIGHT -1- j) + i * DEFAULT_BYTE_PER_PIXEL + 2) = 255;
			*((unsigned char *) lpDIBBits + lLineBytes * (DEFAULT_HEIGHT -1- j) + i * DEFAULT_BYTE_PER_PIXEL + 3) = 255;
		} else {  //白色透明
			*((unsigned char *) lpDIBBits + lLineBytes * (DEFAULT_HEIGHT -1- j) + i * DEFAULT_BYTE_PER_PIXEL) = 0;
			*((unsigned char *) lpDIBBits + lLineBytes * (DEFAULT_HEIGHT -1- j) + i * DEFAULT_BYTE_PER_PIXEL + 1) = 0;
			*((unsigned char *) lpDIBBits + lLineBytes * (DEFAULT_HEIGHT -1- j) + i * DEFAULT_BYTE_PER_PIXEL + 2) = 0;
			*((unsigned char *) lpDIBBits + lLineBytes * (DEFAULT_HEIGHT -1- j) + i * DEFAULT_BYTE_PER_PIXEL + 3) = 0;
		}
}

static void print_row(char *bmpName, char ch, int row)
{
    int i;
    for (i = 0; i < FONT_COLS; i++) {
        //printf("%c", get_char_xy(ch, i, row));
	xCount++;
	if (get_char_xy(ch, i, row) == BRUSH_CHAR)
            setSinglePix(bmpName,xCount,yCount,0);
    }
}

void clean_one_line(unsigned char *lpDIBBits, int j) {
    int i;
    for (i = 0; i < DEFAULT_WIDTH; i++ )
	setSinglePix(lpDIBBits, i,j, 1);
}


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

void draw(unsigned char *lpDIBBits, char *str) {
int i, j, k, len, index = 0;
len = strlen(str);
while (index < len) {  //设置字符串所对应的像素点
        for (i = 0; i < FONT_ROWS; i++) {
            for (j = 0; j < CHAR_PER_LINE && j + index < len; j++) {
                print_row(lpDIBBits, str[index + j], i);
                for (k = 0; k < HORIZONTAL_DISTANCE; k++) {
                    //printf("%c", BLANK_CHAR);
		    xCount++;
                }
            }
            //printf("\n");
	    xCount = 0;
	    yCount++;
	    yCount %= DEFAULT_HEIGHT;
	    clean_one_line(lpDIBBits, yCount);
        }
        index += CHAR_PER_LINE;
        for (k = 0; k < VERTICAL_DISTANCE; k++) {
           // printf("\n");
	    yCount++;
	    yCount %= DEFAULT_HEIGHT;
	    clean_one_line(lpDIBBits, yCount);
        }
    }
}

int main(int argc, char *argv[])
{
    char writePath[] = "/home/wanghui/Desktop/rs.BMP";//输出的图片路径
    unsigned char pBmpBuf[(DEFAULT_WIDTH * 4 + 3)/4*4 * DEFAULT_HEIGHT];//存储图片像素信息的数组
    //char str[80] = { '\0' };
    
    //printf("Please input a string:\n");
    //scanf("%s", str); //输入要绘制的字符串 
    int i;
    char str[ ]={"qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"};                           
    for (i = 0; i < 25; i++)
        draw(pBmpBuf, str);
    char str1[ ]={"ttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttttt"};                           
    for (i = 0; i < 10; i++)
        draw(pBmpBuf, str1);
    saveBmp(writePath, pBmpBuf, DEFAULT_WIDTH, DEFAULT_HEIGHT, 32);//存储图片
    return 0;
} 
