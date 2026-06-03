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
DTU测试平台:

[银尔达工具](http://test.yinerda.com)

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

# 五、TCP透传模式控制流程
<font style="color:#DF2A3F;">特别注意:4G模块在上电、重启的时候，主动发出开机日志，这些日志一般需要过滤。然后再去执行AT命令。</font>

![画板](https://cdn.nlark.com/yuque/0/2024/jpeg/804193/1715666208673-f13bb3ca-aafc-497d-aa8a-e1afd75d1b87.jpeg)

# 六、UDP透传模式测试
按顺序给模块发命令，用电脑测试正常后，熟悉模块工作逻辑和数据格式后，就可以把发送命令流程写到自己的MCU里面。

银尔达提供串口调试工具和测试工程，推荐使用。

| 序号 | 步骤 | 实例 |
| --- | --- | --- |
| 1 | 给设备模块上电，LED会闪烁，串口有日志输出。实际在MCU使用的时候，模组启动收到的信息都丢弃不处理 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/804193/1722850448677-f1775d63-1714-4084-b66f-ff1877daa9b7.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_18%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="616" title="" crop="0,0,1,1" id="uf91127ed" class="ne-image"> | |
| 2 | 发AT\r,确认开机，自适应波特率，如果收到AT\r\nOK\r\n表示正常,9000~230400~都可以自适应<br/>实际在MCU使用，确认波特率之前的数据都丢弃不处理 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730191181876-2aa6841e-de0c-458c-9c43-754adb192ed2.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_10%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="288.8" title="" crop="0,0,1,1" id="ud5a0029c" class="ne-image"> | |
| 3 | 发ATE0\r,关闭回显。关闭回显不是必须的，没关闭回显的时候会重复你发的数据，可能会影响你数据解析，建议关闭 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192447897-fe06c27d-66cb-451a-ad78-18ef5cfa0783.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_23%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="649.6" title="" crop="0,0,1,1" id="u645c53c3" class="ne-image"> | |
| 4 | 确认SIM卡插好，查询SIM卡的ICCID。只有有SIM卡了，才可能联网。如果查不到，卡可能没查好或者反了或者卡槽坏了<br/><font style="color:#DF2A3F;">ICCID是管理SIM卡的重要号码，建议传送给服务器方便你后续管理设备。</font> | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192534881-22ed43dc-63ef-4c47-9bdc-235959280d7d.png?x-oss-process=image%2Fcrop%2Cx_40%2Cy_0%2Cw_450%2Ch_192" width="360" title="" crop="0.0816,0,1,1" id="u2533253f" class="ne-image"> | |
| 5 | 确认是否附着网络，只有返回1表示基站正常，然后再执行下面操作。如果一直为0，可能卡停机了或者天线没接 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192554811-513163da-fb24-42b5-a5f8-f20b7750154d.png?x-oss-process=image%2Fcrop%2Cx_40%2Cy_0%2Cw_448%2Ch_171" width="358" title="" crop="0.0821,0,1,1" id="u25b19471" class="ne-image"> | |
| 6 | 设置UDP为单链接。可以不设置，默认就是 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192588226-3877b676-a10d-4767-b327-d566d626dbd4.png?x-oss-process=image%2Fcrop%2Cx_34%2Cy_0%2Cw_407%2Ch_145" width="326" title="" crop="0.0765,0,1,1" id="u22c91833" class="ne-image"> | |
| 7 | 设置为透明模式。 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730251761671-d8629a6c-abcc-488e-90ac-db90693f90ee.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_12%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="335.2" title="" crop="0,0,1,1" id="u775366ef" class="ne-image"> | |
| 8 | 设置APN，大部分卡只需要设置"","",""即可，模块自动选择APN，如果卡有特殊APN需要设置正确的APN，才能正常。APN信息询问卡供应商，银尔达的设置""即可。 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192729500-4e1a7481-06c9-4a65-9300-5cd289a06443.png?x-oss-process=image%2Fcrop%2Cx_41%2Cy_0%2Cw_454%2Ch_127" width="363" title="" crop="0.0833,0,1,1" id="ubde23e3d" class="ne-image"> | |
| 9 | 激活网络，设置APN后激活网络，只能激活一次 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192808302-cd4a7166-00cc-4ac2-a5a7-2b65bec57854.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_11%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="305.6" title="" crop="0,0,1,1" id="ude4e7c0c" class="ne-image"> | |
| 10 | 查询IP，只有激活网络后，查询到IP后才能进行下一步。<br/>这个IP是基站分配的地址，大部分是内网地址，不用用来直接通讯。 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192827192-d9d7dfa7-d5ec-40a5-9d3f-7e641764cf59.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_11%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="307.2" title="" crop="0,0,1,1" id="uab5ec993" class="ne-image"> | |
| 11 | 创建UDP连接，支持IP和域名。用域名直接替换IP即可。连接成功后会返回CONNECT OK<br/>可以用自己的服务器，替换IP和端口，测试的时候可以用银尔达测试服务器获取IP<br/>浏览器打开[http://test.yinerda.com](http://test.yinerda.com)，选择“UDP测试工具”，点击"打开"可以获取测试服务器信息<br/>4g 模块创建UDP后，服务器是没反应的，必须主动给服务器发数据，服务器才会显示连接 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192872804-d8410b3a-fd7e-4ec0-a40e-5beb6090ac9e.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_18%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="497.6" title="" crop="0,0,1,1" id="u88ea8115" class="ne-image"><br/><img src="https://cdn.nlark.com/yuque/0/2024/png/804193/1715666042192-98c4d282-ba3a-44f8-97ec-afbf1eb21020.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_48%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="1668" title="" crop="0,0,1,1" id="ucae42d18" class="ne-image"> | |
| 12 | 串口发送数据，收到CONNECT OK后表示连接服务器，设备就自动进入透传模式了，串口直接发数据即可 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730251937545-4e9832a9-8183-4f00-a2fe-f0005a6df259.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_33%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="938.4" title="" crop="0,0,1,1" id="u50a33d28" class="ne-image"> | |
| 13 | 服务器发任意数据，串口能收到 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730252025823-02eb5c3c-009e-4a3b-812c-4ee931680580.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_34%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="942.4" title="" crop="0,0,1,1" id="u46d7e641" class="ne-image"> | |
| 14 | 如果中途要退出透传模式，可以发+++退出透传模式，然后之前其他AT命令<br/>注意+++前后不能有数据，间隔1秒以上，否则可能被认为数据透传到服务器了 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730252056173-67d46f35-018e-4cd9-8321-bb64d6aa1fcf.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_16%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="280" title="" crop="0,0,1,1" id="ue2e07226" class="ne-image"> | |
| 15 | 如果要从命令模式进入透传模式，可以发ATO\r，重新进入。<br/>收到 CONNECT  表示服务器正常，如果连接异常，需要重新去连接服务器 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730252078242-f8a980bb-d3a8-46de-a849-26e12be4f857.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_14%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="256" title="" crop="0,0,1,1" id="uf96062b2" class="ne-image"> | |


# 七、异常排查
UDP是无连接的，虽然流程和TCP一样。但是注意

1、UDP协议必须设备主动给服务器发数据，服务器才能给设备发数据，因为设备是内网地址。

2、UDP协议不会有错误，但是数据实际可能没发出去，所以必须设计业务一问一答的方式，否则你不知道服务器是否收到了。

3、UDP协议需要周期给服务器通讯，以保存连接，建议2-3分钟通讯一次。



## 