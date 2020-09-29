#include <HX711.h>
#include <TM1637_6D.h>

// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = D5;
const int LOADCELL_SCK_PIN = D6;

// HX711 init
HX711 scale;

int weight = 0;
int lastweight = 0;
TM1637_6D display_b(D0,D1); // 6 digit led

void setup() {
  Serial.begin(115200);
  
  display_b.init();
  display_b.set(BRIGHTEST);
   
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(120);
  scale.tare();
}

void loop() {
  if (scale.is_ready()) {
    long f = scale.get_units();
    weight = int(f);
    if (weight < 0){
      weight = 0;
    }
    //display_b.displayFloat(weight);
    
    if (weight > 0){
      if (lastweight!=weight){
         lastweight = weight;
         Serial.print("last-weight:");
         Serial.println(lastweight);
         Serial.print("weight:");
         Serial.println(weight);
         display_b.displayFloat(weight);
         delay(3000);
      }
    }
    
  } else {
    Serial.println("HX711 not found.");
  }

  //display_a.showNumber(1234); 
  delay(1000);
}
