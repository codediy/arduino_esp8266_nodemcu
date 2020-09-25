#include <TM1637TinyDisplay.h>


TM1637TinyDisplay display_a(D1,D2);  // 4 digit led

void setup() {
  // put your setup code here, to run once:
  display_a.setBrightness(0x0f);
}

void loop() {
  // put your main code here, to run repeatedly:
  display_a.showNumber(1234); 
  delay(2000);
  display_a.showString("5678");
  delay(3000);
}
