#ifndef UARTBOOTLOADER_H
#define UARTBOOTLOADER_H

#include "bootloader.h"
#include <QtSerialPort/QSerialPort>
#include <QFile>

typedef union {
    struct __attribute__ ((packed)){
        uint32_t guard;
        uint32_t size;
        uint8_t command;
    };
    char bytes[9];
} TxHeader;

class UARTBootloader : public Bootloader
{
public:
    UARTBootloader(QString portName, int baud);
    virtual bool isConnected() override;
    virtual int readBootInfo() override;
    virtual bool setFile(QString fileName) override;
    virtual bool eraseFlash() override;
    virtual bool programFlash() override;
    virtual uint16_t readCRC() override;
    virtual void jumpToApp() override;
    virtual bool verify() override;
private:
    enum {BL_CMD_UNLOCK= 0xa0, BL_CMD_DATA = 0xa1, BL_CMD_VERIFY = 0xa2, BL_CMD_RESET = 0xa3};
    enum {BL_RESP_OK = 0x50, BL_RESP_ERROR = 0x51, BL_RESP_INVALID = 0x52, BL_RESP_CRC_OK = 0x53,
          BL_RESP_CRC_FAIL = 0x54};
    const uint32_t BTL_GUARD = 0x5048434D;
    QString m_portName;
    int m_baud;
    bool m_connected;
    std::unique_ptr<QFile> m_binFile;
    uint32_t m_flashStart;
    uint16_t m_eraseBlockSize;
    TxHeader m_txHeader;
    std::unique_ptr<QSerialPort> m_port;
    uint32_t generateCRC(uint32_t &flashLen);
    uint32_t m_flashCRC;
};

#endif // UARTBOOTLOADER_H
