#include <TM1637_6D.h>
TM1637_6D display_b(D1,D2); // 6 digit led
TM1637_6D display_c(D7,D8);// 6 digit led

void setup() {
  // put your setup code here, to run once:
  display_b.init();
  display_c.init();
  display_b.set(BRIGHTEST);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;
  display_c.set(BRIGHTEST);//BRIGHT_TYPICAL = 2,BRIGHT_DARKEST = 0,BRIGHTEST = 7;

  display_b.displayFloat(0);
  display_c.displayFloat(0);
}

void loop() {
  // put your main code here, to run repeatedly:
  display_b.displayFloat(1234);
  display_c.displayFloat(1234.56);
  delay(3000);
}
