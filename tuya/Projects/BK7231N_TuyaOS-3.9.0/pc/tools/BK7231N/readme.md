# BK7231N开发平台原厂烧录工具使用说明

## 烧录流程介绍
### 1.打开原厂烧录工具

以管理员身份打开名为 `bk_writer_gui_V1.6.3.exe` 的可执行文件。

![image.png](https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/content-platform/hestia/165907441218640e2f422.png)

### 2. 烧录对象

烧录对象选择 BK7231n

![image.png](https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/content-platform/hestia/16590745837d8e0658731.png)

### 3.烧录地址

当我们烧录UA文件的时候，需要把起始地址设置为： `0X00011000`。

当我们烧录QIO文件的时候，需要把起始地址设置为：`0X00000000`，开发者拿到手上的模组一般都是经过烧录授权的，所以只需要烧录UA文件即可。


![image.png](https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/content-platform/hestia/1659074935596a531462f.png)

### 4.选择烧录文件

1. 第一步选择浏览
2. 第二步选择编译生成的UA文件
3. 第三步点击打开

![image.png](https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/content-platform/hestia/16590754123b6df3482bd.png)

### 5.选择Efuse Key 文件
一般情况下，`Efuse Key`  文件是默认选择好的。

1. 第一步选择浏览
2. 第二步 选择 `efuse_7231N.txt`文件。这个文件和`bk_writer_gui_V1.6.3.exe`  原厂烧录工具在同一个目录下。
3. 第三步点击打开。

![image.png](https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/content-platform/hestia/1659075730e40ab4d7972.png)

### 6.选择COM口
如果开发者不知道是那个下载口，可打开计算机设备管理器，点击端口下拉菜单。
连接、断开模组的下载口，观察端口的变化，即可看到模组的下载口。

![image.png](https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/content-platform/hestia/1659076018447b23fc4f1.png)

选择COM口。
![image.png](https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/content-platform/hestia/165907598950275a373a2.png)

### 7.烧录
点击烧录，同时按下模组的复位按钮，即可完成烧录流程。

![image.png](https://airtake-public-data-1254153901.cos.ap-shanghai.myqcloud.com/content-platform/hestia/16590762120baa5a4a7c1.png)


