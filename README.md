# B站视频下载，mp3下载工具  
Bilibili官网 - https://www.bilibili.com/  
## 声明  
软件均仅用于学习交流，请勿用于任何商业用途！感谢大家！  
## 介绍 
1.工具只支持Windows系统，win10上可正常运行。  
2.代码使用VS2019开发，界面库用的开源duilib，项目包括几个主要模块：异步线程池、下载中心、音视频编解码，视频下载后自动转换成mp3保存到本地，支持断点续传。  
3.视频下载接口资料来自网络，仅限于学习交流，请勿用于任何商业用途。

使用的开源库有:  
duilib: https://github.com/qdtroy/DuiLib_Ultimate  
easylogging: https://github.com/amraynonweb/easyloggingpp    
libcurl: https://github.com/curl/curl  
openssl: https://github.com/openssl/openssl  
zlib: http://www.zlib.net/  
ffmpeg: http://ffmpeg.org/   
mp3lame: https://lame.sourceforge.io/  
jsoncpp: https://github.com/open-source-parsers/jsoncpp  
restclient: https://github.com/mrtazz/restclient-cpp  
libjpeg-turbo: https://libjpeg-turbo.org/   
sqlite: https://www.sqlite.org/index.html   
## 更新记录  
### 2024年2月25日 -- 1.6.0 
+ 1.更新B站登录API，解决无法登录的问题。 
+ 2.更新B站用户信息API，解决登陆后请求用户信息失败的问题。 
### 2022年11月6日 -- 1.5.0 
+ 1.引入开源库sqlite3、turbojpeg。 
+ 2.支持写入mp3封面图片。 
+ 3.解决下载信息窗口码率下拉列表显示的bug。
### 2022年9月12日 -- 1.4.0
+ 1.增加打开下载文件夹功能。
### 2022年6月12日 -- 1.3.0
+ 1.监控剪贴板，支持赋值后直接打开下载窗口。  
+ 2.修改下载文件命名规则。  
### 2022年5月21日 -- 1.2.0
+ 1.支持自动登录。  
+ 2.UI调整，添加窗口阴影。  
### 2022年5月13日 -- 1.1.0  
+ 1.增加二维码登录功能，支持下载高清视频。  
+ 2.UI调整。  
+ 3.修复若干bug。  
## 使用截图 
![](https://raw.githubusercontent.com/JelinYao/BVLoader/main/Bin/img/screen1.png)  

![](https://raw.githubusercontent.com/JelinYao/BVLoader/main/Bin/img/screen2.png)  

![](https://raw.githubusercontent.com/JelinYao/BVLoader/main/Bin/img/screen3.png)  

![](https://raw.githubusercontent.com/JelinYao/BVLoader/main/Bin/img/screen4.png)  

![](https://raw.githubusercontent.com/JelinYao/BVLoader/main/Bin/img/screen5.png)  
 
