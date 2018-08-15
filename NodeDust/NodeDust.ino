#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "MyQueue.h"

//PIN Defination
#define DUST_PIN A0
#define LED_POWER_PIN D2
#define LED_BUILTIN D4

//Sensor value min
#define DUST_MIN_VAL 37

//queue capacity
#define QUALITY_QUEUE_CAPACITY 100

//WIFI config
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASSWORD = "YOUR_PASSWORD";

//delay time
const int LED_PULSE_TIME = 280;
const int LED_PULSE_TIME2 = 40;
const float OFF_TIME = 9680;

//float val
const double DOUBLE_INF = 1.0 / 0;

//status
bool need_check_dust = true;

//history data queue
MyQueue quality_que(QUALITY_QUEUE_CAPACITY);


ESP8266WebServer server(80);
int getDustSensorVal(void);
double getNowAirQuality();
void handleRoot();
void handleNotFound();
void drawGraph();

//timer
Ticker ticker;
//timer handler
void setDustCheck() {
  need_check_dust = true;
}


void setup() {
  //init
  pinMode(LED_POWER_PIN, OUTPUT);
  digitalWrite(LED_POWER_PIN, HIGH);//power off the led
  pinMode(DUST_PIN, INPUT);
  digitalWrite(DUST_PIN, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  //connect to WIFI
  digitalWrite(LED_BUILTIN, HIGH);//power off the led
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println();
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WIFI_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }
  //set web server handler
  server.on("/", handleRoot);
  server.on("/monitor.svg", drawGraph);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
  //set timer handler
  ticker.attach(3, setDustCheck);
}

void loop() {
  if (need_check_dust)
  {
    digitalWrite(LED_BUILTIN, LOW);
    double quality = getNowAirQuality();
    if (quality > 0)
    {
      quality_que.push(getNowAirQuality());
    }
    need_check_dust = false;
    digitalWrite(LED_BUILTIN, HIGH);
  }
  server.handleClient();
}

int getDustSensorVal(void)
{
  digitalWrite(LED_POWER_PIN, LOW);//power on the led
  delayMicroseconds(LED_PULSE_TIME);
  int dust_val = analogRead(DUST_PIN);
  delayMicroseconds(LED_PULSE_TIME2);
  digitalWrite(LED_POWER_PIN, HIGH);
  delayMicroseconds(OFF_TIME);
  Serial.println(dust_val);
  return dust_val;
}

double getNowAirQuality()
{
  int max_sensor_val = 0;
  int min_sensor_val = 1023;
  int sensor_val_sum = 0;
  //Reduce the error
  for (int i = 0; i < 5; i++)
  {
    int sensor_val = getDustSensorVal();
    if (sensor_val > max_sensor_val)
    {
      max_sensor_val = sensor_val;
    }
    if (sensor_val < min_sensor_val)
    {
      min_sensor_val = sensor_val;
    }
    sensor_val_sum += sensor_val;
  }
  double dust_val  = (sensor_val_sum - max_sensor_val - min_sensor_val) / 3.0;
  double voltage = dust_val * (5.0 / 1024.0);
  //Serial.println(voltage);
  return (double(dust_val) / 1024 - 0.0356) * 120000 * 0.03 + 50;
}

void handleRoot()
{
  digitalWrite(LED_BUILTIN, LOW);
  char temp[1000];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;
  double air_quality = quality_que.rear();
  String title = air_quality < 0 ? "传感器错误" : "现在的空气质量数值为:" + String(air_quality);
  String quality_level;
  if (air_quality < 75)
  {
    quality_level = "非常好";
  } else if (air_quality < 150)
  {
    quality_level = "很好";
  } else if (air_quality < 300)
  {
    quality_level = "好";
  } else if (air_quality < 1050)
  {
    quality_level = "一般";
  } else if (air_quality < 3000)
  {
    quality_level = "差";
  } else
  {
    quality_level = "很差";
  }
  String context = "<!DOCTYPE html>\n<html>\
  <head>\
    <meta charset='utf-8' http-equiv='refresh' content='5'/>\
    <title>空气质量检测</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>" + title + "</h1>\
    <h2>空气质量级别为：" + quality_level + "</h2>\
    <img src='/monitor.svg' />\
    <h3>近期空气质量走势图</h3>\
    <p>Uptime: %02d:%02d:%02d</p>\
  </body>\
</html>";

  snprintf(temp, 1000, context.c_str(),
           hr, min % 60, sec % 60
          );
  server.send(200, "text/html", temp);
  digitalWrite(LED_BUILTIN, HIGH);
}


void handleNotFound() {
  digitalWrite(LED_BUILTIN, LOW);
  String message = "";
  message += "<!DOCTYPE html>\n<html>\
  <head>\
    <meta charset='utf-8'/>\
    <title>诶呀！出错了！</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>好像没有你要访问的页面呀</h1>\
    <p>";
  message += "URI: ";
  message += server.uri();
  message += "<br/>Method: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "<br/>Arguments: ";
  message += server.args();
  message += "<br/>";
  message += "</p></body>\
</html>";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/html", message);
  digitalWrite(LED_BUILTIN, HIGH);
}

void drawGraph() {
  String out = "";
  char temp[100];
  //draw history data
  out += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\" width=\"800\" height=\"500\">\n";
  out += "<rect width=\"800\" height=\"500\" fill=\"rgb(250, 230, 210)\" stroke-width=\"1\" stroke=\"rgb(0, 0, 0)\" />\n";
  out += "<line x1='0' y1='450' x2='800' y2='450' stroke='red'/>";
  out += "<g stroke=\"black\">\n";
  const double *que_ptr = quality_que.getArr();
  int que_front = quality_que.getFront();
  int prev = que_front;
  for (int i = (que_front + 1) % QUALITY_QUEUE_CAPACITY, x = 0; i != que_front; i = (i + 1) % QUALITY_QUEUE_CAPACITY)
  {
    //Serial.println(i);
    sprintf(temp, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke-width=\"1\" />\n", x, 450 - int(que_ptr[prev] / 10 + 0.5), x + 45, 450 - int(que_ptr[i] / 10 + 0.5));
    out += temp;
    prev = i;
    x += 45;
  }
  //Serial.println(que_front);
  out += "</g>\n</svg>\n";

  server.send(200, "image/svg+xml", out);
}
