#include <Sd2Card.h>

#define SDSS_PIN       6
#define DEFAULT_SD_ID  99
#define SD_INFO_BLOCK0 3
#define SD_INFO_BLOCK1 3
#define SD_INFO_BLOCK2 3
#define SD_START_BLOCK 10

uint8_t sdData[512];
uint32_t totalBlocks;

Sd2Card sd;

typedef struct {
  uint8_t prefix = 0xFF;
  uint8_t id = 0;
  uint32_t lastBlock = 0;
  uint32_t globalTime = 0;
} sdInfo_t;

static sdInfo_t sdInfo;

uint8_t initCard(void) {
  if (!sd.init(SPI_FULL_SPEED, SDSS_PIN)) return 0;
  totalBlocks = sd.cardSize() / 512;
  uint8_t buff[512];
  sd.readBlock(SD_INFO_BLOCK0, buff);
  memcpy(&sdInfo, buff, sizeof(sdInfo));
  if (sdInfo.id != DEFAULT_SD_ID) {
    sd.erase(SD_INFO_BLOCK0, SD_INFO_BLOCK2);
    sdInfo.id = DEFAULT_SD_ID;
    sdInfo.globalTime = millis();
    sdInfo.lastBlock = 0;
    uint8_t *info = reinterpret_cast<uint8_t*>(&sdInfo);
    sd.writeBlock(SD_INFO_BLOCK0, info);
    sd.writeBlock(SD_INFO_BLOCK1, info);
    sd.writeBlock(SD_INFO_BLOCK2, info);
    return 2;
  } return 1;
}

uint8_t writeSD(uint8_t *buff, uint16_t len, uint8_t updateInfo) {
  if ((sdInfo.lastBlock + 1) > totalBlocks) sdInfo.lastBlock = SD_START_BLOCK;
  uint8_t retry = 0;
  while (!sd.writeBlock(sdInfo.lastBlock++, buff)) if (retry++ == 80) initCard();
  if (updateInfo && (retry < 80)) {
    uint8_t *info = reinterpret_cast<uint8_t*>(&sdInfo);
    sd.writeBlock(SD_INFO_BLOCK0, info);
    sd.writeBlock(SD_INFO_BLOCK1, info);
    sd.writeBlock(SD_INFO_BLOCK2, info);
  } return (retry < 80);
}

void setup() {
  pinMode(SDSS_PIN, OUTPUT);
  memset(sdData, 5, 512);
  initCard();
}

void loop() {
  writeSD(sdData, 512, 1);
}
