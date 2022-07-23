#include "sd_block.h"

inline void SD_block::csLow(void) {
  SPI.beginTransaction(_spiSettings);
  digitalWrite(_cs, LOW);
}

inline void SD_block::csHigh(void) {
  digitalWrite(_cs, HIGH);
  SPI.endTransaction();
}

uint8_t SD_block::waitAvailable(uint16_t timeout) {
  uint16_t t0 = millis();
  uint16_t d = millis() - t0;
  while (d < timeout) {
    if (SPI.transfer(0xFF) == 0xFF) return 1;
    d = millis() - t0;
  } return 0;
}

uint8_t SD_block::sendCMD(uint8_t cmd, uint32_t arg) {
  uint8_t crc {0xFF};
  if (cmd == CMD0) crc = 0x95;
  if (cmd == CMD8) crc = 0x87;
  csHigh();
  asm volatile("nop\n\r");
  csLow();
  if (!waitAvailable(300)) return -1;
  SPI.transfer(cmd | 0x40);
  for (int8_t offset = 24; offset >= 0; offset -= 8) SPI.transfer(arg >> offset);
  SPI.transfer(crc);
  for (uint8_t i {0}; (_status = SPI.transfer(0xFF) & 0x80) && (i != 0xFF); i++);
  return _status;
}

uint8_t SD_block::sendACMD(uint8_t cmd, uint32_t arg) {
  sendCMD(CMD55, 0);
  return sendCMD(cmd, arg);
}

int8_t SD_block::init(uint8_t cs) {
  _cs = cs;
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, HIGH);
  uint32_t arg;
  uint16_t timer = millis();
  SPI.begin();
  _spiSettings = SPISettings(250000, MSBFIRST, SPI_MODE0);
  SPI.beginTransaction(_spiSettings);
  for (uint8_t i = 0; i < 10; i++) SPI.transfer(0xFF);
  SPI.endTransaction();
  csLow();
  while ((_status = sendCMD(CMD0, 0)) != R1_IDLE) {
    uint16_t d = millis() - timer;
    if (d > SD_TIMEOUT_INIT) {
      _errorCode = -1;
      goto fail;
    }
  } if ((sendCMD(CMD8, 0x1AA) & R1_ILLCMD)) _type = SD_TYPE_V1;
  else {
    for (uint8_t i = 0; i < 4; i++) _status = SPI.transfer(0xFF);
    if (_status != 0XAA) {
      _errorCode = -8;
      goto fail;
    } _type = SD_TYPE_V2;
  } arg = _type == SD_TYPE_V2 ? 0X40000000 : 0;
  while ((_status = sendACMD(ACMD41, arg)) != R1_READY) {
    uint16_t d = millis() - timer;
    if (d > SD_TIMEOUT_INIT) {
      _errorCode = -41;
      goto fail;
    }
  } if (_type == SD_TYPE_V2) {
    if (sendCMD(CMD58, 0)) {
      _errorCode = -58;
      goto fail;
    } if ((SPI.transfer(0xFF) & 0XC0) == 0XC0) _type = SD_TYPE_HC;
    for (uint8_t i = 0; i < 3; i++) SPI.transfer(0xFF);
  } csHigh();
  _errorCode = 1;
  return 1;
fail:
  csHigh();
  return _errorCode;
}

int8_t SD_block::writeBlock(uint32_t addr, uint8_t *buff) {
  if (_type != SD_TYPE_HC) addr <<= 9;
  if (sendCMD(CMD24, addr)) {
    _errorCode = -24;
    goto fail;
  } if (!waitAvailable(SD_TIMEOUT_WRITE)) {
    _errorCode = 7;
    goto fail;
  } !waitAvailable(SD_TIMEOUT_WRITE);
  SPI.transfer(TOKEN_WIRTEMULT);
  for (uint16_t i = 0; i < 512; i++) SPI.transfer(buff[i]);
  SPI.transfer(0xFF);
  SPI.transfer(0xFF);
  _status = SPI.transfer(0xFF);
  if ((_status & DATA_RESMASK) != DATA_ACCEPTED) {
    _errorCode = -3;
     goto fail;
  }
  csHigh();
  return 1;
fail:
  csHigh();
  return _errorCode;
}

uint8_t SD_block::waitStartBlock(void) {
  unsigned int t0 = millis();
  while ((_status = SPI.transfer(0xFF)) == 0XFF) {
    unsigned int d = millis() - t0;
    if (d > SD_TIMEOUT_READ) {
      _errorCode = 16;
      goto fail;
    }
  } if (_status != DATA_STARTBLOCK) {
    _errorCode = 17;
    goto fail;
  } return true;
fail:
  csHigh();
  return false;
}

int8_t SD_block::readBlock(uint32_t addr, uint8_t *buff) {
  if (_type != SD_TYPE_HC) addr <<= 9;
  if (sendCMD(CMD17, addr)) {
    _errorCode = -17;
    goto fail;
  } if (!waitStartBlock()) goto fail;
  for (uint16_t i = 0; i < 512; i++) buff[i] = SPI.transfer(0xFF);
  csHigh();
  return 1;
fail:
  csHigh();
  return _errorCode;
}

uint8_t SD_block::writeData(uint8_t *buff, uint16_t size) {
  uint8_t result = 1;
  if (_currentByte + size >= 512) {
    _addr++;
    uint8_t retry = 0;
    while (!writeBlock(_addr, _virtualSector) && (retry++ < 255));
    if (retry >= 255) return 0;
    result = 2;
  } for (uint8_t i = _currentByte; i < _currentByte + size; i++) _virtualSector[i] = buff[i - _currentByte];
  _currentByte += size;
  return result;
}