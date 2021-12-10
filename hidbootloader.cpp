#include "hidbootloader.h"

HidBootloader::HidBootloader(BootLoaderUSBLink &hidLink):
    link(hidLink)
{

}

int HidBootloader::getHardwareVersion()
{
    uint16_t version = 0;
    transferBuffer[0] = READ_BOOT_INFO;
    bufferLen = 1;
    processOutput();
    link.WriteDevice(processedBuffer, 1);
    link.ReadDevice(transferBuffer);
    bufferLen = 64;
    bufferLen = processInput();
    if (bufferLen != 3) {
        return 0;
    } else {
        version = (processedBuffer[1] << 8) + processedBuffer[2];
    }
    return version;
}

bool HidBootloader::setHexFile(QString fileName)
{

}

bool HidBootloader::eraseDevice()
{

}

uint16_t HidBootloader::calculateCRC(uint8_t *data, uint32_t len)
{
    static const uint16_t crc_table[16] =
    {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
    };
    uint32_t i;
    uint16_t crc = 0;

    while(len--)
    {
        i = (crc >> 12) ^ (*data >> 4);
        crc = crc_table[i & 0x0F] ^ (crc << 4);
        i = (crc >> 12) ^ (*data >> 0);
        crc = crc_table[i & 0x0F] ^ (crc << 4);
        data++;
    }

    return (crc & 0xFFFF);
}


int HidBootloader::processOutput()
{
    int outPos = 0;
    //append crc to buffer
    uint16_t crc = calculateCRC(transferBuffer, bufferLen);
    transferBuffer[bufferLen++] = crc & 0xff;
    transferBuffer[bufferLen++] = (crc >> 8) & 0xff;
    //add header and escape special values
    processedBuffer[outPos++] = SOH;
    for (int i = 0; i < bufferLen; ++i) {
        if (transferBuffer[i] == SOH || transferBuffer[i] == EOT || transferBuffer[i] == DLE) {
            processedBuffer[outPos++] = DLE;
        }
        processedBuffer[outPos++] = transferBuffer[i];
    }
    processedBuffer[outPos++] = EOT;
    return outPos;
}

int HidBootloader::processInput()
{
    int outPos = 0;
    if (transferBuffer[0] != SOH) {
        return 0;
    }
    int i = 1;
    while (i < bufferLen) {
        if (transferBuffer[i] == EOT) {
            break;
        }
        if (transferBuffer[i] == DLE) {
            ++i;
        }
        processedBuffer[outPos++] = transferBuffer[i++];
    }
    if (transferBuffer[i] != EOT) {
        return 0;
    }
    uint16_t calculatedCrc = calculateCRC(processedBuffer, outPos - 2);
    uint16_t receivedCrc = processedBuffer[outPos - 2] + (processedBuffer[outPos - 1] << 8);
    if (calculatedCrc != receivedCrc) {
        return 0;
    } else {
        return outPos - 2;
    }
}
