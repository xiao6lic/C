//test 24C0x.c
/*
程序名称：		AT24C0x EEPROM存储器测试程序
编写者：		芯腾电子 http://shop35968526.taobao.com
程序操作：		把J9跳线连接左2
程序效果：		显示上次程序对EEPROM的写入结果（十进制），再写入一个新的值（默认是51）供下次上电显示。
*/

/*------------------------------------------------------------------------------
〖说明〗24Cxx I2C EEPROM字节读写驱动程序，芯片A0-A1-A2要接GND(24C65接VCC,具体看DataSheet)。
现缺页写、页读，和CRC校验程序。以下程序经过50台验证，批量的效果有待考察。
为了安全起见，程序中很多NOP是冗余的，希望读者能进一步精简，但必须经过验证。
Atmel 24C01 比较特殊,为简约型,为其单独编程.
51晶振为11.0592MHz
--------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------
调用方式：void WriteIIC_24CXX(enum EEPROMTYPE eepromtype,unsigned int address,unsigned char ddata) ﹫2001/09/18
函数说明：对于IIC芯片24CXX，在指定地址address写入一个字节ddata

调用方式：unsigned char ReadIIC_24CXX(enum EEPROMTYPE eepromtype,unsigned int address) ﹫2001/09/18

函数说明：读取IIC芯片24CXX，指定地址address的数据。
-----------------------------------------------------------------------------------------------------------------*/
#include "reg51.h"
#include "intrins.h"
sbit SCL= P3^2;					//定义24C02的SCL引脚连接方式
sbit SDA= P3^3;					//定义24C02的SCL引脚连接方式
void delay1(unsigned int x);
void display1(unsigned char d3,unsigned char d4);
#define shuma P0										//定义P0口为数据口
sbit LED_0=P1^4;										//以下定义P1各口为控制口
sbit LED_1=P1^5;
sbit LED_2=P1^6;
sbit LED_3=P1^7;
enum EEPROMTYPE {IIC24C01,IIC24C01A,IIC24C02,IIC24C04,IIC24C08,IIC24C16,IIC24C32,IIC24C64,IIC24C128,IIC24C256};
enum EEPROMTYPE eepromtype;
/*定义段码=====0-9=====A-G=====*/
unsigned char a[16]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,
             0x80,0x90,0x88,0x83,0xc6,0xa1,0x86,0x8e};
			 //共阳极数码管的段码0 1 2 3 4 5 6 7 8 9 A B C D E F

void delay()					//串行读写需要的延时函数
{
	unsigned int i=1200;
	while(i--);
}
/*----------------------------------------------------------------------------
调用方式：write_8bit(unsigned char ch)
函数说明：内函数，私有，用户不直接调用。
-------------------------------------------------------------------------------*/
void write_8bit(unsigned char ch)
{
	unsigned char i=8;
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();
	while (i--)
	{
		SDA=(bit)(ch&0x80);
		_nop_();_nop_();_nop_();_nop_();_nop_();
		ch<<=1;
		SCL=1;
		_nop_();_nop_();_nop_();_nop_();_nop_();
		SCL=0;
		_nop_();_nop_();_nop_();_nop_();_nop_();
	}
	_nop_();_nop_();_nop_();_nop_();_nop_();
	_nop_();_nop_();_nop_();_nop_();_nop_();
}
/*------------------------------------------------------------------------------
调用方式：void ACK(void)
函数说明：内函数，私有，用户不直接调用。
-------------------------------------------------------------------------------*/
void ACK(void)
{
	unsigned char time_1;
	SDA=1;
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();
	SCL=1;
	time_1=5;
	while(SDA) {if (!time_1) break;} 								//ACK
	SCL=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();
}
void WriteIIC_24C01(unsigned char address,unsigned char ddata)
{
	SCL=1;
	_nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 			//Tsu:STA
	SDA=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();_nop_(); 				//Thd:STA
	SCL=0; 															//START
	write_8bit( (address<<1) & 0xfe); 								//写页地址和操作方式,对于24C32－24C256，page不起作用
	ACK();
	write_8bit(ddata); 												//发送数据
	ACK();
	SDA=0;
	_nop_();SCL=1;_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=1; 															//STOP
	delay();
}

/*---------------------------------------------------------------------------------------------------------------
调用方式：void WriteIIC_24CXX(enum EEPROMTYPE eepromtype,unsigned int address,unsigned char ddata)
函数说明：对于IIC芯片24CXX，在指定地址address写入一个字节ddata
-----------------------------------------------------------------------------------------------------------------*/
void WriteIIC_24CXX(enum EEPROMTYPE eepromtype,unsigned int address,unsigned char ddata)
{
	unsigned char page,address_in_page;
	if(eepromtype==IIC24C01) 											//如果是24c01
	{
		WriteIIC_24C01(address,ddata);
		return;
	}
	page=(unsigned char)(address>>8) & 0x07;
	page=page<<1;
	address_in_page=(unsigned char)(address);
	SCL=1;
	_nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 				//Tsu:STA
	SDA=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();_nop_(); 					//Thd:STA
	SCL=0; 																//START
	write_8bit(0xa0 | page); 											//写页地址和操作方式,对于24C32－24C256，page不起作用
	ACK();
	if(eepromtype>IIC24C16) 											//如果是24C01－24C16，地址为一字节;24C32－24C256，地址为二字节
	{
		write_8bit(address>>8);
		ACK();
	}
	write_8bit(address_in_page);
	ACK();
	write_8bit(ddata);
	ACK();
	SDA=0;
	_nop_();SCL=1;_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=1; 																//STOP
	delay();
}
unsigned char ReadIIC_24C01(unsigned char address)
{
	unsigned char ddata=0;
	unsigned char i=8;
	SCL=1;
	_nop_(); _nop_(); _nop_(); _nop_(); _nop_(); _nop_(); 				//Tsu:STA
	SDA=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();_nop_(); 					//Thd:STA
	SCL=0; 																//START
	write_8bit( (address<<1) | 0x01); 									//写页地址和操作方式
	ACK();
	while (i--)
	{
		SDA=1;
		ddata<<=1;
		SCL=0;
		_nop_();_nop_();_nop_();_nop_();_nop_();
		SCL=1;
		if (SDA) ddata|=0x01;
	}
	SCL=0;_nop_();SCL=1;_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=0;_nop_();SCL=1;_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=1; 																//STOP
	delay();
	return ddata;
}
/*----------------------------------------------------------------------------------------------------
调用方式：unsigned char ReadIIC_24CXX(enum EEPROMTYPE eepromtype,unsigned int address)
函数说明：读取IIC芯片24CXX，指定地址address的数据。
------------------------------------------------------------------------------------------------------*/
unsigned char ReadIIC_24CXX(enum EEPROMTYPE eepromtype,unsigned int address)
{
	unsigned char page,address_in_page;
	unsigned char ddata=0;
	unsigned char i=8;
	if(eepromtype==IIC24C01)
	{
		return( ReadIIC_24C01(address) );
	}
	page=(unsigned char)(address>>8) & 0x07;
	page=page<<1;
	address_in_page=(unsigned char)(address);
	SDA=0;_nop_();SCL=0; 												//START
	write_8bit(0xa0 | page); 											//写页地址和操作方式,对于24C32－24C256，page不起作用
	ACK();
	if(eepromtype>IIC24C16) 											//如果是24C32－24C256，地址为二字节，先送高位，后送低位
	{
		write_8bit(address>>8);
		ACK();
	}
	//如果是24C01－24C16，地址为一字节;
	write_8bit(address_in_page);
	ACK();																//以上是一个"哑"写操作，相当于设置当前地址
	SCL=1;
	_nop_();_nop_();_nop_();_nop_();_nop_();_nop_(); 					//Tsu:STA
	SDA=0;
	_nop_();_nop_();_nop_();_nop_();_nop_();_nop_(); 					//Thd:STA
	SCL=0; 																//START
	write_8bit(0xa1); 													//写从地址，置为读模式
	ACK();
	while (i--)
	{
		SDA=1;
		ddata<<=1;
		SCL=0;_nop_();_nop_();_nop_();_nop_();_nop_();SCL=1;
		if (SDA) ddata|=0x01;
	}
	SCL=0;_nop_();SCL=1;_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=0;_nop_();SCL=1;_nop_();_nop_();_nop_();_nop_();_nop_();_nop_();
	SDA=1; 																//STOP
	delay();
	return ddata;
}
/*分析：
该芯片采用传统的IIC口的规约形式，是一个标准的经典IIC封装。
注：使用该程序时注意改变芯片各个接口的修改。注意屏蔽主函数。
*/
main()
{
	unsigned char res;									//读内存单元结果存放
	unsigned char d3,d4;								//数码管3-4位显示使用
	res=ReadIIC_24CXX(IIC24C02,100);					//使用I2C协议读取EEPROM地址为100的数据
	WriteIIC_24CXX(IIC24C02,100,51);
	while(1)
	{
		d3=res/10;										//分离数据方便数码管显示
		d4=res%10;
		display1(d3,d4);								//把读到的值通过U4数码管的后两位显示出来
	}
}

/*-----------------------------以下是数码管显示函数------------------------------*/
void display1(unsigned char d3,unsigned char d4)
{
  	shuma=a[d3];										//选中第三位，发送第三位段码
  	LED_2=0;
  	delay1(100);
  	LED_2=1;

  	shuma=a[d4];										//选中第四位，发送第四位段码
  	LED_3=0;
  	delay1(100);
  	LED_3=1;
}
void delay1(unsigned int x)
{
  	unsigned int i;
														//x为延时长度,可以设置
  	for(i=0;i<x;i++);
}
