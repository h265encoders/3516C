## introduction of the scheme
---
ENC1 is an open source single channel encoder product, which supports 1080P high and low code stream coding, H265, RTSP, RTMP, onvif, etc., and can dock with the live broadcasting platform and security system.

![输入图片说明](https://images.gitee.com/uploads/images/2019/0604/203350_02d42046_1759637.png "屏幕截图.png")

## The source code is introduced
---
Encoder project is the main program of the Encoder, which controls coding, streaming and other functions through RPC communication with web pages.

Wifi Ctrl project is used to control Wifi module (optional)

Gpio project is used to respond to the event of the Reset button to restore the factory configuration after a long press of 5 seconds.


###
