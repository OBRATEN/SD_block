#include "sd_block.h"

#define SDSS_PIN 8

SD_block sd;
uint32_t addr = 0;
uint8_t buff[512];

void setup(void) {
  Serial.begin(115200);
  sd.init(SDSS_PIN);
  memset(buff, 3);
}

void loop(void) {
  Serial.println(sd.writeBlock(addr++, buff));
}