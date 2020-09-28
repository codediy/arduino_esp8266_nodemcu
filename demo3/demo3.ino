#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal

ESP8266WebServer httpServer(8080);

const char* ssid = "测试路由器";
const char* password = "12356789";

String Mac_Addr = "";
int weight = 0;

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

void handleRoot() {
  String message = String(weight);
  httpServer.send(200, "text/html", message);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  setup_wifi();
  httpServer.on("/", handleRoot);
  httpServer.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  weight = weight + 1;
  httpServer.handleClient();
  delay(100);
}
