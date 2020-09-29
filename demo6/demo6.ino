#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266WiFi.h>

#include <TM1637_6D.h>
#include <WebSocketClient.h>
#include <ArduinoJson.h>

//clientDebug
#define DEBUGGING

const char* ssid = "833-2.4G";
const char* password = "12356789";

char path[] = "/";
char host[] = "192.168.1.121";

WebSocketClient webSocketClient;
WiFiClient client;
String Mac_Addr = "";

String deviceid = "";
int weight = 0;

TM1637_6D display_b(D0,D1); // 6 digit led

void setup_wifi() {
  delay(10);
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

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Mac_Addr = WiFi.macAddress();
  Mac_Addr.replace(":", "");
  Serial.println("MAC ADDR:" + Mac_Addr);
}

void websocket_connect(){
  if (client.connect(host, 8080)) {
    Serial.println("client Connected");
  } else {
    Serial.println("client Connection failed.");
  }

  webSocketClient.path = path;
  webSocketClient.host = host;
  if (webSocketClient.handshake(client)) {
    Serial.println("Handshake successful");
  } else {
    Serial.println("Handshake failed.");  
  }  
}

void setup() {
   
   Serial.begin(115200);
   display_b.init();
   display_b.set(BRIGHTEST);
   deviceid = "1234";
   setup_wifi();
   websocket_connect();
}

void loop() {
  
   String data;
  
   if (client.connected()) {
        webSocketClient.sendData("{'deviceid':'"+deviceid+"', 'weight':"+String(weight)+"}");
        webSocketClient.getData(data);
        Serial.print("Received data: ");
        Serial.println(data);
        if(data.length() > 0){
            display_b.displayFloat(weight);
        }
        weight = weight + 1;
   }else{
       Serial.print("client close ");
       websocket_connect();
   }

   delay(1000);
}
