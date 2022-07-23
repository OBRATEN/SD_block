#ifndef SD_BLOCK_H
#define SD_BLOCK_H

/*
  Arduino library for block write/read data without file system;
  Based on default SD library for Arduino;
  Author: Arthur Garagulya, SPORADIC, Kursk;
*/

#include <Arduino.h>
#include <SPI.h>

#define CMD0   0x00
#define CMD8   0x08
#define CMD9   0x09
#define CMD10  0x0A
#define CMD13  0x0D
#define CMD17  0x11
#define CMD24  0x18
#define CMD25  0x19
#define CMD32  0x20
#define CMD33  0x21
#define CMD38  0x26
#define CMD55  0x37
#define CMD58  0x3A
#define ACMD23 0x17
#define ACMD41 0x29

#define R1_READY         0x00
#define R1_IDLE          0x01
#define R1_ILLCMD        0x04
#define DATA_STARTBLOCK  0xFE
#define DATA_RESMASK     0x1F
#define DATA_ACCEPTED    0x05
#define TOKEN_STOPTRAIN  0xFD
#define TOKEN_WIRTEMULT  0xFC

#define SPI_FULLSPEED 0
#define SPI_HALFSPEED 1
#define SPI_QARTSPEED 2

#define SD_TIMEOUT_INIT  100
#define SD_TIMEOUT_ERASE 300
#define SD_TIMEOUT_READ  300
#define SD_TIMEOUT_WRITE 200

typedef enum {
  SD_TYPE_V1 = 1,
  SD_TYPE_V2 = 2,
  SD_TYPE_HC = 3,
} SDtype_e;

class SD_block {
public: // init
  SD_block(void) {}
  int8_t init(uint8_t cs);
  uint8_t begin(uint8_t cs);
public: // status
  int8_t getErrorCode(void) { return _errorCode; }
  uint8_t getStatus(void) { return _status; }
  uint8_t available(void);
public: // functional
  uint8_t waitAvailable(uint16_t timeout);
  int8_t readBlock(uint32_t addr, uint8_t *buff);
  int8_t writeBlock(uint32_t addr, uint8_t *buff);
  uint8_t writeData(uint8_t *buff, uint16_t size);
private: // status
  int8_t _errorCode;
  uint8_t _status;
private: // hardware
  uint8_t _cs;
  SPISettings _spiSettings;
  SDtype_e _type;
private: // hardware funcs
  inline void csLow(void);
  inline void csHigh(void);
  uint8_t waitStartBlock(void);
  uint8_t sendCMD(uint8_t cmd, uint32_t arg);
  uint8_t sendACMD(uint8_t cmd, uint32_t arg);
private:
  uint8_t _virtualSector[512];
  uint16_t _currentByte = 0;
  uint32_t _addr = 0;
};

#endif // SD_BLOCK_H