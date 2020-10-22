/**
 * 货柜货架称重核心
 * 通过ws客户端，与app的ws服务器通信， 7080端口
 * ws消息类型
 * type:report_ping          心跳消息，定时更新ip，上报秤的正常情况
 * type:report_setup         读取初始化信息，
 * type:report_device        设备信息上报，上报mac,ip,误差weight，绑定货架id
 * type:report_factor_info   误差信息临时上报，上报称重结果
 * type:report_factor        误差信息上报，上报factor
 * type:report_weight        单重信息上报，
 * type:report_change        重量变化上报，上报mac对应的weight
 * type:report_error         错误信息上报，
 * 
 * app通过http服务器 给秤发送消息 6080端口
 
 * type:set_setup        设备初始化信息
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
#define REPORT_SETUP "report_setup"
#define REPORT_DEVICE "report_device"
#define REPORT_FACTOR_INFO "report_factor_info"  //计算的信息
#define REPORT_FACTOR "report_factor"
#define REPORT_WEIGHT "report_weight"
#define REPORT_CHANGE "report_change"

// http消息类型
#define SET_DEVICE "set_device"
#define SET_LOCATION "set_location"
#define SET_FACTOR "set_factor"
#define SET_FACTOR_INFO "set_factor_info"
#define SET_WEIGHT "set_weight"
#define SET_CHANGE "set_change"

// 秤的状态
//-1 读取设备信息 0 默认状态 1 误差计算中 2 误差计算成功，3 单重测试中 4 单重计算成功 5 等待变化，6 变化发送处理中，7 故障停用 
#define WAIT_SETUP -1
#define INIT_ING 0
#define FACTOR_ING 1
#define FACTOR_SUCCESS 2
#define WEIGHT_ING 3
#define WEIGHT_SUCCESS 4
#define CHANGE_ING 5
#define CHANGE_SEND_ING 6
#define STOP_ING 7

// error 错误信息
#define WS_CONNECTED_FAILED 1 //ws服务器链接失败
#define WEIGHT_NO_ZERO 2      //计算单重前未清空

#endif

const int httpPort = 6080;
const int wsPort = 7080;

const char* ssid = "wqvmi";
const char* password = "wq12356789;";
char host[] = "192.168.0.101"; //ws服务器

bool is_send_setup = false;
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
int scale_status = WAIT_SETUP;

/**
 * 六位灯
 */
const int WEIGHT_CLK_PIN = D7;
const int WEIGHT_DIO_PIN = D8;

const int WEIGHT_CHANGE_CLK_PIN = D1;
const int WEIGHT_CHANGE_DIO_PIN = D2;

TM1637_6D display_weight(WEIGHT_CLK_PIN,WEIGHT_DIO_PIN);
TM1637_6D display_weight_change(WEIGHT_CHANGE_CLK_PIN,WEIGHT_CHANGE_DIO_PIN);

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
      
       // 秤初始化
       if(type == SET_DEVICE){
          Serial.println("SET_DEVICE:");
          if(httpServer.arg("status").length()  > 0){
             const char* tmp_loc =  httpServer.arg("loc").c_str();
             const char* tmp_scale =  httpServer.arg("scale").c_str();
             const char* tmp_per =  httpServer.arg("per").c_str();
             const char* tmp_status =  httpServer.arg("status").c_str();
             int set_status = atoi(tmp_status);

             Serial.println("SET_DEVICE set_status:"+set_status);
             
             if(set_status > INIT_ING){
                //设置初始化信息
                location = httpServer.arg("loc");
                calibration_factor = atof(tmp_scale);
                per_weight  = atof(tmp_per);
                Serial.println("SET_DEVICE calibration_factor:"+String(calibration_factor));
                Serial.println("SET_DEVICE per_weight:"+String(per_weight));
            
                scale.set_scale(calibration_factor);
                Serial.println("SET_DEVICE set_scale:"+String(calibration_factor));
                float weight_result = (float)scale.get_units();
                Serial.println("SET_DEVICE weight_result:"+String(weight_result));
                
                //重置重量信息
                before_weight = 0;
                before_number = 0;
                after_weight = 0;
                after_number = 0;
              
                display_change();
             }
             scale_status = set_status;
          } 
       }
       // 读取位置信息
       if(type == SET_LOCATION){
          location = httpServer.arg("value");
          Serial.print("SET_LOCATION:");
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
  if (client.connect(host, wsPort)) {
    Serial.println("client Connected");
  } else {
    Serial.println("client Connection failed.");
  }
  webSocketClient.host = host;
  webSocketClient.path = "/";
  
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
     after_weight = (float)scale.get_units();
     int weight_change = after_weight - before_weight;
     
     Serial.println("weight_change  "); 
     Serial.println(before_weight);
     Serial.println(after_weight);
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

              //重置重量信息
              before_weight = 0;
              before_number = 0;
              after_weight = 0;
              after_number = 0;
          }
      }
      if(scale_status == FACTOR_ING){   //误差计算中
          
         
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
      if(scale_status == WEIGHT_SUCCESS){   //单重计算成功
          Serial.println("WEIGHT_SUCCESS...: ");
      }
      if(scale_status == CHANGE_ING){   //等待变化
         Serial.println("CHANGE_ING...: ");  
         int temp_change = after_number - before_number;
         String requestString = "{'type':'"REPORT_CHANGE"','mac':'"+mac+"', 'ip':'"+ip+"','value':'"+String(temp_change)+"'}";
         Serial.print("CHANGE_SEND_ING requestString: "); 
         Serial.println(requestString);
           
         webSocketClient.sendData(requestString);
         webSocketClient.getData(response);
          
         if(response.length() > 0){
            Serial.print("WEIGHT_SUCCESS result: ");  
            Serial.println(response);
         } 
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
   Serial.println("scale_handle: "+String(scale_status));
   
   if(scale_status == INIT_ING){    //启动初始化
        //after_weight = weight_result; 
   }
   if(scale_status == FACTOR_ING){  //误差较重
        factor_handle();
   }
   if(scale_status == FACTOR_SUCCESS){  //误差较重成功
        
   }
   if(scale_status == WEIGHT_ING){  //单重计算
        weight_handle();
   }
   if(scale_status == WEIGHT_SUCCESS){  //单重计算成功
        change_setup();
   }
   if(scale_status == CHANGE_ING){  //重量变化开始
        change_handle();
   }
   if(scale_status == CHANGE_SEND_ING){  //重量变化发送
        
   }
   if(scale_status == STOP_ING){  //停用
        
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
         scale.set_scale(calibration_factor); 
         scale_status = FACTOR_SUCCESS;
      }
  }
}
void weight_handle(){
  Serial.println("weight_handle");
  int temp_weight = (int)scale.get_units();
  Serial.println(goods_number);
 
  if(goods_number > 0 && temp_weight > 0){
     float weight_result = (float)scale.get_units();
     per_weight = weight_result / goods_number;
     Serial.println("weight_handle:WEIGHT_SUCCESS "+String(per_weight));
  }

  //重置重量信息
  before_weight = 0;
  before_number = 0;
  after_weight = 0;
  after_number = 0;

  display_change();
}

/**
 * 重量变化初始化
 */
void change_setup(){
   Serial.print("change_setup: ");
   after_weight = (float)scale.get_units();
   Serial.println(after_weight);
    
   if(after_weight <= 2){
       before_weight = 0;
       before_number = 0;
       after_weight = 0;
       after_number = 0;
   }
}
void display_change(){
   int change_number = after_number - before_number;
   
   Serial.println("display_change:before_number,after_number,change_number ");
   Serial.println(before_number);
   Serial.println(after_number);
   Serial.println(change_number);
  
   display_weight.displayFloat(before_number);
   display_weight_change.displayFloat(change_number);
}

void change_handle(){
  if(per_weight > 0){
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
         // 发送变化数据
         Serial.println("weight_change: ");
      }
  }
}

/**
 * 初始化读取设备信息
 */
void init_scale(){
  Serial.println("init_scale: ");
  if (client.connected()) {
    if(!is_send_setup){
        Serial.println("init_scale: send_setup");
        is_send_setup = true;
        String response; //响应值
        String requestString = "{'type':'"REPORT_SETUP"','mac':'"+mac+"', 'ip':'"+ip+"','value':'"+scale_status+"'}";
              
        Serial.print("REPORT_SETUP requestString: "); 
        Serial.println(requestString);
        
        webSocketClient.sendData(requestString);
        webSocketClient.getData(response);
        
        if(response.length() > 0){
            is_send_setup = true;
            Serial.print("REPORT_SETUP result: ");  
            Serial.println(response);
        }
    }else{
        Serial.println("init_scale: had_send_setup");
    }
   
  }else{
    Serial.println("init_scale:connect ");
    // ws客户端
    websocket_connect();
  }
}

void setup() {
  // 板子通电后要启动，稍微等待一下让板子点亮
  delay(10);
  Serial.begin(115200);

  
  // http服务器
  setup_wifi();
  httpServer.on("/", handleRoot);
  httpServer.begin();

  // ws客户端链接
  websocket_connect();

  // 秤
  scale.begin(SCALE_DOUT_PIN, SCALE_CLK_PIN);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  
  before_weight = (float)scale.get_units();
  after_weight = before_weight;
  
  // 灯
  display_location.setBrightness(0x0f);
  
  display_weight.init();
  display_weight.set(BRIGHTEST);

  display_weight_change.init();
  display_weight_change.set(BRIGHTEST);

  //六位灯显示
  display_change();
}

void loop() {
  Serial.println("loop: "+String(scale_status));

  if(scale_status < 0){
    //六位灯显示
    display_change();
    // 初始化信息读取
    init_scale();
  }else{
    // 秤处理
    scale_handle();
    // ws处理
    ws_handle();
  }
  
  // http响应处理
  httpServer.handleClient();
  
  delay(1000);
}
