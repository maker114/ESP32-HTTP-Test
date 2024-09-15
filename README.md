# ESP32 HTTP Test


这是一个基于ESP32开发板的联网小时钟，同时也被用于测试各种有关远程仓库的内容
##### 详情信息  
|开发板|开发平台|编程框架|  
|:--:|:--:|:--:|  
|ESP32|VScode+platformIO|Arduino|

---
## 更新日志
### 2024.09.09：  
- 修改了WiFi连接与HTTP有关的代码，使其更加简洁与规范
- 修改了按键的代码，但仍然存在部分问题
- 由于运行开发板的变更，删除了有关温度传感器有关的代码
- 将所有代码进行规范化整理，并删除了部分冗余代码
- 新建Simplify分支（已合并）

### 2024.09.10：
- 修改了程序的结构，现在显示模式1与2将作为单独的子函数存在
- 完善了按键有关的代码，现在可以正常且快速的切换显示模式
- 加入了有关HTTP重连的代码，现在在HTTP返回值不为200时自动重连
- 更改了主函数的循环结构，现在主函数将以约10ms的周期循环

### 2024.09.11：
- 修改了显示模式3的代码，使用了较低功耗不同的字体（平均电流48ma）
- 修改了显示模式4的代码，解决了其卡顿与部分段码偏移的问题
- 修改了WIFI的连接方式，现在再每次更新数据后会自动关闭WIFI降低功耗
- 将显示函数改为由RTC进行触发，这样可以使显示数字的变动与RTC同步

### 2024.09.12：
- 彻底完善了模式四的段码偏移问题
- 修改了WIFI的初始化内容，现在会自动在存储的WIFI列表内自动匹配并连接，
- 会将上一次成功连接的编号存入EEPROM，下一次可跳过选择WIFI的步骤
- 现在在无法重连至WIFI时会自动终止天气更新程序并跳转至模式四以节省电量
- 现在模式三与模式四不再会自动更新天气
- 增加了main.h文件，用于存放函数定义

### 2024.09.13：
- 增加了手动更新天气的选项，现在可以通过长按按键2来手动更新天气
- 天气更新程序增加手动模式以忽略时间检测

### 2024.09.14：
- 为启动界面添加了动画，同时简化了配网的UI
- 为启动界面与模式一之间添加了动画
- 为模式一的数字变化添加了动画