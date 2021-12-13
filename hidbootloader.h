#ifndef HIDBOOTLOADER_H
#define HIDBOOTLOADER_H

#include "bootloaderusblink.h"
#include "bootloader.h"
#include <memory>
#include <QFile>



class HidBootloader : public Bootloader
{
public:
    HidBootloader(uint16_t vid, uint16_t pid);
    ~HidBootloader() {};
    virtual bool isConnected() override;
    virtual bool setFile(QString fileName) override;
    virtual int readBootInfo() override;
    virtual bool eraseFlash() override;
    virtual bool programFlash() override;
    virtual void jumpToApp() override;
    virtual bool verify() override;
private:
    enum {READ_BOOT_INFO = 1, ERASE_FLASH, PROGRAM_FLASH, READ_CRC, JMP_TO_APP};
    enum {SOH = 0x01, EOT = 0x04, DLE = 0x10};
    int processOutput(void);
    int processInput(void);
    uint8_t transferBuffer[512];
    uint8_t processedBuffer[512];
    int bufferLen;
    int processedLen;
    std::unique_ptr<BootLoaderUSBLink> m_link;
    std::unique_ptr<QFile> m_hexFile;
    bool parseHexRecord(char *hexRec);
    uint8_t hexCharToInt(char c);
    uint16_t readCRC(uint32_t address, uint32_t len);
    uint16_t calculateCRC(uint8_t *data, uint32_t len, uint16_t crc = 0);
};

#endif // HIDBOOTLOADER_H
