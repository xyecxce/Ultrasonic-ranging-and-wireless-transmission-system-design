#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

//设置超声波传感器数据传输引脚
const int trigPin = 4;  //D2
const int echoPin = 5;  //D1
const int alarmpin = 13;//D7
const int ledpin = 16; //D0

const int alarmlen = 50; //警报阈值 
const int updatetime = 33;//ms单位

//定义eoch引脚高电平持续时间变量、毫米百倍长度变量、毫米整数部分长度变量、毫米小数部分长度变量

unsigned long duration = 0;
unsigned long Len_mm_X100 = 0;
unsigned long Len_Integer = 0; 
unsigned int Len_Fraction = 0;

//定义将要连接并显示在OLED上的字符变量
const char str1[] = "distance:";
char intstr[10]="";
const char a[]= ".";
char frastr[5]=""; 
const char b[]="mm";

//定义将要连接成JSON格式的框架内容
char A[30] = "{\"distance\": \"";
char B[5] = "\"}";

//定义重连时间间隔
const int reconinterval = 1000;
// 设置WiFi参数
const char* ssid = "Merc";
const char* password = "lhz2717939";

// 设置MQTT服务器参数

//const char* username1 = "e1llo3rmw50zr85d";
//const char* username2 = "88eu32lbpitpkbzk"  

const char* mqtt_server = "bj-3-mqtt.iot-api.com";
const int mqtt_port = 1883;
const char* mqtt_username = "x0ftno8zx5mjmu73";
const char* mqtt_password = "Vx6RxfBovK";

// 设置MQTT主题
const char* mqtt_topic = "attributes";

//#
              //创建一系列对象

//定义一个display设备，并且设置它的SDA为D3引脚，SCL为D5引脚
SSD1306Wire display(0x3c, D3, D5);

//创建一个ui,并且把display传递给ui
OLEDDisplayUi ui(&display);

WiFiClient espClient;
PubSubClient client(espClient);

//#

//一系列函数声明

//绘制Frame,这个函数最后由ui来调用，参数格式是给定的
void DrawFrame(OLEDDisplay *display,OLEDDisplayUiState* state, int16_t x, int16_t y)
{
  //设置绘制文字的方式为左对齐
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  //设置字体为ArialMT_Plain_16,并在屏幕(0,10)的位置绘制Hello World!
  display->setFont(ArialMT_Plain_24);
  
  display->drawString(8 + x, 8 + y,str1);
  display->drawString(8 + x, 32 + y,intstr);
}

//创建FrameCallBack给ui使用，这里只有一个Frame,Frame的作用可以让ui来切换不同的frame绘制不同的画面
FrameCallback frames[] = {DrawFrame};
//只有一个Frame,所以frame count是1
int frameCount = 1;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

//定义MQTT连接反馈,方便查看MQTT连接状态
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)intstr[i]);//(char)payload[i]
  }
  Serial.println();
}
                  //boolean connect (clientID, [username, password], [willTopic, willQoS, willRetain, willMessage], [cleanSession])
void reconnect() {
  while (!client.connected()) {
   // Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_username, mqtt_password,mqtt_topic, 0, false, intstr)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      delay(reconinterval);
    }
  }
}

void setup() {
  //#超声波测距及OLED显示部分
                    
  pinMode(trigPin, OUTPUT); 
  pinMode(echoPin, INPUT); 
  pinMode(alarmpin,OUTPUT);
  pinMode(ledpin,OUTPUT);
  //设置刷新率为1秒30帧
  ui.setTargetFPS(30);
  //给ui传递frame信息
  ui.setFrames(frames,frameCount);
  //关闭frame标签
  ui.disableAllIndicators();
  //关闭自动切换frame功能
  ui.disableAutoTransition();
  //初始化ui
  ui.init();
  //翻转屏幕180度
  display.flipScreenVertically();
  //设置ui绘制当前frame为frames的第0个frame
  ui.switchToFrame(0);
  //#

  Serial.begin(115200);

  //#WIFI及MQTT协议连接的初始化
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  //#
}

void loop() {
  
  // 初始化trigPin引脚
  digitalWrite(trigPin, LOW);
  digitalWrite(alarmpin,LOW);
  digitalWrite(ledpin,HIGH);
  delayMicroseconds(2);

  // 使trigPin引脚保持10微妙的高电平
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // 读取echopin,返回声波传播时间,单位为微秒
  duration = pulseIn(echoPin, HIGH);
  
  //整数和小数部分数值计算
  if((duration < 60000) && (duration > 1))
  {

    Len_mm_X100 = (duration*34)/2; //如果反过来在右边做除法，由于arduino浮点数据类型运算值容易出错，会致使结果误差很大
    Len_Integer = Len_mm_X100/100 - 10;
    if(Len_Integer<=alarmlen)
    {
      digitalWrite(alarmpin,HIGH); 
      digitalWrite(ledpin,LOW);
    }

    Len_Fraction = Len_mm_X100%10;
    
  }

  //整合OLED所要显示的内容a,b变量不改变，intstr及frastr在每次循环中改变
  sprintf(intstr,"%d",Len_Integer);
  sprintf(frastr,"%d",Len_Fraction);
  strcat(intstr,a);
  strcat(intstr,frastr);
  strcat(intstr,b);
    
   
  //帧速率由ui控制，更新完ui后会返回下一帧需要等待的时间
  int remainingTimeBudget = ui.update();
   
    
  //延迟对应的时间后可以再次更新屏幕
  if(remainingTimeBudget >updatetime)
  {
    delay(remainingTimeBudget); 
  }
  else
  {
    delay(updatetime);
  }
    
   
  //保持mqtt的连接
  if (!client.connected()) 
  {
    reconnect();
  }
  client.loop();
    
  //将要发送的内容整合到变量A
  strcat(A,intstr);
  strcat(A,B);
  Serial.println(A);
    
  // 发布数据到MQTT主题
  client.publish(mqtt_topic,A);

  //初始化A,B字符串的内容
  strcpy(A,"{\"distance\": \"");
  //strcpy(B,"\"}");

  //发布延时
  delay(11000); // 每隔11秒发布一次数据
}