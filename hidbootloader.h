#ifndef HIDBOOTLOADER_H
#define HIDBOOTLOADER_H

#include "bootloaderusblink.h"



class HidBootloader
{
public:
    HidBootloader(BootLoaderUSBLink &hidLink);
    int getHardwareVersion(void);
    bool setHexFile(QString fileName);
    bool eraseDevice(void);
    bool programDevice(void);
    uint16_t getCRC(void);
    void jumpToApp(void);
private:
    enum {READ_BOOT_INFO = 1, ERASE_FLASH, PROGRAM_FLASH, READ_CRC, JMP_TO_APP};
    enum {SOH = 0x01, EOT = 0x04, DLE = 0x10};
    BootLoaderUSBLink &link;
    uint16_t calculateCRC(uint8_t *data, uint32_t len);
    int processOutput(void);
    int processInput(void);
    uint8_t transferBuffer[512];
    uint8_t processedBuffer[512];
    int bufferLen;
    int processedLen;
};

#endif // HIDBOOTLOADER_H
