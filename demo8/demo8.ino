/**
 * 单重计算
 */
#include <ESP8266WiFi.h>       
#include <DNSServer.h>            
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>     

// 4位灯
#include <TM1637TinyDisplay.h>
#include <TM1637_6D.h>
#include <WebSocketClient.h>
#include <ArduinoJson.h>

// 秤
#include "HX711.h"

#define DOUT  D5
#define CLK  D6

#define INIT_ING 0
#define FACTOR_ING 1
#define FACTOR_SUCCESS 2
#define WEIGHT_ING 3
#define WEIGHT_SUCCESS 4
#define CHANGE_ING 5
#define CHANGE_SEND_ING 6
#define STOP_ING 7



//默认误差值
float real_weight = 245;   //误差计算是真实重量
float calibration_factor = 110; //默认误差值


int goods_number = 1000;     //单重计算是物品的数量
float per_weight = 20;     //单重

float before_weight = 0;  //重量变化前的值
int before_number = 0;
float after_weight = 0;   //重量变化后的值
int after_number = 0;
/**
 * 四位灯显示位置信息
 */
const int LOCATION_CLK_PIN = D3;
const int LOCATION_DIO_PIN = D4;
TM1637TinyDisplay display_location(LOCATION_CLK_PIN,LOCATION_DIO_PIN);

/**
 * 秤
 */
const int SCALE_DOUT_PIN = D5;
const int SCALE_CLK_PIN = D6;

HX711 scale;
int scale_status = FACTOR_ING;

/**
 * 六位灯
 */
const int WEIGHT_CLK_PIN = D7;
const int WEIGHT_DIO_PIN = D8;

const int WEIGHT_CHANGE_CLK_PIN = D1;
const int WEIGHT_CHANGE_DIO_PIN = D2;

TM1637_6D display_weight(WEIGHT_CLK_PIN,WEIGHT_DIO_PIN);
TM1637_6D display_weight_change(WEIGHT_CHANGE_CLK_PIN,WEIGHT_CHANGE_DIO_PIN);

/**
 * 动态计算误差值
 */
void factor_handle(){
  Serial.println("factor_handle: ");
  Serial.println(real_weight);
  scale.set_scale(calibration_factor); 
  float weight_result = (float)scale.get_units();
  
  while(real_weight > 0 && weight_result > 100 && scale_status == FACTOR_ING){
      scale.set_scale(calibration_factor); 
      weight_result = (float)scale.get_units();
      
      Serial.print("Reading: ");
      Serial.println(weight_result);
      
      Serial.print("Real: ");
      Serial.println(real_weight);
    
      Serial.println(calibration_factor);
    
      float cha = real_weight - weight_result;
      
      if(cha > 100){
         calibration_factor = calibration_factor - 1;
      }if(cha > 20){
         calibration_factor = calibration_factor - 0.1;
      }else if(cha > 1){
         calibration_factor = calibration_factor - 0.01;
      }else if(cha < -100){
         calibration_factor = calibration_factor + 1;
      }else if(cha < -20){
         calibration_factor = calibration_factor + 0.1;
      }else if(cha < 0){
         calibration_factor = calibration_factor + 0.01;
      }else {
         Serial.println("factor success");
         scale_status = FACTOR_SUCCESS;
      }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("HX711 calibration sketch");

  
  scale.begin(SCALE_DOUT_PIN, SCALE_CLK_PIN);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0

  display_location.setBrightness(0x0f);
  
  display_weight.init();
  display_weight_change.init();

  
  display_weight.set(BRIGHTEST);
  display_weight_change.set(BRIGHTEST);
  
}

void weight_handle(){
  float weight_result = (float)scale.get_units();
  if(goods_number > 0){
     per_weight = weight_result / goods_number;
  }

  Serial.println("weight_handle: ");
  Serial.print("weight_result: ");
  Serial.println(weight_result);
  Serial.print("goods_number: ");
  Serial.println(goods_number);
  Serial.print("per_weight: ");
  Serial.println(per_weight);


  // 重置称重参数准备计算变化信息
  before_weight = 0;
  before_number = 0;
  after_weight = 0;
  after_number = 0;

  display_change();
  scale_status = CHANGE_ING;
}

void display_change(){
   int change_number = after_number - before_number;
   
   Serial.print("display_change: ");
   Serial.println(before_number);
   Serial.println(after_number);
   Serial.println(change_number);
  
   display_weight.displayFloat(before_number);
   
   display_weight_change.displayFloat(change_number);
}

void change_handle(){
  Serial.println("change_handle: ");
  after_weight = (float)scale.get_units();
  after_number = after_weight / per_weight;
  
  Serial.print("after_weight: ");
  Serial.println(after_weight);
  Serial.println(after_number);

  Serial.println(before_weight);
  Serial.println(before_number);
  
  display_change();
  
  if(after_number - before_number > 1){
     //显示变化数量
     delay(3000); //3秒后修改当前数量
     before_weight = after_weight;
     before_number = after_number;
     display_change();

     // 发送变化数据
     //scale_status = CHANGE_SEND_ING
  }
}

void loop() {

  Serial.println("loop: ");

  display_location.showString("A101");
  
  if(scale_status == FACTOR_ING){
     factor_handle();
  }
  if(scale_status == FACTOR_SUCCESS){
     weight_handle();
  }
  if(scale_status == CHANGE_ING){
     change_handle();
  }
  
  delay(1000);
}
