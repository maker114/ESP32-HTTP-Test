#include <Arduino.h>
#include <WiFi.h>
#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Time.h>
// #include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 32

// 定义子函数
void weather_update(void);
void Button_CallBack(void);

// 定义WiFi密码及SSID
const char *password = "00000000";
const char *ssid = "sun";

String url = "https://restapi.amap.com/v3/weather/weatherInfo";
String url2 = "https://api.bilibili.com/x/report/click/now";
String citys = "420115"; // 江夏区420115
String key = "e2cdd38696d47e8da458493bef49a838";

// 配置OLED屏幕的SPI
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/14, /* dc=*/12, /* reset=*/13); // scl：io18  sda：io23

// 定义JSON解析得到的变量
int temperature = 0;  // 温度
int humidity = 0;     // 湿度
String windpower;     // 风力
String province;      // 省份
String city;          // 城市
String weather;       // 天气
String winddirection; // 风向
String reporttime;    // 时间
int nowtime;          // 北京时间

// 配置RTC时钟
ESP32Time rtc(3600 * 8); // offset in seconds GMT+8

// 初始化变量
int key_num = 0;    // 按键值
float PotValue = 0; // 电压值

// 初始化标志位
int Update_Flag = 0; // 更新标志位，为了避免在同一时间内反复触发
int key_flag = 0;    // 按键标志位，为了避免在按键按下后被视为连按
int mode_flag = 0;   // 模式标志位，为了在多个模式中进行切换

void setup()
{
  // 初始化IO
  pinMode(17, INPUT_PULLUP);                     // 控制按键
  pinMode(2, OUTPUT);                            // 控制LED
  attachInterrupt(17, Button_CallBack, FALLING); // 配置按键中断

  // 初始化OLED
  u8g2.begin();
  u8g2.enableUTF8Print();                 // 开启中文字符支持
  u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置字体

  // 初始化串口
  Serial.begin(9600);
  Serial.println("Hello World!\r\n");

  // 显示连接UI
  u8g2.setCursor(0, 12);
  u8g2.printf("SSID:  %s", ssid);
  u8g2.setCursor(0, 24);
  u8g2.printf("密码:  %s", password);
  u8g2.setCursor(0, 36);
  u8g2.printf("连接中");
  u8g2.sendBuffer();

  // 初始化WIFI
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    u8g2.printf(".");
    digitalWrite(2, HIGH);
    delay(200);
    digitalWrite(2, LOW); // 闪烁表示正在连接
    delay(200);
    u8g2.sendBuffer();
  }
  digitalWrite(2, LOW);  // 熄灭表示连接成功
  u8g2.setCursor(0, 48); // U8G2显示连接成功
  u8g2.printf("网络连接成功");
  u8g2.setCursor(0, 60); // 显示获取状态
  u8g2.printf("联网获取数据中...");
  u8g2.sendBuffer();

  // 初始化HTTP连接，联网以获取JSON文件
  HTTPClient http;                                    // 创建HTTP对象
  http.begin(url + "?city=" + citys + "&key=" + key); // 访问指定URl获取天气信息
  int httpCode1 = http.GET();                         // 接受HTTP相应状态码
  String response = http.getString();                 // 获取天气正文
  http.end();                                         // 结束HTTP连接

  http.begin(url2);                    // 访问指定URl获取时间信息（感谢阿b的API）
  int httpCode2 = http.GET();          // 接受HTTP相应状态码
  String response2 = http.getString(); // 获取时间正文
  http.end();                          // 结束HTTP连接

  u8g2.setCursor(0, 60); // 显示获取状态
  if (httpCode1 == 200 && httpCode2 == 200)
    u8g2.printf("时间与天气获取成功");
  else
    u8g2.printf("HTTP获取失败");
  u8g2.sendBuffer();

  // 解析并转换JSON天气数据
  DynamicJsonDocument doc(1024);  // 初始化DynamicJsonDocument对象
  deserializeJson(doc, response); // 解析JSON数据，获取实时天气
  temperature = doc["lives"][0]["temperature"].as<int>();
  humidity = doc["lives"][0]["humidity"].as<int>();
  windpower = doc["lives"][0]["windpower"].as<String>();
  winddirection = doc["lives"][0]["winddirection"].as<String>();
  province = doc["lives"][0]["province"].as<String>();
  city = doc["lives"][0]["city"].as<String>();
  weather = doc["lives"][0]["weather"].as<String>();
  reporttime = doc["lives"][0]["reporttime"].as<String>();

  // 解析并转换JSON时间数据
  DynamicJsonDocument doc2(1024);          // 初始化DynamicJsonDocument对象
  deserializeJson(doc2, response2);        // 解析JSON数据，获取时间
  nowtime = doc2["data"]["now"].as<int>(); // 赋值

  // 初始化RTC时钟
  rtc.setTime(nowtime);
  delay(2000);
}

void loop()
{
  // OLED帧起始
  u8g2.clearBuffer();
  // 部分一：按键检测，切换模式
  // if (digitalRead(17) == 0)
  //{
  //  delay(20);
  //  if (digitalRead(17) == 0 && key_flag == 0)
  //  {
  //    if (key_num == 1)
  //      key_num = 0;
  //    else if (key_num == 0)
  //      key_num = 1;
  //    key_flag = 1;
  //  }
  //}
  // else if (digitalRead(17) != 0)
  //  key_flag = 0;

  weather_update(); // 天气更新

  // 模式1，显示时间
  if (mode_flag == 0)
  { /*===========================================*/
    // 模式一，正常时间显示界面
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
  }

  // 模式2，显示天气
  else if (mode_flag == 1)
  {
    /*===========================================*/
    // 模式二，详细天气显示界面
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
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(0, 36);
    float PotValue = analogRead(34) * 0.001705;
    u8g2.printf("电池电量：%.1fV", PotValue);
    u8g2.setCursor(0, 50);
    u8g2.printf("天气更新时间：");
    u8g2.setFont(u8g2_font_6x10_mf);
    u8g2.setCursor(0, 64);
    u8g2.printf("%s", reporttime.c_str());
  }

  // 帧结尾，500ms刷新一次
  u8g2.sendBuffer();
  delay(500);
}
/**
 * @brief 联网以更新天气数据
 */
void weather_update(void)
{
  if (rtc.getMinute() % 30 == 0 && Update_Flag == 0)
  {
    // UI显示
    u8g2.clearDisplay();
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);
    u8g2.setCursor(0, 12);
    u8g2.printf("更新中...");
    u8g2.sendBuffer();

    // 创建HTTP对象
    HTTPClient http;
    http.begin(url + "?city=" + "420115" + "&key=" + key); // 访问指定URL
    int httpCode = http.GET();                             // 接受HTTP相应状态码
    String response = http.getString();                    // 获得相应正文
    http.end();                                            // 关闭链接

    // UI显示
    u8g2.setCursor(0, 24);
    u8g2.printf("HTTP:%d", httpCode);
    u8g2.sendBuffer();

    // 转换JSON数据并更新
    DynamicJsonDocument doc(1024);  // 初始化DynamicJsonDocument对象
    deserializeJson(doc, response); // 解析JSON数据
    temperature = doc["lives"][0]["temperature"].as<int>();
    humidity = doc["lives"][0]["humidity"].as<int>();
    windpower = doc["lives"][0]["windpower"].as<String>();
    winddirection = doc["lives"][0]["winddirection"].as<String>();
    province = doc["lives"][0]["province"].as<String>();
    city = doc["lives"][0]["city"].as<String>();
    weather = doc["lives"][0]["weather"].as<String>();
    reporttime = doc["lives"][0]["reporttime"].as<String>();

    // UI显示
    u8g2.setCursor(0, 40);
    u8g2.printf("更新成功！");
    u8g2.setFont(u8g2_font_6x10_mf);
    u8g2.setCursor(0, 64);
    u8g2.printf("%s", reporttime.c_str());
    u8g2.sendBuffer();
    delay(2000);
    u8g2.clearBuffer(); // 清除缓冲区避免残留影响主函数显示
    Update_Flag = 1;
  }
  if (rtc.getMinute() % 30 != 0 && Update_Flag == 1)
    Update_Flag = 0; // 避免反复触发
}

void Button_CallBack(void)
{
  // 古法延时
  for (uint16_t i = 0; i < 65534; i++)
    ;

  if (digitalRead(17) == LOW)
  {
    if (mode_flag == 1)
      mode_flag = 0;
    else if (mode_flag == 0)
      mode_flag = 1;
  }
}