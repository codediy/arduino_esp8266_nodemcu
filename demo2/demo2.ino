#include <TM1637_6D.h>
TM1637_6D display_b(D0,D1); // 6 digit led
TM1637_6D display_c(D2,D3);// 6 digit led

void setup() {
  // put your setup code here, to run once:
  display_b.init();
  display_c.init();
  display_b.set(BRIGHTEST);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  display_c.set(BRIGHTEST);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
}

void loop() {
  // put your main code here, to run repeatedly:
  display_b.displayFloat(1234);
  display_c.displayFloat(1234.56);
  delay(3000);
}
