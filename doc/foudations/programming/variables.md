## Variables

```c++
int pin = 13
pinMode(pin,OUTPUT)
pinMode(13,OUTPUT)
```
```c++
int pin = 13;
void setup()
{
    pinMode(pin, OUTPUT);
}
void loop()
{
    digitalWrite(pin, HIGH);
}
```

```c++
void setup()
{
    int pin = 13;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);
}
```