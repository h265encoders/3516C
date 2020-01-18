## 方案介绍
---
ENC1是一个开源的单路编码器产品，支持1080P高低码流编码，支持H265，支持rtsp、rtmp、onvif等，可以对接直播平台、安防系统。

![输入图片说明](https://images.gitee.com/uploads/images/2019/0604/203350_02d42046_1759637.png "屏幕截图.png")

## 源码介绍
---
Encoder工程是编码器的主程序，通过和网页进行rpc通信来控制编码、串流等功能。

WifiCtrl工程用于控制wifi模块(选配)

Gpio工程用于响应Reset按键的事件，实现长按5秒后恢复出厂配置