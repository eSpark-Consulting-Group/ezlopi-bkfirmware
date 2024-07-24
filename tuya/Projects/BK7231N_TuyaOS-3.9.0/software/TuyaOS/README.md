# TuyaOS 快速体验

## 简介

[`TuyaOS `](https://developer.tuya.com/cn/tuyaos) 是基于 `RTOS`、`Linux`、`Non-OS` 等设计的应用于物联网（`IoT`）领域面向全连接、全场景的分布式跨平台操作系统，具备轻量级、互联互通、安全传输、组件丰富、快速开发等关键能力，广泛应用于电工照明、大小家电、视频监控、中控网关、安防传感、工业农业等物联网设备。 

## 目录

```
<TuyaOS-x.x.x-tuyaos-iot-xxxxxxx>
|---adapter						#适配层定义
|---apps						#demo程序
|---build						#编译配置
|---docs						#文档
|---include						#头文件，包括公共头文件和组件头文件
|---libs						#libtuyaos-iot/ipc/gw.a，独立链接的.a文件
|---scripts						#工具、脚本
|---vendor						#原厂库，空，第一次编译按需下载
|---build.sh/py					#tuyaos编译入口
|---README.md
|---LICENSE
|---CHANGELOG
|---RELEASELOG
```

## 开发

通过 [`TuyaOS Wind IDE`](https://developer.tuya.com/cn/ide) 可以快速下载 [`TuyaOS `](https://developer.tuya.com/cn/tuyaos) 开发框架，并创建产品开发工程。具体操作请参照[获取`TuyaOS`开发框架](https://developer.tuya.com/cn/docs/iot-device-dev/Development-Tools?id=Kb0vhc1zz50dk)。

`TuyaOS` 开发框架提供了演示程序，帮助开发者更快的了解如何基于 `TuyaOS` 开发框架进行二次开发，细节请参照 `apps` 目录下的演示程序源码。同时也提供演示程序的快速学习文档，操作方式请参照 [Demo程序Quick Start](https://developer.tuya.com/cn/docs/iot-device-dev/Development-Tools?id=Kb0vhc1zz50dk)。

## 编译

通过 `IDE` 提供了方便快捷的一键编译功能来编译用户应用程序或者是`TuyaOS`开发框架自带的演示程序，具体操作流程请参照[编译Demo](https://developer.tuya.com/cn/docs/iot-device-dev/Development-Tools?id=Kb0vhc1zz50dk)。

或者是在 `IDE` 的终端界面执行命令编译用户应用程序或者是`TuyaOS`开发框架自带的演示程序，示例如下（每个框架带的演示程序名称不一样，此处假设示例程序名称`tuyaos_demo_rtos_func`）：

```shell
./build_app.sh apps/tuyaos_demo_rtos_func tuyaos_demo_rtos_func 1.0.0
```

生成的产物保存于`./apps/tuyaos_demo_rtos_func/output/`目录下，详情参照示例程序的帮助文档。

## 烧录

在实际生产过程中，需要有专门的生产系统和工具。对于生产系统和工具，涂鸦有一套完整的智慧生产体系，可以参考[烧录授权](https://developer.tuya.com/cn/docs/iot/flash-and-authorize?id=Kaytfatwdbfj2)来自己开发的产品仅是批量的烧录和授权。

## FAQ

在使用过程中，如果遇到问题，可以访问涂鸦开发者平台[Wi-Fi设备接入-故障排除](https://developer.tuya.com/cn/docs/iot-device-dev/faq?id=Kaiuyfi4402ee)，按照自己遇到的问题分类，寻找合适的解决方法。
