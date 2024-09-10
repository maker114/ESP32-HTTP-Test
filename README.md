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
- 同时更改了主函数的循环结构，现在主函数将以约10ms的周期循环
