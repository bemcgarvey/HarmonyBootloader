#ifndef HIDBOOTLOADER_H
#define HIDBOOTLOADER_H

#include "bootloaderusblink.h"
#include "bootloader.h"
#include <memory>



class HidBootloader : public Bootloader
{
public:
    HidBootloader(uint16_t vid, uint16_t pid);
    ~HidBootloader() {};
    virtual bool isConnected() override;
    virtual bool setFile(QString fileName) override;
    virtual int getHardwareVersion() override;
    virtual bool eraseDevice() override;
    virtual bool programDevice() override;
    virtual uint16_t getCRC() override;
    virtual void jumpToApp() override;
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
};

#endif // HIDBOOTLOADER_H
