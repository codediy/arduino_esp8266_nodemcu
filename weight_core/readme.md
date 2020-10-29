1. 秤的设备状态
    - scale_status //0 初始化中，1 误差计算中 2 误差计算成功 3 单重计算中 4 正常称重中
2. ws通信消息格式
    - {type:"xx",mac:"",ip:"",value:""}
    - type:ping             心跳消息       
    - type:report_device    设备信息上报
    - type:report_factor    误差信息上报
    - type:report_weight    单重信息上报
    - type:report_change    重量变化上报
3. http消息
    - {type:"xx",value:"xx"}
    - type:set_location     货架位置信息
    - type:set_factor       进行误差计算
    - type:set_weight       进行单重计算
    - type:set_change       等待重量变化

```
              report_device                               
        ------------------------------->
              set_xx
                根据mac绑定的货架信息进行处理
                    0. 如果mac没有绑定货架信息，则发送货架位置信息set_location
                    1. 如果mac对应的货架没有误差信息，则进入误差计算中(scale_status = 1)     
                        set_factor  value=砝码重量
                    3. 如果mac对应的货架已有误差么有单重，则进入单重计算中(scale_status = 3) 
                        set_weight  value=物品数量
                    4. 如果mac对应的货架已有误差和单重，则进入正常称重中(scale_status = 4)   
                        set_change  value=重量变化前的值
        <-------------------------------
秤http                                                     平板App

           1. report_factor (value=误差值)
        --------------------------------->          

           3. report_weight (value=单重信息)
        --------------------------------->    
           4. report_change (value=重量变化信息)
        ---------------------------------> 
```


4. 集成称重 
    GND    黑 黑 
    DT     蓝 绿     D5  
    SCK    白 白     D6
    VCC    红 红


    E+    红  红
    E-    黑  灰
    A-    白  白
    A+    绿  蓝

5 路由器信息
wqvmi
wq12356789;

路由器后台密码 341201

6 配货员
17472147214
123456

7 取货员
14312341234
123456

9 无秤货柜的配货员与取货员
配货员 18936983698 123456
取货员 18525872587 123456


10 苏州仓库
suzhoucang

11 38号设备
配货员 17414581458 123456
取货员 17414781478 123456


12 

                        货架个数变化六位灯
                            CLK D1
                            DIO D2
                            vcc 3v
                            GND
                        货架编码四位灯
                            CLK D3
                            DIO D4
                            vcc 3v
                            GND
                        秤
                          绿/蓝   DOT D5
                             白   CLK D6
                             红   vcc 5v->vin
                             黑   GND GND
                        货架物品个数六位灯
                            CLK D7
                            DIO D8
                            vcc 3v
                            GND

  