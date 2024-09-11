#include <Arduino.h>
#include <WiFi.h>
#include <U8g2lib.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32Time.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 32

// 定义子函数
void weather_update(void);
void Display_Mode1(void);
void Display_Mode2(void);
void Display_Mode3(void);
void Display_Mode4(void);
void Button_Scan(void);
void HTTP_LinkError_Handle(void);
void Move_Cursor(int GoalValue, float *CurrentValue);
void NUM_Display(int num, int x, int y, float change[], int W, int H);

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
int key_num = 0;            // 按键值
float PotValue = 0;         // 电压值
uint16_t Time_count = 0;    // 时间计数
String response, response2; // HTTP响应

// 初始化标志位
int Update_Flag = 0; // 更新标志位，为了避免在同一时间内反复触发
int Button_Flag = 0; // 按键标志位，为了避免在按键按下后被视为连按
int mode_flag = 0;   // 模式标志位，为了在多个模式中进行切换
void setup()
{
  // 初始化IO
  pinMode(17, INPUT_PULLUP); // 控制按键
  pinMode(2, OUTPUT);        // 控制LED

  // 初始化OLED
  u8g2.begin();
  u8g2.enableUTF8Print();                 // 开启中文字符支持
  u8g2.setFont(u8g2_font_wqy12_t_gb2312); // 设置字体

  // 初始化串口
  // Serial.begin(9600);
  // Serial.println("Hello World!\r\n");

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
  response = http.getString();                        // 获取天气正文
  http.end();                                         // 结束HTTP连接

  http.begin(url2);             // 访问指定URl获取时间信息（感谢阿b的API）
  int httpCode2 = http.GET();   // 接受HTTP相应状态码
  response2 = http.getString(); // 获取时间正文
  http.end();                   // 结束HTTP连接

  u8g2.setCursor(0, 60); // 显示获取状态
  if (httpCode1 == 200 && httpCode2 == 200)
    u8g2.printf("时间与天气获取成功");
  else
  {
    u8g2.printf("时间与天气获取失败");
    HTTP_LinkError_Handle(); // 联网失败处理函数
  }
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

  weather_update(); // 天气更新
  Button_Scan();    // 按键扫描

  if (Time_count % 100 == 0) // 1s刷新一次
  {
    u8g2.clearBuffer();
    switch (mode_flag)
    {
    case 0:
      Display_Mode1();
      break;
    case 1:
      Display_Mode2();
      break;
    case 3:
      Display_Mode4();
      break;
    default:
      break;
    }
    Time_count = 0; // 作为循环中时间最长的进程，执行完之后就清除计数值
  }
  if (rtc.getSecond() % 20 == 0 && mode_flag == 2) // 60s刷新一次
  {
    u8g2.clearBuffer();
    Display_Mode3();
  }
  // 计数装置
  Time_count++;
  delay(10);
}

void Display_Mode1(void)
{
  /*===========================================*/
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
  u8g2.sendBuffer();
}

void Display_Mode2(void)
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
  u8g2.sendBuffer();
}

float change_H1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
float change_H2[8] = {0, 0, 0, 0, 0, 0, 0, 0};

float change_M1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
float change_M2[8] = {0, 0, 0, 0, 0, 0, 0, 0};

float change_S1[8] = {0, 0, 0, 0, 0, 0, 0, 0};
float change_S2[8] = {0, 0, 0, 0, 0, 0, 0, 0};

int num1 = 0, num2 = 0;
void Display_Mode3(void)
{
  /*===========================================*/
  // 模式三，简单时钟界面
  /*===========================================*/
  do
  {
    u8g2.clearBuffer(); // 清空缓冲区
    NUM_Display(rtc.getHour() / 10, 25, 20, change_H1, 12, 15);
    NUM_Display(rtc.getHour() % 10, 25 + 15, 20, change_H2, 12, 15);

    u8g2.drawVLine(60, 25, 5);
    u8g2.drawVLine(60, 40, 5);

    NUM_Display(rtc.getMinute() / 10, 65, 20, change_M1, 12, 15);
    NUM_Display(rtc.getMinute() % 10, 65 + 15, 20, change_M2, 12, 15);

    // NUM_Display(rtc.getSecond() / 10, 90, 20, change_S1, 5, 8);
    // NUM_Display(rtc.getSecond() % 10, 90 + 8, 20, change_S2, 5, 8);

    u8g2.sendBuffer(); // 发送缓冲区数据
  } while (change_H1[7] < 15 && change_H2[7] < 15 && change_M1[7] < 15 && change_M2[7] < 15);
  change_H1[7] = 0;
  change_H2[7] = 0;
  change_M1[7] = 0;
  change_M2[7] = 0;
  change_S1[7] = 0;
  change_S2[7] = 0;
}

void Display_Mode4(void)
{
  /*===========================================*/
  // 模式四，带有秒针简单时钟界面
  /*===========================================*/
  do
  {
    u8g2.clearBuffer(); // 清空缓冲区
    NUM_Display(rtc.getHour() / 10, 14, 20, change_H1, 12, 15);
    NUM_Display(rtc.getHour() % 10, 14 + 15, 20, change_H2, 12, 15);

    u8g2.drawVLine(46, 25, 5);
    u8g2.drawVLine(46, 40, 5);

    NUM_Display(rtc.getMinute() / 10, 51, 20, change_M1, 12, 15);
    NUM_Display(rtc.getMinute() % 10, 51 + 15, 20, change_M2, 12, 15);

    u8g2.drawVLine(83, 25, 5);
    u8g2.drawVLine(83, 40, 5);

    NUM_Display(rtc.getSecond() / 10, 88, 20, change_S1, 12, 15);
    NUM_Display(rtc.getSecond() % 10, 88 + 15, 20, change_S2, 12, 15);

    u8g2.sendBuffer(); // 发送缓冲区数据
  } while (change_H1[7] < 15 || change_H2[7] < 15 || change_M1[7] < 15 || change_M2[7] < 15 || change_S1[7] < 15 || change_S2[7] < 15);
  change_H1[7] = 0;
  change_H2[7] = 0;
  change_M1[7] = 0;
  change_M2[7] = 0;
  change_S1[7] = 0;
  change_S2[7] = 0;
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

void Button_Scan(void)
{
  if (digitalRead(17) == LOW && Button_Flag == 0)
  {
    delay(20);
    if (digitalRead(17 == LOW) && Button_Flag == 0)
    {
      u8g2.clearBuffer();
      if (mode_flag == 0)
      {
        mode_flag = 1;
        Display_Mode2();
      }
      else if (mode_flag == 1)
      {
        mode_flag = 2;
        Display_Mode3();
      }
      else if (mode_flag == 2)
      {
        mode_flag = 3;
        Display_Mode4();
      }
      else if (mode_flag == 3)
      {
        mode_flag = 0;
        Display_Mode1();
      }
      Button_Flag = 1; // 标志等于1表示被按下
    }
  }
  else if (digitalRead(17) == HIGH && Button_Flag == 1)
  {
    Button_Flag = 0; // 标志等于0表示未被按下
  }
}

void HTTP_LinkError_Handle(void)
{
  int Link_Time = 0; // 记录重连次数
  int httpCode1 = 0;
  int httpCode2 = 0;
  do
  {
    // 重新获取连接
    HTTPClient http;                                    // 创建HTTP对象
    http.begin(url + "?city=" + citys + "&key=" + key); // 访问指定URl获取天气信息
    httpCode1 = http.GET();                             // 接受HTTP相应状态码
    response = http.getString();                        // 获取天气正文
    http.end();                                         // 结束HTTP连接

    http.begin(url2);             // 访问指定URl获取时间信息
    httpCode2 = http.GET();       // 接受HTTP相应状态码
    response2 = http.getString(); // 获取时间正文
    http.end();                   // 结束HTTP连接
    Link_Time++;                  // 记录重连次数
    // 重新连接获取UI
    u8g2.clearBuffer();
    u8g2.setCursor(0, 12);
    u8g2.printf("重新获取数据中(%d)", Link_Time);
    u8g2.setCursor(0, 24);
    u8g2.printf("HTTPCode1=%d", httpCode1);
    u8g2.setCursor(0, 36);
    u8g2.printf("HTTPCode2=%d", httpCode2);
    u8g2.sendBuffer();
    delay(100);
  } while (httpCode1 != 200 && httpCode2 != 200);
  u8g2.setCursor(0, 48);
  u8g2.printf("重新获取成功！");
  u8g2.sendBuffer();
  delay(1000);
}

void Move_Cursor(int GoalValue, float *CurrentValue)
{
  float Error = GoalValue - *CurrentValue; // 误差等于目标值减去实际值
  if (fabs(Error) > 1)                     // 误差大于1
  {
    *CurrentValue += Error / 4; // 实际值加上误差除以10
  }
  else
    *CurrentValue = GoalValue; // 误差小于1 直接等于目标值
  // delay(1);
}

void NUM_Display(int num, int x, int y, float change[], int W, int H)
{
  int Goal_Value[7] = {0, 0, 0, 0, 0, 0, 0}; // 目标数组
  switch (num)
  {
  case 0:
    Goal_Value[0] = W;
    Goal_Value[1] = 0;
    Goal_Value[2] = W;
    Goal_Value[3] = H;
    Goal_Value[4] = H;
    Goal_Value[5] = H;
    Goal_Value[6] = H;
    break;

  case 1:
    Goal_Value[0] = 0;
    Goal_Value[1] = 0;
    Goal_Value[2] = 0;
    Goal_Value[3] = 0;
    Goal_Value[4] = H;
    Goal_Value[5] = 0;
    Goal_Value[6] = H;
    break;

  case 2:
    Goal_Value[0] = W;
    Goal_Value[1] = W;
    Goal_Value[2] = W;
    Goal_Value[3] = 0;
    Goal_Value[4] = H;
    Goal_Value[5] = H;
    Goal_Value[6] = 0;
    break;

  case 3:
    Goal_Value[0] = W;
    Goal_Value[1] = W;
    Goal_Value[2] = W;
    Goal_Value[3] = 0;
    Goal_Value[4] = H;
    Goal_Value[5] = 0;
    Goal_Value[6] = H;
    break;

  case 4:
    Goal_Value[0] = 0;
    Goal_Value[1] = W;
    Goal_Value[2] = 0;
    Goal_Value[3] = H;
    Goal_Value[4] = H;
    Goal_Value[5] = 0;
    Goal_Value[6] = H;
    break;

  case 5:
    Goal_Value[0] = W;
    Goal_Value[1] = W;
    Goal_Value[2] = W;
    Goal_Value[3] = H;
    Goal_Value[4] = 0;
    Goal_Value[5] = 0;
    Goal_Value[6] = H;
    break;

  case 6:
    Goal_Value[0] = W;
    Goal_Value[1] = W;
    Goal_Value[2] = W;
    Goal_Value[3] = H;
    Goal_Value[4] = 0;
    Goal_Value[5] = H;
    Goal_Value[6] = H;
    break;

  case 7:
    Goal_Value[0] = W;
    Goal_Value[1] = 0;
    Goal_Value[2] = 0;
    Goal_Value[3] = 0;
    Goal_Value[4] = H;
    Goal_Value[5] = 0;
    Goal_Value[6] = H;
    break;

  case 8:
    Goal_Value[0] = W;
    Goal_Value[1] = W;
    Goal_Value[2] = W;
    Goal_Value[3] = H;
    Goal_Value[4] = H;
    Goal_Value[5] = H;
    Goal_Value[6] = H;
    break;

  case 9:
    Goal_Value[0] = W;
    Goal_Value[1] = W;
    Goal_Value[2] = W;
    Goal_Value[3] = H;
    Goal_Value[4] = H;
    Goal_Value[5] = 0;
    Goal_Value[6] = H;
    break;

  default:
    break;
  }

  Move_Cursor(Goal_Value[0], &change[0]);
  Move_Cursor(Goal_Value[1], &change[1]);
  Move_Cursor(Goal_Value[2], &change[2]);
  Move_Cursor(Goal_Value[3], &change[3]);
  Move_Cursor(Goal_Value[4], &change[4]);
  Move_Cursor(Goal_Value[5], &change[5]);
  Move_Cursor(Goal_Value[6], &change[6]);

  Move_Cursor(H, &change[7]); // 计数用

  u8g2.drawHLine(x + 1, y, change[0]);             // 上横线
  u8g2.drawHLine(x, y + H, change[1]);             // 中横线
  u8g2.drawHLine(x + 1, y + 2 * H - 1, change[2]); // 下横线

  u8g2.drawVLine(x, y, change[3]);         // 左上竖线
  u8g2.drawVLine(x + W, y + 1, change[4]); // 右上竖线
  u8g2.drawVLine(x, y + H, change[5]);     // 左下竖线
  u8g2.drawVLine(x + W, y + H, change[6]); // 右下竖线
}