<font style="color:#DF2A3F;">特别注意:4G模块在上电重启的时候，主动发出开机日志，这些日志一般需要过滤。然后再去执行AT命令。</font>

# 一、YEDTestTools软件
软件包含了AT测试工程，写好了对应功能的命令，方便你快速执行命令测试功能。

[YEDTestTools软件使用方法](https://yinerda.yuque.com/yt1fh6/4gdtu/tl1vaqdylayghdhz)

# 二、银尔达提供AT手册
Air724官方AT命令手册下载：

[AT命令手册.zip](https://yinerda.yuque.com/attachments/yuque/0/2024/zip/43308887/1720700033297-05c07957-78ed-4d9f-af77-431c37e91048.zip)

Air780官方AT命令手册下载

[Air700ECT&Air780EHT&Air780EVT&Air780EGT&Air780EPT_AT指令手册V1.0.10.zip](https://yinerda.yuque.com/attachments/yuque/0/2026/zip/804193/1775532150871-a13b004d-e0d6-47f3-b7c3-535b876b5d7d.zip)

# 三、LED状态指示灯说明
   AT固件，只有1个LED可以观察状态，只有在0.125秒的时候，网络才正常。

| LED | 状态 | 说明 |
| --- | --- | --- |
| <br/>NET LED | 亮0.2秒，灭1.8秒 | 搜索状态 |
| | 亮1.8秒，灭0.2秒 | 待机 |
| | 亮0.125秒，灭0.125秒 | 激活网络 |


#  四、硬件稳定性设计建议
<font style="color:#DF2A3F;">硬件设计，强烈建议需要能够硬件控制4G模组重启，实现异常状态恢复逻辑，切记！</font>

<font style="color:#DF2A3F;">注意电源供电功率一定要充足，对4G模组供电的功率要求最少7.6W的功率，推荐10W。</font>

## 4.1、使用控制RST管脚复位模组
<img src="https://cdn.nlark.com/yuque/0/2024/png/804193/1709890489614-60973c10-1982-44c6-850f-38326e26ae3d.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_14%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="479" title="" crop="0,0,1,1" id="oiEEQ" class="ne-image">

## 4.2、使用控制模组供电方法复位模组