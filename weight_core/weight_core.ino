/**
 * 货柜货架称重核心
 * 通过ws客户端，与app的ws服务器通信， 7080端口
 * ws消息类型
 * type:ping                 心跳消息，定时更新ip，上报秤的正常情况
 * type:report_device        设备信息上报，上报mac,ip,误差weight，绑定货架id
 * type:report_factor_info   误差信息临时上报，上报称重结果
 * type:report_factor        误差信息上报，上报factor
 * type:report_weight        单重信息上报，
 * type:report_change        重量变化上报，上报mac对应的weight
 * 
 * app通过http服务器 给秤发送消息 6080端口

 * type:set_location     货架位置信息
 * type:set_factor       误差计算中
 * type:set_factor_info  设置误差中 
 * type:set_weight       单重计算中 
 * type:set_change       重量计算中

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

#ifndef WEIGHT
// ws消息类型
#define REPORT_PING "report_ping"  //心跳监测
#define REPORT_DEVICE "report_device"
#define REPORT_FACTOR_INFO "report_factor_info"  //计算的信息
#define REPORT_FACTOR "report_factor"
#define REPORT_WEIGHT "report_weight"
#define REPORT_CHANGE "report_change"

// http消息类型
#define SET_LOCATION "set_location"
#define SET_FACTOR "set_factor"
#define SET_FACTOR_INFO "set_factor_info"
#define SET_WEIGHT "set_weight"
#define SET_CHANGE "set_change"

// 秤的状态
//0 默认状态 1 误差计算中 2 误差计算成功，3 单重测试中 4 单重计算成功 5 等待变化，6 变化发送处理中，7 故障停用 
#define INIT_ING 0
#define FACTOR_ING 1
#define FACTOR_SUCCESS 2
#define WEIGHT_ING 3
#define WEIGHT_SUCCESS 4
#define CHANGE_ING 5
#define CHANGE_SEND_ING 6
#define STOP_ING 7
#endif

const int httpPort = 6080;
const int wsPort = 7080;

const char* ssid = "技术部";
const char* password = "12356789";
char host[] = "192.168.1.179"; //ws服务器

String mac = "";
String ip = "";
String location = ""; //货架位置信息 4位灯显示

float real_weight = 5000;   //误差计算是真实重量
float calibration_factor = 110; //默认误差值

int goods_number = 0;     //单重计算是物品的数量
float per_weight = 0;     //单重

float before_weight = 0;  //重量变化前的值
int before_number = 0;
float after_weight = 0;   //重量变化后的值
int after_number = 0;

WebSocketClient webSocketClient;
WiFiClient client;


/**
 * http服务器
 */
ESP8266WebServer httpServer(httpPort);
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
int scale_status = INIT_ING;

/**
 * 六位灯
 */
const int WEIGHT_CLK_PIN = D7;
const int WEIGHT_DIO_PIN = D8;

const int WEIGHT_CHANGE_CLK_PIN = D9;
const int WEIGHT_CHANGE_DIO_PIN = D10;

//TM1637_6D display_weight(WEIGHT_CLK_PIN,WEIGHT_DIO_PIN);
//TM1637_6D display_weight_change(WEIGHT_CHANGE_CLK_PIN,WEIGHT_CHANGE_DIO_PIN);

void setup_wifi() {
  // 板子通电后要启动，稍微等待一下让板子点亮
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  ip = WiFi.localIP().toString();
  mac = WiFi.macAddress();
  mac.replace(":", "");
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(ip);
  Serial.println("MAC ADDR:" + mac);
}

void handleRoot() {
    String message = "HTTP Data:\n";
    for (uint8_t i = 0; i < httpServer.args(); i++) {
      message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
    }
    Serial.println(message);
    String type = httpServer.arg("type");
    Serial.println(type.length());
    
    if(type.length() > 0 && httpServer.arg("value").length() > 0){
       // 读取位置信息
       if(type == SET_LOCATION){
          location = httpServer.arg("value");
          Serial.print("Location:");
          Serial.println(location);
          display_location.showString(location.c_str());
       }
       // 设置误差信息
       if(type == SET_FACTOR){
          const char* tmp = httpServer.arg("value").c_str();
          real_weight =  atof(tmp);
          Serial.print("weight:");
          Serial.println(real_weight);
          scale_status = FACTOR_ING;
       }
       // 重置误差信息
       if(type == SET_FACTOR_INFO){
           const char* tmp = httpServer.arg("value").c_str();
           calibration_factor =  atof(tmp);
           Serial.print(SET_FACTOR_INFO);
           Serial.println(calibration_factor);
           scale.set_scale(calibration_factor); 
       }
       
       // 单重计算中
       if(type == SET_WEIGHT){
          const char* tmp = httpServer.arg("value").c_str();
          goods_number =  atof(tmp);
          Serial.print("goods_number:");
          Serial.println(goods_number);

          before_weight = 0;
          before_number = 0;
          after_weight = 0;
          after_number = 0;
          
          scale_status = WEIGHT_ING;
       }
       // 重量计算中
       if(type == SET_CHANGE){
          const char* tmp = httpServer.arg("value").c_str();
          before_weight =  atof(tmp);
          Serial.print("before_weight:");
          Serial.println(before_weight);
          scale_status = CHANGE_ING;
       }
    }
    
    httpServer.send(200, "text/plain", message);
}



void websocket_connect(){
  Serial.println("host:ip");
  Serial.println(host);
  Serial.println(wsPort);
    
  if (client.connect(host, wsPort)) {
    Serial.println("client Connected");
  } else {
    Serial.println("client Connection failed.");
  }
  webSocketClient.host = host;
  webSocketClient.path = "/";
  
  Serial.println(host);
   
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {
    Serial.println("Handshake failed.");  
  }  
}

/*
 * ws客户消息处理
  */
void ws_handle(){
  Serial.println("ws_handle: ");
  if (client.connected()) {
     String response; //响应值
     Serial.println("ws_handle:connected ");
     float weight_change = after_weight - before_weight;
     
     Serial.print("weight_change  "); 
     Serial.println(weight_change);
     
     if(scale_status == INIT_ING && weight_change > 100){  // 启动初始化 当重量变化的时候 report_device
          String requestString = "{'type':'"REPORT_DEVICE"','mac':'"+mac+"', 'ip':'"+ip+"','value':'"+scale_status+"'}";
          
          Serial.print("REPORT_DEVICE requestString: "); 
          Serial.println(requestString);
          
          webSocketClient.sendData(requestString);
          webSocketClient.getData(response);
          
          if(response.length() > 0){
              Serial.print("REPORT_DEVICE result: ");  
              Serial.println(response);
          }
      }
      if(scale_status == FACTOR_ING){   //误差计算
          
         
      }
      if(scale_status == FACTOR_SUCCESS){   //误差计算成功
          Serial.println("FACTOR_SUCCESS...: ");  
          //最后的称重结果
          float weight_result = (float)scale.get_units();
          String requestString = "{'type':'"REPORT_FACTOR_INFO"','mac':'"+mac+"', 'ip':'"+ip+"','value':'"+weight_result+"'}";
          Serial.print("FACTOR_ING requestString: "); 
          Serial.println(requestString);
           
          webSocketClient.sendData(requestString);
          webSocketClient.getData(response);
          
          if(response.length() > 0){
              Serial.print("REPORT_FACTOR result: ");  
              Serial.println(response);
          }
          
          //误差值          
          requestString = "{'type':'"REPORT_FACTOR"','mac':'"+mac+"', 'ip':'"+ip+"','value':'"+String(calibration_factor)+"'}";
          Serial.print("FACTOR_SUCCESS requestString: "); 
          Serial.println(requestString);
           
          webSocketClient.sendData(requestString);
          webSocketClient.getData(response);
          
          if(response.length() > 0){
              Serial.print("FACTOR_SUCCESS result: ");  
              Serial.println(response);
          }
      }
      if(scale_status == WEIGHT_ING){   //单重计算中
         Serial.println("WEIGHT_ING...: ");
      } 
      if(scale_status == WEIGHT_SUCCESS){   //单重计算成功
         
          
          String requestString = "{'type':'"REPORT_WEIGHT"','mac':'"+mac+"', 'ip':'"+ip+"','value':'"+String(per_weight)+"'}";
          Serial.print("WEIGHT_SUCCESS requestString: "); 
          Serial.println(requestString);
           
          webSocketClient.sendData(requestString);
          webSocketClient.getData(response);
          
          if(response.length() > 0){
              Serial.print("WEIGHT_SUCCESS result: ");  
              Serial.println(response);
          }
      }
      if(scale_status == CHANGE_ING){   //等待变化
         Serial.println("CHANGE_ING...: ");  
      }
      if(scale_status == CHANGE_SEND_ING){   //重量变化处理中
         Serial.println("CHANGE_SEND_ING...: ");  
      }
      if(scale_status == STOP_ING){   //停用
         Serial.println("STOP_ING...: ");  
      }
      
      //实时心跳 发送状态信息
      String requestString = "{'type':'"REPORT_PING"','mac':'"+mac+"', 'ip':'"+ip+"','value':'"+scale_status+"'}";
          
      Serial.print("REPORT_PING requestString: "); 
      Serial.println(requestString);
      
      webSocketClient.sendData(requestString);
      webSocketClient.getData(response);
      
      if(response.length() > 0){
          Serial.print("REPORT_PING result: ");  
          Serial.println(response);
      }
      
  }else{
    // ws客户端
    websocket_connect();
  }
}

/**
 * 秤的处理
 */
void scale_handle(){
   Serial.println("scale_handle: ");
   scale.set_scale(calibration_factor); 
   float weight_result = (float)scale.get_units();
  
   Serial.print("Reading: ");
   Serial.println(calibration_factor);
   Serial.println(weight_result);
  
   if(scale_status == INIT_ING){    //启动初始化
        after_weight = weight_result; 
   }
   if(scale_status == FACTOR_ING){  //误差较重
        factor_handle();
   }
   if(scale_status == FACTOR_SUCCESS){  //误差较重
        
   }
   if(scale_status == WEIGHT_ING){  //误差较重
        weight_handle();
   }
   if(scale_status == WEIGHT_SUCCESS){  //误差较重
       
   }
   if(scale_status == CHANGE_ING){  //误差较重
        
   }
   if(scale_status == CHANGE_SEND_ING){  //误差较重
        
   }
   if(scale_status == STOP_ING){  //误差较重
        
   }
   
}

/**
 * 动态计算误差值
 */
void factor_handle(){
  Serial.println("factor_handle: ");
  Serial.println(real_weight);
  scale.set_scale(calibration_factor); 
  float weight_result = (float)scale.get_units();

  //重试次数 给ws留出时间发送误差信息
  int nums = 0;  
  int maxNum = 10;  
  
  while(real_weight > 0 && weight_result > 100 && scale_status == FACTOR_ING){
      Serial.println("factor_handle nums: ");
      Serial.println(nums);
      nums = nums + 1;
      if(nums >= maxNum){
          String response; //响应值
          String requestString = "{'type':'"REPORT_FACTOR_INFO"','mac':'"+mac+"', 'ip':'"+ip+"','value':'"+weight_result+"'}";
          Serial.print("FACTOR_ING requestString: "); 
          Serial.println(requestString);
           
          webSocketClient.sendData(requestString);
          webSocketClient.getData(response);
          
          if(response.length() > 0){
              Serial.print("REPORT_FACTOR result: ");  
              Serial.println(response);
          }
          nums = 0;
          continue;
      }
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
void weight_handle(){
  float weight_result = (int)scale.get_units();
  if(goods_number > 0){
     per_weight = weight_result / goods_number;
     Serial.println("weight success");
     scale_status = WEIGHT_SUCCESS;
  }
}
void setup() {
  // 板子通电后要启动，稍微等待一下让板子点亮
  delay(10);
  Serial.begin(115200);

  display_location.setBrightness(0x0f);
  
  // http服务器
  setup_wifi();
  httpServer.on("/", handleRoot);
  httpServer.begin();

  // ws客户端链接
  websocket_connect();

  // 秤的初始化
  scale.begin(SCALE_DOUT_PIN, SCALE_CLK_PIN);
  scale.set_scale(calibration_factor); 
  before_weight = (float)scale.get_units();
  Serial.print("before_weight: ");
  Serial.println(before_weight);

//  display_weight.init();
//  display_weight.set(BRIGHTEST);
//
//  display_weight_change.init();
//  display_weight_change.set(BRIGHTEST);
}

void loop() {
  Serial.println("loop: ");
  
  // 秤处理
  scale_handle();
  // ws处理
  ws_handle();
  // http响应处理
  httpServer.handleClient();

  delay(1000);
}
