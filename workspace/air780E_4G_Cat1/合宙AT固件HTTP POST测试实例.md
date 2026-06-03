<font style="color:#DF2A3F;">AT固件不支持RS485发送，支持TTL，RS232等发送。</font>

# 一、简介
AT固件是模块厂家提供的固件，命令灵活，功能全面，命令繁多。支持电话，短信，FTP,蓝牙，TCP，MQTT，HTTP等功能，但是需要发送AT命令去维护网络，检查模块状态，发送数据。所以你需要按某些顺序去发命令，并且检测应答结果，只有满足条件后再向下执行。

# 二、开发建议
1、能用DTU透传固件的，优先用DTU透传固件，实在不行的或者特殊需求才考虑AT固件。

2、AT固件的难点在于控制流程和业务稳定性维护。

3、调试方面AT固件，MCU使用串口与4G模块通讯，先通过电脑串口，熟悉AT命令与4G模块的通讯过程，了解发送和返回的数据格式，对代码设计和数据处理有很大帮助。所以在调试的时候，不要一开始就用MCU去写代码，而是用电脑串口先发命令测试模块。

4、熟悉数据格式，可以用串口工具的HEX模式，看细节，特别注意回车换行或者HEX数据。

<font style="color:#DF2A3F;">5、串口通讯硬件无关，只要控制串口的收发数据即可。了解模块的控制方法，熟悉模块的数据格式，所以无MUC参考代码。</font>

# 三、工具简介
## 3.1、银尔达测试服务器
DTU测试平台:[http://test.yinerda.com](http://test.yinerda.com)

在浏览器中打开链接可以获取到到测试服务器，支持TCP/UDP/MQTT/HTTP协议，方便在没有服务器的情况下快速评估4G模块，也可以用来排查你服务器是否正常，如果测试服务器正常，自己的服务器不行，大概率就是自己服务器问题。

注意:浏览器工具只是用来测试和验证设备使用。10分钟没有任何交互会自动关闭服务器，如果发现连接不上了，重新刷新浏览器，重新打开，获取新资源测试。

<img src="https://cdn.nlark.com/yuque/0/2024/png/804193/1715654303855-55efccf5-612e-4c85-8199-acd59076859a.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_54%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="1880" title="" crop="0,0,1,1" id="ua9d7009a" class="ne-image">

## 3.2、串口工具YED-UUART-211
USB转串口调试工具:"[YED-UUART-211](https://yinerda.yuque.com/yt1fh6/4gdtu/rfvpd0gwbr6vhfb4)"，集成电源，TTL，RS232，RS485专门为设备调试设计,或者任意自己熟悉的串口调试工具,使用教程参考:

[YED-UUART-211](https://yinerda.yuque.com/yt1fh6/4gdtu/nggppiz5x1gi9fe5)

绝大部分串口都是GND接GND，TX接RX，RX接TX；偶尔会遇到客户是GND接GND，TX接RX，RX接TX方法。

注意:自己的串口工具，要保证供电电源稳定；自己的串口工具如果是独立供电GND需要共地。如果用自己的工具发现串口不通，排查固件问题后，先重点参考银尔达提供的YED-UUART-211工具的使用方法。<font style="color:#DF2A3F;">电脑USB供电的工具，都无法给4G模块供电，供电不足，比如以下这些常见串口工具供电都不行。充电器，充电宝这些供电也不行。</font>



<img src="https://cdn.nlark.com/yuque/0/2024/png/804193/1715583894710-9b0bf409-53ab-4ff9-9078-0796d3a46317.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_22%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="755" title="" crop="0,0,1,1" id="KGzH3" class="ne-image">

## 3.3、串口调试软件
串口测试软件:

[测试工程、工具、驱动等](https://yinerda.yuque.com/yt1fh6/4gdtu/rfvpd0gwbr6vhfb4)

软件使用方法:

[YEDTestTools软件使用方法](https://yinerda.yuque.com/yt1fh6/4gdtu/tl1vaqdylayghdhz)

# 五、HTTP POST 控制流程
<font style="color:#DF2A3F;">特别注意:4G模块在上电、重启的时候，主动发出开机日志，这些日志一般需要过滤。然后再去执行AT命令。</font>

![画板](https://cdn.nlark.com/yuque/0/2024/jpeg/804193/1715689950708-d3ee59c8-59c6-40f2-9dab-80ba00fa9afe.jpeg)

# 六、HTTP POST 模式测试
按顺序给模块发命令，用电脑测试正常后，熟悉模块工作逻辑和数据格式后，就可以把发送命令流程写到自己的MCU里面。

银尔达提供串口调试工具和测试工程，推荐使用。

| 序号 | 步骤 | 实例 |
| --- | --- | --- |
| 1 | 给设备模块上电，LED会闪烁，串口有日志输出。实际在MCU使用的时候，模组启动收到的信息都丢弃不处理 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/804193/1722850411181-b2f6c86d-f046-4e84-acfa-b02b56597065.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_18%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="616" title="" crop="0,0,1,1" id="uc155a0ae" class="ne-image"> | |
| 2 | 发AT\r,确认开机，自适应波特率，如果收到AT\r\nOK\r\n表示正常,9000~230400~都可以自适应<br/>实际在MCU使用，确认波特率之前的数据都丢弃不处理 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730191181876-2aa6841e-de0c-458c-9c43-754adb192ed2.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_10%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="288.8" title="" crop="0,0,1,1" id="ud5a0029c" class="ne-image"> | |
| 3 | 发ATE0\r,关闭回显。关闭回显不是必须的，没关闭回显的时候会重复你发的数据，可能会影响你数据解析，建议关闭 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192447897-fe06c27d-66cb-451a-ad78-18ef5cfa0783.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_23%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="649.6" title="" crop="0,0,1,1" id="u645c53c3" class="ne-image"> | |
| 4 | 确认SIM卡插好，查询SIM卡的ICCID。只有有SIM卡了，才可能联网。如果查不到，卡可能没查好或者反了或者卡槽坏了<br/><font style="color:#DF2A3F;">ICCID是管理SIM卡的重要号码，建议传送给服务器方便你后续管理设备。</font> | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192534881-22ed43dc-63ef-4c47-9bdc-235959280d7d.png?x-oss-process=image%2Fcrop%2Cx_40%2Cy_0%2Cw_450%2Ch_192" width="360" title="" crop="0.0816,0,1,1" id="u2533253f" class="ne-image"> | |
| 5 | 确认是否附着网络，只有返回1表示基站正常，然后再执行下面操作。如果一直为0，可能卡停机了或者天线没接 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192554811-513163da-fb24-42b5-a5f8-f20b7750154d.png?x-oss-process=image%2Fcrop%2Cx_40%2Cy_0%2Cw_448%2Ch_171" width="358" title="" crop="0.0821,0,1,1" id="u25b19471" class="ne-image"> | |
| 6 | 设置PDP承载类型，固定格式 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730271051022-d818dc35-ca52-47ec-a4ca-0b8a866c2cb5.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_15%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="408.8" title="" crop="0,0,1,1" id="u22c23a63" class="ne-image"> | |
| 7 | 设置PDP承载的APN，固定格式 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730271061331-af0875ce-dc7a-4977-8d8c-3db619d8db3b.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_13%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="368" title="" crop="0,0,1,1" id="ua84fd2ea" class="ne-image"> | |
| 8 | 激活PDP，固定格式 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730271069823-be73c4b4-f468-4ecb-a1ac-00f6527ee74b.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_11%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="301.6" title="" crop="0,0,1,1" id="u31e341ed" class="ne-image"> | |
| 9 | 查询PDP IP地址，获取到IP地址后详细执行 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730271079685-65b6f6df-0958-4546-9bd5-2520a4ad8e7d.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_13%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="356.8" title="" crop="0,0,1,1" id="u8c27b86a" class="ne-image"> | |
| 10 | 初始化HTTP，固定格式 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730271087678-0bc756e4-c292-474d-bee6-66593b0f0ea9.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_11%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="306.4" title="" crop="0,0,1,1" id="u8ca9a33e" class="ne-image"> | |
| 11 | 设置HTTP 承载PDP上下文，固定格式 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730271099466-73c74bfa-5b63-4c0b-ad4d-0e3111f54bc3.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_13%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="376.8" title="" crop="0,0,1,1" id="u39154e44" class="ne-image"> | |
| 12 | 设置HTTP 数据类型，这个是不必须的，根据服务器需求来定 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730273881458-4fc620f9-c7a9-4e59-87eb-5dd1c0538282.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_17%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="464" title="" crop="0,0,1,1" id="u0b2b3a9d" class="ne-image"> | |
| 13 | 设置HTTP参数连接，支持IP和域名。<br/>浏览器打开[http://test.yinerda.com](http://test.yinerda.com)，选择“HTTP测试工具”，点击"打开"可以获取测试服务器信息 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730274325414-5f7d6fc7-7414-41e5-985a-8a89c3f54ab1.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_47%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="1321.6" title="" crop="0,0,1,1" id="uf27de728" class="ne-image"> | |
| 14 | 关闭HTTP，在关闭之前，之前的参数一直有效，关闭后需要重新初始化<br/>如果HTTP异常可以关闭，但是不必须。如果没关闭，后续只需要提交数据，提交请求即可 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730272065058-bc4aeffe-8799-48ab-b6f9-267b9a0bfb1d.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_25%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="708" title="" crop="0,0,1,1" id="ub062d11a" class="ne-image"> | |


# 七、使用建议
HTTP POST可以传16进制数据。
