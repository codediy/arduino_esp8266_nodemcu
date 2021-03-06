#include "HX711.h"

#define DOUT  D5
#define CLK  D6

#define INIT_ING 0
#define FACTOR_ING 1
#define FACTOR_SUCCESS 2
#define WEIGHT_ING 3
#define WEIGHT_SUCCESS 4
#define CHANGE_ING 5
#define CHANGE_SEND_ING 6
#define STOP_ING 7

HX711 scale;

//默认误差值
float real_weight = 5000;   //误差计算是真实重量
float calibration_factor = 110; //默认误差值
int scale_status = FACTOR_ING;
/**
 * 动态计算误差值
 */
void factor_handle(){
  Serial.println("factor_handle: ");
  Serial.println(real_weight);
  scale.set_scale(calibration_factor); 
  float weight_result = (float)scale.get_units();
  
  while(real_weight > 0 && weight_result > 100 && scale_status == FACTOR_ING){
      scale.set_scale(calibration_factor); 
      weight_result = (float)scale.get_units();
      
      Serial.print("Reading: ");
      Serial.println(weight_result);
      
      Serial.print("Real: ");
      Serial.println(real_weight);
    
      Serial.println(calibration_factor);
    
      float cha = real_weight - weight_result;
      
      if(cha > 100){
         calibration_factor = calibration_factor - 1;
      }if(cha > 20){
         calibration_factor = calibration_factor - 0.1;
      }else if(cha > 1){
         calibration_factor = calibration_factor - 0.01;
      }else if(cha < -100){
         calibration_factor = calibration_factor + 1;
      }else if(cha < -20){
         calibration_factor = calibration_factor + 0.1;
      }else if(cha < 0){
         calibration_factor = calibration_factor + 0.01;
      }else {
         Serial.println("factor success");
         scale_status = FACTOR_SUCCESS;
      }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");
  
  scale.begin(DOUT, CLK);
  scale.set_scale();
  scale.tare(); //Reset the scale to 0
  
  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);

}

void loop() {
  float weight_result = (int)scale.get_units();
  Serial.println("weight_result: ");
  Serial.println(weight_result);

  if(scale_status == FACTOR_ING){
     factor_handle();
  }
  delay(1000);
}
