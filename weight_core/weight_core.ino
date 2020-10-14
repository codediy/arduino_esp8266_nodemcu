/**
 * 货柜货架称重核心
 * 通过ws客户端，与app的ws服务器通信， 7080端口
 * ws消息类型
 * type:ping            心跳消息，定时更新ip，上报秤的正常情况
 * type:repont_device   设备信息上报，上报mac,ip,误差weight，绑定货架id
 * type:report_factor   误差信息上报，上报factor
 * type:report_weight   单重信息上报，
 * type:report_change   重量变化上报，上报mac对应的weight
 * 
 * app通过http服务器 给秤发送消息 6080端口

 * type:set_location     货架位置信息
 * type:set_factor       误差计算中
 * type:set_weight       单重计算中 
 * type:set_change       重量计算中

 */

#include <ESP8266WiFi.h>       
#include <DNSServer.h>            
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>     

// 4位灯
#include <TM1637TinyDisplay.h>

#include <WebSocketClient.h>
#include <ArduinoJson.h>

// 秤
#include "HX711.h"

const int httpPort = 6080;
const int wsPort = 7080;

const char* ssid = "833-2.4G";
const char* password = "12356789";
char host[] = "192.168.1.179"; //ws服务器

String mac = "";
String ip = "";
String location = ""; //货架位置信息 4位灯显示

float real_weight = 10;   //误差计算是真实重量
float calibration_factor = 460; //默认误差值
int goods_number = 0;     //单重计算是物品的数量
float per_weight = 0;     //单重
float before_weight = 0;  //重量变化前的值
float after_weight = 0;   //重量变化后的值

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
int scale_status = 0; //0 默认状态 1 误差计算中 2 误差计算成功，3 单重测试中 4 正常称重中

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
    String message = "POST form was:\n";
    for (uint8_t i = 0; i < httpServer.args(); i++) {
      message += " " + httpServer.argName(i) + ": " + httpServer.arg(i) + "\n";
    }
    Serial.println(message);
    String type = httpServer.arg("type");
    Serial.println(type.length());
    
    if(type.length()>0){
       // 读取位置信息
       if(type == "set_location" && httpServer.arg("location").length() > 0){
          location = httpServer.arg("location");
          Serial.print("Location:");
          Serial.println(location);
          display_location.showString(location.c_str());
       }
       // 设置误差信息
       if(type == "set_factor" && httpServer.arg("weight").length() > 0){
          const char* tmp = httpServer.arg("weight").c_str();
          real_weight =  atof(tmp);
          Serial.print("weight:");
          Serial.println(real_weight);
          scale_status = 1;
          // 动态计算误差值
          factor_handle();
       }
       // 单重计算中
       if(type == "set_weight" && httpServer.arg("number").length() > 0){
          const char* tmp = httpServer.arg("number").c_str();
          goods_number =  atof(tmp);
          Serial.print("goods_number:");
          Serial.println(goods_number);
          scale_status = 3;
          // 动态计算误差值
          weight_handle();
       }
       // 重量计算中
       if(type == "set_change" && httpServer.arg("before").length() > 0){
          const char* tmp = httpServer.arg("before").c_str();
          before_weight =  atof(tmp);
          Serial.print("before_weight:");
          Serial.println(before_weight);
          scale_status = 4;
          // 动态计算误差值
          change_handle();
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
  if (client.connected()) {
     String response; //响应值
     
     if(location.length() > 0){
        if(scale_status == 1){   //误差计算中
         
        }
        if(scale_status == 2){   //误差计算成功
            String requestString = "{'type':'init_scale','mac':'"+mac+"', 'ip':'"+ip+"','weight':'"+factor+"'}";
            
            webSocketClient.sendData(requestString);
            webSocketClient.getData(response);
            
            if(response.length() > 0){
                Serial.print("Scale result: ");  
                Serial.println(response);
            }
        }
        if(scale_status == 3){   //单重计算中
        
        } 
        if(scale_status == 4){   //正常使用中
         
        }
     }else{
        if(scale_status == 0){  //启动初始化 init
            String requestString = "{'type':'init','mac':'"+mac+"', 'ip':'"+ip+"','weight':'0'}";
            
            webSocketClient.sendData(requestString);
            webSocketClient.getData(response);
            
            if(response.length() > 0){
                Serial.print("Init result: ");  
                Serial.println(response);
            }
        }
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
   if(scale_status == 0){  //启动初始化
   
   }
   if(scale_status == 1){  //初始化中 误差较重
        factor_handle();
   }
   if(scale_status == 2){  //单重计算
   
   }
   if(scale_status == 3){  //正常称重
   
   }
}

/**
 * 动态计算误差值
 */
void factor_handle(){
  scale.set_scale(calibration_factor); //Adjust to this calibration factor
  float weight_result = (float)scale.get_units();
  
  Serial.print("Reading: ");
  Serial.print(weight_result);
  
  if(weight_result == real_weight){
      scale_status = 2; //误差计算成功
  }else if(weight_result > real_weight){
      calibration_factor = calibration_factor + 5;
  }else if(weight_result < real_weight){
      calibration_factor = calibration_factor - 5;
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
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
}

void loop() {
  // http响应处理
  httpServer.handleClient();
  // ws处理
  ws_handle();
  // 秤处理
  scale_handle();
  
  delay(1000);
}
