## Digital Pins

- 引脚可以配置为输入或者输出，

### Properties of Pins Configured as INPUT
- 引脚默认为输入的 
- 可以使用`pinMode(pin,INPUT)`指定模式
### Pullup Resistors with pins configured as INPUT
- 可以通过pullup或者pulldown电阻设置
### Properties of Pins Configured as INPUT_PULLUP
- 内置的20kpullup电阻，可以使用`pinMode(pin,INPUT_PULLUP)`使用
### Properties of Pins Configured as OUTPUT

## Analog Input Pins
- ATmega8,ATmega168,ATmega328P,ATmega1280

### A/D Conveter
- 可以包含6通道的AD转换 包含10bit的0~1023

### Pin mapping
```
pinMode(A0,OUTPUT);
digitalWrite(A0,HIGH);
```

### Pull-up resistors
```
pinMode(A0, INPUT_PULLUP);
```

## PWM
- Pulse Width Modulation 脉冲调制
- PWM来控制LED 
- `analogWrite()`是0到255的范围,
- `analogWrite(255)`读取完整一个周期
- `analogWrite(127)`读取半个周期


## Memory
- 三块不同的内存，`Flash`,`SRAM`,`EEPROM `
- `Flash` 控制器的控制程序
- `SRAM`  运行内存
- `EEPROM` 长期内存

- 需要注意内存溢出