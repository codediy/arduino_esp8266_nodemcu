/*
 * TM1637_6D.cpp
 * A library for the 4 digit display
 *
 * Copyright (c) 2012 seeed technology inc.
 * Website    : www.seeed.cc
 * Author     : Frankie.Chu
 * Create Time: 9 April,2012
 * Change Log :
 *
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "TM1637_6D.h"
#include <Arduino.h>
#include <ctype.h>
static int8_t TubeTab[] = {0x3f,0x06,0x5b,0x4f,
                           0x66,0x6d,0x7d,0x07,
                           0x7f,0x6f,0x77,0x7c,
                           0x39,0x5e,0x79,0x71};//0~9,A,b,C,d,E,F
TM1637_6D::TM1637_6D(uint8_t Clk, uint8_t Data)
{
  Clkpin = Clk;
  Datapin = Data;
  pinMode(Clkpin,OUTPUT);
  pinMode(Datapin,OUTPUT);
}

void TM1637_6D::init(void)
{
  clearDisplay();
}

int TM1637_6D::writeByte(int8_t wr_data)
{
  uint8_t i,count1;
  for(i=0;i<8;i++)        //sent 8bit data
  {
    digitalWrite(Clkpin,LOW);
    if(wr_data & 0x01)digitalWrite(Datapin,HIGH);//LSB first
    else digitalWrite(Datapin,LOW);
    wr_data >>= 1;
    digitalWrite(Clkpin,HIGH);

  }
  digitalWrite(Clkpin,LOW); //wait for the ACK
  digitalWrite(Datapin,HIGH);
  digitalWrite(Clkpin,HIGH);
  pinMode(Datapin,INPUT);

  bitDelay();
  uint8_t ack = digitalRead(Datapin);
  if (ack == 0) 
  {
     pinMode(Datapin,OUTPUT);
     digitalWrite(Datapin,LOW);
  }
  bitDelay();
  pinMode(Datapin,OUTPUT);
  bitDelay();
  
  return ack;
}
//send start signal to TM1637_6D
void TM1637_6D::start(void)
{
  digitalWrite(Clkpin,HIGH);//send start signal to TM1637_6D
  digitalWrite(Datapin,HIGH);
  digitalWrite(Datapin,LOW);
  digitalWrite(Clkpin,LOW);
}
//End of transmission
void TM1637_6D::stop(void)
{
  digitalWrite(Clkpin,LOW);
  digitalWrite(Datapin,LOW);
  digitalWrite(Clkpin,HIGH);
  digitalWrite(Datapin,HIGH);
}
//display function.Write to full-screen.
void TM1637_6D::displaylong(int8_t DispData[])
{
  int8_t SegData[6];
  uint8_t i;
  for(i = 0;i < 6;i ++)
  {
    SegData[i] = DispData[i];
     Serial.println( SegData[i]);
  }
  coding(SegData);
  start();          //start signal sent to TM1637_6D from MCU
  writeByte(ADDR_AUTO);//
  stop();           //
  start();          //
  writeByte(Cmd_SetAddr);//
  for(i=0;i < 6;i ++)
  {
    writeByte(SegData[i]);        //
  }
  stop();           //
  start();          //
  writeByte(Cmd_DispCtrl);//
  stop();           //
}

void TM1637_6D::displayFloat(float floatdisplay)
{  int8_t tempListDisp[6] = {10,10,10,10,10,10}; // fill array with value for blank digit
  int8_t tempListDispPoint[6] = {0x00,0x00,0x00,0x00,0x00,0x00}; // don't show any points
  int8_t i = 0;
  // String for converting millis value to seperate characters in the string
  String floatstring;  
  
  // if the integer is bigger than 6 characters, display an error(dashes)
  if(floatdisplay >= 999999.5 || floatdisplay <= -99999.5)
  {
 //display(0);
  }
  else
  {  
    // Calculate how many digits we have before the decimal point
    if(floatdisplay < 0) { floatstring =  String(floatdisplay*-1.0, 1);} // with one "dummy" decimal
    else { floatstring =  String(floatdisplay, 1);} // with one "dummy" decimal
  for(i = 0; i < floatstring.length(); i++) 
    {
      // check how many digits we have before the decimal point
      if(floatstring[i] == '.') break;
    }
    uint8_t numberofdigits = i;
    uint8_t decimal_point_add = 0; // used to skip the decimal point in the string
  
  if(floatdisplay < 0)
  {
      floatstring =  String((floatdisplay*-1.0), 5-numberofdigits); // convertering the inverted integer to a string
    decimal_point_add = 0;
      for(i = 0; i < floatstring.length(); i++) 
      {
        // number "0" in ASCII is 48 in decimal. If we substract the character value
        // with 48, we will get the actual number it is representing as a character
      tempListDisp[i] = floatstring[floatstring.length()-i-1-decimal_point_add] - 48;
        if(floatstring[floatstring.length()-i-2] == '.')
        {
          tempListDispPoint[i+1] =1; // set decimal point
      decimal_point_add = 1;
        }
        
      }
      tempListDisp[5] = 11;// adding the dash/minus later for this negative integer
  }
  else
  {
      floatstring =  String(floatdisplay, 6-numberofdigits); // convertering integer to a string
      decimal_point_add = 0;
      Serial.println(floatstring.length());
      for(i = 0; i < (floatstring.length() - decimal_point_add); i++) 
      {
        
        // number "0" in ASCII is 48 in decimal. If we substract the character value
        // with 48, we will get the actual number it is representing as a character
        tempListDisp[i] = floatstring[floatstring.length()-i-1-decimal_point_add] - 48;
        if(floatstring[floatstring.length()-i-2] == '.')
        {
          tempListDispPoint[i+1] = 1; // set decimal point
          decimal_point_add = 1;
        }
      }
  }
  if(decimal_point_add == 0) tempListDispPoint[0] = 1;

  for(int n = 0;n<6;n++){
      display(n,tempListDisp[5-n],tempListDispPoint[5-n]);
      Serial.println(tempListDispPoint[5-n]);
  }

  }
}

void TM1637_6D::displayIngRight(int numberDisplay)
{ 
  /* 显示内容 tempListDisp数字显示,tempListDispPoint小数点显示 */
  int8_t tempListDisp[6] = {10,10,10,10,10,10}; // fill array with value for blank digit
  int8_t tempListDispPoint[6] = {0x00,0x00,0x00,0x00,0x00,0x00}; // don't show any points
  int8_t i = 0; 
  
  String numberStirng; 
  if(numberDisplay > 0 && numberDisplay <= 999999)
  {
      numberStirng =  String(numberDisplay); // convertering integer to a string
      //从第一个位置开始取数字
      /* 
        d  5 4 3 2 1 0  显示的位置
        s  0 0 0 0 0 1  数字长度
           0 0 0 0 0 0  
       */    
      int pad_length = 6 - numberStirng.length();
      Serial.print("numberStirng:");
      Serial.println(numberStirng);
      Serial.println(pad_length);

      for(i = 0; i < 6; i++) 
      {
         Serial.println("tempListDisp");
         Serial.println(i);

         if(i < pad_length){
            tempListDisp[5 - i] = 0;
         } else{
            Serial.println(numberStirng[i - pad_length]);
            tempListDisp[5 - i] = numberStirng[i - pad_length] - 48;
         }
         
         Serial.println(tempListDisp[i]);
      }
  }

  for(int n = 0;n<6;n++){
      display(n,tempListDisp[5-n],tempListDispPoint[5-n]);
      Serial.print("number digit");
      Serial.print(n);
      Serial.print(",");
      Serial.println(tempListDispPoint[5-n]);
  }

}

//******************************************
void TM1637_6D::display(uint8_t BitAddr,int8_t DispData,int8_t PointFlag)
{
  int8_t SegData;
  SegData = coding(DispData,PointFlag);
  start();          //start signal sent to TM1637_6D from MCU
  writeByte(ADDR_FIXED);//
  stop();           //
  start();          //
  writeByte(BitAddr|0xc0);//
  writeByte(SegData);//
  stop();            //
  start();          //
  writeByte(Cmd_DispCtrl);//
  stop();           //
}

void TM1637_6D::clearDisplay(void)
{
  display(0x00,0x7f,0);
  display(0x01,0x7f,0);
  display(0x02,0x7f,0);
  display(0x03,0x7f,0);
  display(0x04,0x7f,0);
  display(0x05,0x7f,0);
}
//To take effect the next time it displays.
void TM1637_6D::set(uint8_t brightness,uint8_t SetData,uint8_t SetAddr)
{
  Cmd_SetData = SetData;
  Cmd_SetAddr = SetAddr;
  Cmd_DispCtrl = 0x88 + brightness;//Set the brightness and it takes effect the next time it displays.
}

//Whether to light the clock point ":".
//To take effect the next time it displays.
void TM1637_6D::point(boolean PointFlag)
{
  _PointFlag = PointFlag;
}
void TM1637_6D::coding(int8_t DispData[])
{
  uint8_t PointData;
  if(_PointFlag == POINT_ON)PointData = 0x80;
  else PointData = 0;
  for(uint8_t i = 0;i < 6;i ++)
  {
    if(DispData[i] == 0x7f)DispData[i] = 0x00;
    else DispData[i] = TubeTab[DispData[i]] + PointData;
  }
}
int8_t TM1637_6D::coding(int8_t DispData,int8_t PointFlag)
{
  uint8_t PointData;
  if(PointFlag == POINT_ON)PointData = 0x80;
  else PointData = 0;
  if(DispData == 0x7f) DispData = 0x00 + PointData;//The bit digital tube off
  else DispData = TubeTab[DispData] + PointData;
  return DispData;
}
void TM1637_6D::bitDelay(void)
{
	delayMicroseconds(50);
}
