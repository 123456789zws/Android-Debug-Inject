
# 开发初衷
这个项目的开发主是因为于zygiskNext 老版本特征太多了，我改了很多，后来发现需要有些部分需要大改才能完成对抗，索性注入工具部分完全重写了。
zygiskNext rust部分，由于我不太熟悉，同时我希望zygisk 相关部分和 注入工具完全分离，所以抄了magisk zygisk部分的代码，用c重写了zygiskd。
在开发工程中我我又发现了很多可以扩展点，比如任意位置停止，以及init进程注入。


## 项目介绍


这个项目主要是以注入工具Android-Debug-Inject(adi)为主,zygisk 项目使用的是adi程序进行注入，zygisk可以说只是一个adi工具注入init进程的demo，
同时提供了另一个注入drm进程并且修改drm id的项目。


# 工程介绍

## 项目结构
+ adi 注入工具， 一个android 进程注入工具
+ zygisk,提供zygisk 以及zyiskd 服务，单独将zygsk 剥离，使他能够单独使用
+ ADLib, 这是一个注入 drm 进程 的demo,可以修改drm 的id，用于演示注入init子进程
+ module 跟zygisk 一样，用来编译工程的，会生成 zygiskADI 这个magisk模块。

## zygiskADI

通过adi工具注入zygisk，实现zygisk功能，目前已经完成了magisk 模块，可刷机使用。但是目前不建议直接刷入magisk模块开机自启，我暂时关闭了刷机自动启动的命令，可以去post-fs-data.sh 文件修改，开机执行adi程序



## 配置文件例子说明
通过 module/src/zygisk.json 配置文件,将监控zygote启动,并注入libzygisk.so文件  
通过 ADILib/src/main/cpp/inject.json 配置文件,将监控drm进程启动,并注入so文件

## 配置文件参数说明

```json  
{    
"traced_pid": 1,    要监控的父进程  
    "persistence": true,    暂时不用,后续可能会做持久化  
    "childProcess": [       要监控的进程数组  
       {"exec": "/vendor/bin/hw/android.hardware.drm@1.4-service.widevine",    监控的进程exec文件名字  
          "waitSoPath": "/apex/com.android.art/lib64/libart.so",                 等待这个so加载在继续执行  
          "waitFunSym": "",                 等待这个函数执行在继续执行   
"InjectSO": "/data/adb/modules/ZygiskADI/lib/arm64-v8a/libDrmHook.so",  要加载的so文件  
          "InjectFunSym": "DrmIdHook",         要执行的函数  
          "InjectFunArg": "11111111111193baf8cb6a22de8a5ae4bfecc174b1a9405dc71b8b3fac1c734f"    函数参数,目前只支持一个,会传入第二个参数,第一个为so的handle  
       }    
]  }  
  
waitSoPath 尽量不要不写  
waitFunSym 可以不写,如果不写,将在so加载以后直接加载so.  
  
  
waitSoPath和waitFunSym,一般是是配合,表示某个so的某个函数,但这个函数执行以后执行hook代码  
  
```  




## 问题
+ zygisk commpanion

  这个功能并未完全测试,lsp使用虽然测试了,但是lsp并未完全使用,目前不知道这个功能是否有问题,而且这个功能我使用的比较少,抄袭的magisk 的zygisk的代码

+ init 子进程的问题

  1、提供init子进程注入功能,但是目前的zygisk 设计框架可能不一定是适合,比如selinux权限这些  
  2、kernelsu 提供的挂在镜像的 /data/adb/module 更是无法支持,init ns里没有挂载这个目录 
  3、如果想要使用这个功能建议自己处理挂载路径和so权限问题,这比较简单的,而且我觉得是比较正常的.

+ 32 zygisk 不支持  
  android 有两个架构的zygote,但是现在32未程序已经很少了，后续看情况支持

+ 未进行大规模测试

  本程序并未进行大规模手机测试，目前只进行了红米手机测试


## 最后
感谢zygiskNext的开源代码,以及magisk 这些项目,也希望大家且看且珍惜,对开源项目多一点宽容，安利一波公众号，希望大家支持

![输入图片说明](doc/images/wx.jpg)

![输入图片说明](doc/images/start.jpg)




#  感谢
https://github.com/Dr-TSNG/ZygiskNext

https://github.com/topjohnwu/Magisk