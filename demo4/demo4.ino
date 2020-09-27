#include <HX711.h>
// HX711 circuit wiring
const int LOADCELL_DOUT_PIN = D5;
const int LOADCELL_SCK_PIN = D6;

// HX711 init
HX711 scale;

int weight = 0;
int lastweight = 0;

void setup() {
  Serial.begin(115200);
    
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(460);
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
         delay(3000);
      }
    }
    
  } else {
    Serial.println("HX711 not found.");
  }

  //display_a.showNumber(1234); 
  delay(1000);
}
