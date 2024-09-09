#include <Arduino.h>
#include <WiFi.h>
#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Time.h>
// #include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 32

// 初始化单总线通讯
// OneWire oneWire(ONE_WIRE_BUS);

// 配置温度传感器
// DallasTemperature sensors(&oneWire);

void weather_update();
int update = 0;
// 定义WiFi密码及SSID

const char *password = "00000000";
const char *ssid = "sun";

// const char *ssid = "TP-LINK_6B14";
// const char *password = "423 family we are";

String url = "https://restapi.amap.com/v3/weather/weatherInfo";
String url2 = "https://api.bilibili.com/x/report/click/now";
String citys = "420115"; // 江夏区420115
String key = "e2cdd38696d47e8da458493bef49a838";

// 配置OLED屏幕的SPI
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/14, /* dc=*/12, /* reset=*/13); // scl：io18  sda：io23

// 定义JSON解析得到的变量
int temperature = 0; // 温度
int humidity = 0;    // 湿度

String windpower;     // 风力
String province;      // 省份
String city;          // 城市
String weather;       // 天气
String winddirection; // 风向
String reporttime;    // 时间

int nowtime; // 北京时间

// 配置RTC时钟
ESP32Time rtc(3600 * 8); // offset in seconds GMT+8

// 按键值

int key_num = 0;
int key_flag = 0;

float PotValue = 0;

void setup()
{
  // 初始化按键
  pinMode(17, INPUT_PULLUP);
  // 初始化传感器
  // sensors.begin();
  // 初始化串口
  Serial.begin(9600);
  // 初始化OLED
  u8g2.begin();
  // 开启中文字符支持
  u8g2.enableUTF8Print();
  // 设置字体
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);

  // 初始化串口屏
  // Serial.begin(9600);
  Serial.println("Hello World!\r\n");

  // 显示连接UI
  u8g2.setCursor(0, 12);
  u8g2.printf("SSID:  %s", ssid);
  u8g2.setCursor(0, 24);
  u8g2.printf("密码:  %s", password);
  u8g2.setCursor(0, 36);
  u8g2.printf("连接中");
  u8g2.sendBuffer();

  /***********WIFI初始化部分***********/
  WiFi.begin(ssid, password);
  pinMode(2, OUTPUT); // 控制LED
  while (WiFi.status() != WL_CONNECTED)
  {
    u8g2.printf(".");
    digitalWrite(2, HIGH);
    delay(200);
    digitalWrite(2, LOW);
    delay(200);
    u8g2.sendBuffer();
  } // 闪烁表示正在连接
  digitalWrite(2, LOW); // 熄灭表示连接成功
  // U8G2显示连接成功
  u8g2.setCursor(0, 48);
  u8g2.printf("连接成功");
  u8g2.sendBuffer();

  // 创建HTTP对象
  HTTPClient http;

  // 访问指定URL
  http.begin(url + "?city=" + citys + "&key=" + key);

  // 接受HTTP相应状态码
  int httpCode = http.GET();

  // OLED显示
  u8g2.setCursor(0, 60);
  u8g2.printf("HTTP状态码: %d", httpCode);
  u8g2.sendBuffer();

  Serial.printf("Status code=%d", httpCode);

  // 获得相应正文
  String response = http.getString();
  // Serial.println(response);

  // 关闭链接
  http.end();

  // 初始化DynamicJsonDocument对象
  DynamicJsonDocument doc(1024);

  // 解析JSON数据
  deserializeJson(doc, response);

  // 转换JSON数据
  temperature = doc["lives"][0]["temperature"].as<int>();
  humidity = doc["lives"][0]["humidity"].as<int>();

  windpower = doc["lives"][0]["windpower"].as<String>();
  winddirection = doc["lives"][0]["winddirection"].as<String>();
  province = doc["lives"][0]["province"].as<String>();
  city = doc["lives"][0]["city"].as<String>();
  weather = doc["lives"][0]["weather"].as<String>();
  reporttime = doc["lives"][0]["reporttime"].as<String>();

  // 获取实时时间
  http.begin(url2);
  int httpCode2 = http.GET();
  String response2 = http.getString();

  // 获得相应正文
  // Serial.println(httpCode2);
  // Serial.println(response2);

  // 关闭链接
  http.end();

  // 初始化DynamicJsonDocument对象
  DynamicJsonDocument doc2(1024);

  // 解析JSON数据
  deserializeJson(doc2, response2);

  // 赋值
  nowtime = doc2["data"]["now"].as<int>();

  //  初始化RTC时钟
  rtc.setTime(nowtime);
  Serial.println("Hello World!\r\n");
}

void loop()
{
  // OLED相关
  u8g2.clearBuffer();

  // 读取按键
  if (digitalRead(17) == 0)
  {
    delay(20);
    if (digitalRead(17) == 0 && key_flag == 0)
    {
      if (key_num == 1)
      {
        key_num = 0;
      }

      else if (key_num == 0)
      {
        key_num = 1;
      }

      key_flag = 1;
    }
  }
  else if (digitalRead(17) != 0)
    key_flag = 0;

  // 模式1，显示时间
  if (key_num == 0)
  { /*===========================================*/
    // 显示时间
    /*===========================================*/
    u8g2.setFont(u8g2_font_logisoso32_tn);
    u8g2.setCursor(0, 32);
    // 小时
    if (rtc.getHour() < 10)
      u8g2.printf("0%d", rtc.getHour());
    else
      u8g2.printf("%d", rtc.getHour());
    // 分隔
    u8g2.printf(":");
    // 分钟
    if (rtc.getMinute() < 10)
      u8g2.printf("0%d", rtc.getMinute());
    else
      u8g2.printf("%d", rtc.getMinute());
    // 秒
    u8g2.setFont(u8g2_font_logisoso16_tn);
    u8g2.setCursor(100, 16);

    if (rtc.getSecond() < 10)
      u8g2.printf("0%d", rtc.getSecond());
    else
      u8g2.printf("%d", rtc.getSecond());
    // AM/PM
    u8g2.setFont(u8g2_font_profont22_tr);
    u8g2.setCursor(100, 32);

    String nowtime = rtc.getAmPm();
    char *c = (char *)nowtime.c_str();
    u8g2.printf("%s", c);

    u8g2.drawHLine(0, 36, 128);
    /*===========================================*/
    // 中文信息显示界面
    /*===========================================*/

    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(0, 48);
    u8g2.printf("天气： %s %d℃", weather, temperature);
    u8g2.setCursor(0, 62);
    u8g2.printf("风力： %s %s级", winddirection, windpower);
    // 显示天气图标
    u8g2.setCursor(110, 48);
    u8g2.setFontMode(0);
    u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
    if (weather == "晴" && rtc.getHour(true) > 6 && rtc.getHour(true) <= 17) // 上午7点到下午5点为白天
      u8g2.drawGlyph(105, 58, 0x45);
    else if (weather == "晴" && rtc.getHour(true) > 17 && rtc.getHour(true) <= 6) // 下午6点到第二天6点为晚上
      u8g2.drawGlyph(105, 58, 0x42);
    else if (weather == "阴")
      u8g2.drawGlyph(105, 58, 0x41);
    else if (weather == "多云")
      u8g2.drawGlyph(105, 58, 0x40);
    else if (weather == "小雨" || weather == "中雨" || weather == "大雨" || weather == "暴雨" || weather == "雨" || weather == "阵雨")
      u8g2.drawGlyph(105, 58, 0x43);
    else
      u8g2.drawGlyph(105, 58, 0x40);
    // 发送缓冲区，刷新显存

    // 电量显示
    // PotValue = analogRead(34) * 0.001705;
    // u8g2.setFont(u8g2_font_5x8_mr);
    // u8g2.setCursor(95, 64);
    // u8g2.printf("%.0f", ((PotValue - 3.2) / 0.9) * 100);
    u8g2.sendBuffer();
    delay(1000);
  }

  // 模式二，显示天气
  else if (key_num == 1)
  {
    /*===========================================*/
    // 模式二
    /*===========================================*/
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(0, 12);
    u8g2.printf("天气： %s %d℃", weather, temperature);
    u8g2.setCursor(0, 24);
    u8g2.printf("风力： %s %s级", winddirection, windpower);
    // 显示天气图标
    u8g2.setCursor(110, 25);
    u8g2.setFontMode(0);
    u8g2.setFont(u8g2_font_open_iconic_weather_2x_t);
    if (weather == "晴" && rtc.getHour(true) > 6 && rtc.getHour(true) <= 17) // 上午7点到下午5点为白天
      u8g2.drawGlyph(105, 25, 0x45);
    else if (weather == "晴" && rtc.getHour(true) > 17 && rtc.getHour(true) <= 6) // 下午6点到第二天6点为晚上
      u8g2.drawGlyph(105, 25, 0x42);
    else if (weather == "阴")
      u8g2.drawGlyph(105, 25, 0x41);
    else if (weather == "多云")
      u8g2.drawGlyph(105, 25, 0x40);
    else if (weather == "小雨" || weather == "中雨" || weather == "大雨" || weather == "暴雨" || weather == "雨" || weather == "阵雨")
      u8g2.drawGlyph(105, 25, 0x43);
    else
      u8g2.drawGlyph(105, 25, 0x40);
    // 获取温度
    // sensors.requestTemperatures();
    // float tempC = sensors.getTempCByIndex(0);
    // 判断温度
    // if (tempC == DEVICE_DISCONNECTED_C)
    //{
    //  tempC = 404;
    //}
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(0, 36);
    float PotValue = analogRead(34) * 0.001705;
    u8g2.printf("电池电量：%.1fV", PotValue);
    u8g2.setCursor(0, 50);
    u8g2.printf("天气更新时间：");
    u8g2.setFont(u8g2_font_6x10_mf);
    u8g2.setCursor(0, 64);
    u8g2.printf("%s", reporttime.c_str());
    u8g2.sendBuffer();
    delay(1000);
  }

  /*===========================================*/
  // 每30分钟更新一次数据
  /*===========================================*/
  if (rtc.getMinute() % 30 == 0 && update == 0)
  {
    weather_update();
    update = 1;
  }
  if (rtc.getMinute() % 30 != 0 && update == 1)
  {
    update = 0;
  }
}

/**
 * @brief 联网以更新天气数据
 *
 */
void weather_update()
{ // OLED显示

  u8g2.clearDisplay();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(0, 12);
  u8g2.printf("更新中...");
  u8g2.sendBuffer();

  // 创建HTTP对象
  HTTPClient http;

  // 访问指定URL
  http.begin(url + "?city=" + "420115" + "&key=" + key);

  // 接受HTTP相应状态码
  int httpCode = http.GET();

  u8g2.setCursor(0, 24);
  u8g2.printf("HTTP:%d", httpCode);
  u8g2.sendBuffer();

  // 获得相应正文
  String response = http.getString();
  // Serial.println(response);

  // 关闭链接
  http.end();

  // 初始化DynamicJsonDocument对象
  DynamicJsonDocument doc(1024);

  // 解析JSON数据
  deserializeJson(doc, response);

  // 转换JSON数据
  temperature = doc["lives"][0]["temperature"].as<int>();
  humidity = doc["lives"][0]["humidity"].as<int>();

  windpower = doc["lives"][0]["windpower"].as<String>();
  winddirection = doc["lives"][0]["winddirection"].as<String>();
  province = doc["lives"][0]["province"].as<String>();
  city = doc["lives"][0]["city"].as<String>();
  weather = doc["lives"][0]["weather"].as<String>();
  reporttime = doc["lives"][0]["reporttime"].as<String>();

  u8g2.setCursor(0, 40);
  u8g2.printf("更新成功！");
  u8g2.setFont(u8g2_font_6x10_mf);
  u8g2.setCursor(0, 64);
  u8g2.printf("%s", reporttime.c_str());
  u8g2.sendBuffer();
  delay(2000);
}

// 这是一个测试，添加了一部分内容