#include <sd_block.h>

#define SDSS_PIN 8

SD_block sd;

void setup(void) {
  Serial.begin(115200);
}

void loop(void) {
  if (sd.init(SDSS_PIN)) {
    while(1) {
      Serial.println("SD init successfull");
      delay(400);
    }
  } Serial.println("Error init SD!");
}