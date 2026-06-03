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

# 五、MQTT控制流程
<font style="color:#DF2A3F;">特别注意:4G模块在上电、重启的时候，主动发出开机日志，这些日志一般需要过滤。然后再去执行AT命令。</font>

![画板](https://cdn.nlark.com/yuque/0/2024/jpeg/804193/1715669617492-573229f8-f045-4e07-9993-def77204f9bc.jpeg)

# 六、MQTT模式测试
按顺序给模块发命令，用电脑测试正常后，熟悉模块工作逻辑和数据格式后，就可以把发送命令流程写到自己的MCU里面。

银尔达提供串口调试工具和测试工程，推荐使用。

| 序号 | 步骤 | 实例 |
| --- | --- | --- |
| 1 | 给设备模块上电，LED会闪烁，串口有日志输出。实际在MCU使用的时候，模组启动收到的信息都丢弃不处理 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/804193/1722850436504-f5a52669-9298-438b-80f0-a08ccd7e3485.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_18%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="616" title="" crop="0,0,1,1" id="u70ac2277" class="ne-image"> | |
| 2 | 发AT\r,确认开机，自适应波特率，如果收到AT\r\nOK\r\n表示正常,9000~230400~都可以自适应<br/>实际在MCU使用，确认波特率之前的数据都丢弃不处理 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730191181876-2aa6841e-de0c-458c-9c43-754adb192ed2.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_10%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="288.8" title="" crop="0,0,1,1" id="ud5a0029c" class="ne-image"> | |
| 3 | 发ATE0\r,关闭回显。关闭回显不是必须的，没关闭回显的时候会重复你发的数据，可能会影响你数据解析，建议关闭 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192447897-fe06c27d-66cb-451a-ad78-18ef5cfa0783.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_23%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="649.6" title="" crop="0,0,1,1" id="u645c53c3" class="ne-image"> | |
| 4 | 确认SIM卡插好，查询SIM卡的ICCID。只有有SIM卡了，才可能联网。如果查不到，卡可能没查好或者反了或者卡槽坏了<br/><font style="color:#DF2A3F;">ICCID是管理SIM卡的重要号码，建议传送给服务器方便你后续管理设备。</font> | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192534881-22ed43dc-63ef-4c47-9bdc-235959280d7d.png?x-oss-process=image%2Fcrop%2Cx_40%2Cy_0%2Cw_450%2Ch_192" width="360" title="" crop="0.0816,0,1,1" id="u2533253f" class="ne-image"> | |
| 5 | 确认是否附着网络，只有返回1表示基站正常，然后再执行下面操作。如果一直为0，可能卡停机了或者天线没接 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192554811-513163da-fb24-42b5-a5f8-f20b7750154d.png?x-oss-process=image%2Fcrop%2Cx_40%2Cy_0%2Cw_448%2Ch_171" width="358" title="" crop="0.0821,0,1,1" id="u25b19471" class="ne-image"> | |
| 6 | 设置APN，大部分卡只需要设置"","",""即可，模块自动选择APN，如果卡有特殊APN需要设置正确的APN，才能正常。APN信息询问卡供应商，银尔达的设置""即可。 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192729500-4e1a7481-06c9-4a65-9300-5cd289a06443.png?x-oss-process=image%2Fcrop%2Cx_41%2Cy_0%2Cw_454%2Ch_127" width="363" title="" crop="0.0833,0,1,1" id="ubde23e3d" class="ne-image"> | |
| 7 | 激活网络，设置APN后激活网络，只能激活一次 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192808302-cd4a7166-00cc-4ac2-a5a7-2b65bec57854.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_11%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="305.6" title="" crop="0,0,1,1" id="ude4e7c0c" class="ne-image"> | |
| 8 | 查询IP，只有激活网络后，查询到IP后才能进行下一步。<br/>这个IP是基站分配的地址，大部分是内网地址，不用用来直接通讯。 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730192827192-d9d7dfa7-d5ec-40a5-9d3f-7e641764cf59.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_11%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="307.2" title="" crop="0,0,1,1" id="uab5ec993" class="ne-image"> | |
| 9 | 设置MQTT参数连接，支持IP和域名。设置客户端ID，用户名和密码<br/>浏览器打开[http://test.yinerda.com](http://test.yinerda.com)，选择“MQTT测试工具”，点击"打开"可以获取测试服务器信息 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730267508396-36684e1c-b54c-43f7-aef3-cbf0087404ab.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_46%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="1304" title="" crop="0,0,1,1" id="uad9dff07" class="ne-image"> | |
| 10 | 连接MQTT服务器,设置服务器地址和端口，CONNECT OK表示连接成功。 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730267594470-7adfd4a4-c54f-48ac-a96e-cb7f46f164bc.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_46%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="1283.2" title="" crop="0,0,1,1" id="u0cca2893" class="ne-image"> | |
| 11 | 发起会话请求，注意在AT+MIPSTART成功后，收到 CONNECT OK  后，马上发送这个命令<br/>心跳建议60~300秒，这里设置120秒 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730267863793-5c46e112-ecc3-4e10-ade3-56f6b0aa9e99.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_24%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="669.6" title="" crop="0,0,1,1" id="u9b9325d5" class="ne-image"> | |
| 12 | 订阅topic,把提前需要的topic都订阅上，只有订阅后才能收到相关的数据 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730267885463-9096ab17-1321-4d36-9dba-925d394d4c85.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_15%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="417.6" title="" crop="0,0,1,1" id="u8e851cbc" class="ne-image"> | |
| 13<br/> | 服务器发数据给DTU，数据可以是字符串，可以是HEX数据 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730268178949-402b456b-c4d4-4312-ac8d-019ce0a5b6ad.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_47%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="1321.6" title="" crop="0,0,1,1" id="ub2119571" class="ne-image"><img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730268320670-5ea1eaed-0037-42b5-bc4d-ec8657fc7094.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_47%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="1306.4" title="" crop="0,0,1,1" id="u9bd44346" class="ne-image"> | |
| 14 | DTU使用AT+MPUB上传普通字符串数据 | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730268040280-962667b5-400b-44ae-b490-0ae3dead598f.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_28%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="774.4" title="" crop="0,0,1,1" id="u4e0afbd6" class="ne-image"> | |
| 15 | DTU使用AT+MPUB传JSON数据，注意数据中的"号，要用\22替代。<br/>在实际值使用中\本身就要转义，所以MCU实际格式是\\22，如:<br/>AT+MPUB=pubtopic1,0,0,"{\\22test\\22:\\22abc\\22}"\r | |
| | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730268448353-2c225b7c-e0c5-42c9-8c4a-e6f21a1670e4.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_28%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="778.4" title="" crop="0,0,1,1" id="u5eadc792" class="ne-image"> | |
| 16 | DTU 使用AT+MPUBEX发数据，这个与AT+MPUB的区别是要提前指定长度。<br/>可以发HEX，JSON不用转义。前提是要计算长度，如何使用方便如何来即可<br/>AT+MPUBEX命令后，模块返回> 然后发送指定的长度内容即可 | |
|  | <img src="https://cdn.nlark.com/yuque/0/2024/png/42958080/1730268608105-b3867a01-a8de-4187-9fe7-b921d777a678.png?x-oss-process=image%2Fwatermark%2Ctype_d3F5LW1pY3JvaGVp%2Csize_35%2Ctext_eWluZXJkYQ%3D%3D%2Ccolor_FFFFFF%2Cshadow_50%2Ct_80%2Cg_se%2Cx_10%2Cy_10" width="974.4" title="" crop="0,0,1,1" id="u7156d63f" class="ne-image"> | |


# 七、使用建议
MQTT在实际使用的时候，一般通过客户端ID或者订阅和发布topic的名字去区分设备。

4G模块里面有个15位的字符串：IMEI，是4G 模块的唯一标记，理论全球唯一(只要是正规模组，理论都要唯一)，可以用来区分4G模块。
