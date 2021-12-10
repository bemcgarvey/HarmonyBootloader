#include "hidbootloader.h"

HidBootloader::HidBootloader(uint16_t vid, uint16_t pid):
    Bootloader(), m_link(std::unique_ptr<BootLoaderUSBLink>(new BootLoaderUSBLink()))
{
    m_link->Open(pid, vid);
}

bool HidBootloader::isConnected()
{
    return m_link->Connected();
}

int HidBootloader::getHardwareVersion()
{
    uint16_t version = 0;
    transferBuffer[0] = READ_BOOT_INFO;
    bufferLen = 1;
    processOutput();
    m_link->WriteDevice(processedBuffer, 1);
    m_link->ReadDevice(transferBuffer);
    bufferLen = 64;
    bufferLen = processInput();
    if (bufferLen != 3) {
        return 0;
    } else {
        version = (processedBuffer[1] << 8) + processedBuffer[2];
    }
    return version;
}

bool HidBootloader::setFile(QString fileName)
{

}

bool HidBootloader::eraseDevice()
{

}

bool HidBootloader::programDevice()
{

}

uint16_t HidBootloader::getCRC()
{

}

void HidBootloader::jumpToApp()
{

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
