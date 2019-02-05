#include <LiquidCrystal.h>
LiquidCrystal lcd(13, 12, 14, 4, 5, 16);

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

DynamicJsonDocument jsonBuffer(200);

int latchPin = 15;//74HC595
int clockPin = 2;
int dataPin = 0; //这里定义了那三个脚

unsigned char table[12] = {0xc0, 0xf9, 0xa4, 0xb0, 0x99, 0x92, 0x82, 0xf8, 0x80, 0x90,
                           0xff, 0x7f,
                          };//0 1 2 3 4 5 6 7 8 9  灭  点//  共阳数码管

#include <DYWiFiConfig.h>
DYWiFiConfig wificonfig;
ESP8266WebServer webserver(80);
#define DEF_WIFI_SSID ""
#define DEF_WIWI_PASSWORD ""
#define AP_NAME "DYWiFi099"	//dev

void wificb(int c)
{
  Serial.print("=-=-=-=-");
  Serial.println(c);
}

void disp(long num)//数码管显示函数
{
  char str[8] = {0};

  for (int i = 0; i < 8; i++)
  {
    str[i] = num % 10;
    num /= 10;
  }

  if (str[7] == 0)//去零
  {
    str[7] = 10;
  }
  for (int i = 7; i > 0; i--)//偷懒
  {
    if (str[i] == 10 && str[i - 1] == 0)
    {
      str[i - 1] = 10;
    }
  }

  digitalWrite(latchPin, LOW); //将ST_CP口上面加低电平让芯片准备好接收数据

  for (int i = 0; i < 8; i++)
  {
    shiftOut(dataPin, clockPin, MSBFIRST, table[str[i]]);
    //串行数据输入引脚为dataPin，时钟引脚为clockPin，执行MSB有限发送，发送数据table[i]
  }

  digitalWrite(latchPin, HIGH); //将ST_CP这个针脚恢复到高电平
}

void Init()
{
  digitalWrite(latchPin, LOW); //将ST_CP口上面加低电平让芯片准备好接收数据

  for (int i = 0; i < 8; i++)//192.168.4.1
  {
    shiftOut(dataPin, clockPin, MSBFIRST, table[1]);
    shiftOut(dataPin, clockPin, MSBFIRST, table[4]&table[11]);

    shiftOut(dataPin, clockPin, MSBFIRST, table[8]&table[11]);
    shiftOut(dataPin, clockPin, MSBFIRST, table[6]);
    shiftOut(dataPin, clockPin, MSBFIRST, table[1]);

    shiftOut(dataPin, clockPin, MSBFIRST, table[2]&table[11]);
    shiftOut(dataPin, clockPin, MSBFIRST, table[9]);
    shiftOut(dataPin, clockPin, MSBFIRST, table[1]);

    //串行数据输入引脚为dataPin，时钟引脚为clockPin，执行MSB有限发送，发送数据table[i]
  }

  digitalWrite(latchPin, HIGH); //将ST_CP这个针脚恢复到高电平
}

void setup()
{
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT); //让三个脚都是输出状态
  Init();//显示192.168.4.1

  lcd.begin(16, 2);
  lcd.print("check DYWiFi099");
  lcd.setCursor(0, 1);
  lcd.print("goto 192.168.4.1");

  Serial.begin(115200);
  Serial.println("Startup");

  wificonfig.begin(&webserver, "/");

  DYWIFICONFIG_STRUCT defaultConfig =  wificonfig.createConfig();

  strcpy(defaultConfig.SSID, DEF_WIFI_SSID);
  strcpy(defaultConfig.SSID_PASSWORD, DEF_WIWI_PASSWORD);
  strcpy(defaultConfig.HOSTNAME, AP_NAME);
  strcpy(defaultConfig.APNAME, AP_NAME);

  wificonfig.setDefaultConfig(defaultConfig);

  wificonfig.enableAP();
}

long uid;//up主编号
long fans;//粉丝数
bool success = 0; //连接成功标志位

void loop()
{
  wificonfig.handle();

  HTTPClient http;

  http.begin("http://api.bilibili.com/x/relation/stat?vmid=22179720");//自己
  Serial.println("http://api.bilibili.com/x/relation/stat?vmid=22179720");

  int httpCode = http.GET(); //获取数据
  Serial.print("httpCode = ");
  Serial.println(httpCode);

  if (httpCode == 200)//说明已经正常工作
  {
    if (success == 0)
    {
      lcd.clear();
      lcd.print("WiFi connected!");
      lcd.setCursor(0, 1);
      lcd.print("checking");

      for (int i = 0; i < 8; i++)
      {
        delay(500);
        lcd.print(".");
      }

      Serial.println();
      Serial.println("WiFi connected");
      delay(1000);
      lcd.clear();

      success = 1;
    }

    Serial.println("httpCode == 200, http connected!!");
    String resBuff = http.getString();
    DeserializationError error = deserializeJson(jsonBuffer, resBuff);

    JsonObject root = jsonBuffer.as<JsonObject>();//转化数据

    uid = root["data"]["mid"];
    fans = root["data"]["follower"];//定义fans并赋值

    lcd.setCursor(0, 0);
    lcd.print("Uid: ");
    lcd.print(uid);
    lcd.setCursor(0, 1); //LCD屏幕坐标
    lcd.print("Fans: ");
    lcd.print(fans);//打印变量
    disp(fans);//数码管显示

    Serial.print("uid = ");
    Serial.println(uid);
    Serial.print("fans = ");
    Serial.println(fans);

    Serial.println();
  }

  delay(1000);//1s更新一次
}
