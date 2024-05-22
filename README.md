### NativeSFDemo
基于Android Graphics显示系统APIs开发的Native演示小程序

#### 详细说明
参见文章：关注公众号 "Android元宇宙"，图形图像专栏获取程序解读
![图片](https://github.com/yrzroger/NativeSFDemo/assets/18068017/e4ddc7ce-cb94-4029-847c-cdabaa5f5dcd)


#### 分支
main : 基于Android 12平台开发，采用旧有的native_window_xxxx api写作方式  
main_bbq : 基于Android 12平台开发，采用BLASTBufferQueue写作方式  
android_u ：基于Android 14平台开发，采用旧有的native_window_xxxx api写作方式  
android_u_bbq ：基于Android 14平台开发，采用BLASTBufferQueue写作方式  


#### 使用方法
1. 下载代码放到android源码目录下
2. 执行mm编译获得可执行档NativeSFDemo
3. adb push NativeSFDemo /system/bin/
4. adb shell NativeSFDemo 运行程序


#### 效果展示
红色->蓝色->绿色背景交替展示

![puan0-iz683](https://user-images.githubusercontent.com/18068017/146721508-e78d69ca-0e93-4ae6-b76a-94a7c62b5bc3.gif)

#### 多屏情况下展示
1. 通过dumpsys display 获取每一个屏幕的layerStack
2. 执行adb shell NativeSFDemo -d layerStack，这样就可以显示到指定的屏幕上

![录制_2023_10_22_15_54_34_808](https://github.com/yrzroger/NativeSFDemo/assets/18068017/2d1f0c41-7ac9-4dec-a887-c8d38b2bed7d)
