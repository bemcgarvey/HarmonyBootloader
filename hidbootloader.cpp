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

int HidBootloader::readBootInfo()
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
    if (!fileName.endsWith(".hex", Qt::CaseInsensitive)) {
        return false;
    }
    m_hexFile = std::make_unique<QFile>(new QFile());
    m_hexFile->setFileName(fileName);
    if (m_hexFile->open(QIODevice::ReadOnly | QIODevice::Text)) {
        return true;
    } else {
        m_hexFile = nullptr;
        return false;
    }
}

bool HidBootloader::eraseFlash()
{
    emit message("Erasing device");
    transferBuffer[0] = ERASE_FLASH;
    bufferLen = 1;
    processOutput();
    m_link->WriteDevice(processedBuffer, 1);
    m_link->ReadDevice(transferBuffer, 30000);
    bufferLen = 64;
    bufferLen = processInput();
    if (bufferLen != 1 || processedBuffer[0] != ERASE_FLASH) {
        return false;
    } else {
        emit message("Device Erased");
        emit progress(50);
        return true;
    }
}

#include <QDebug>
bool HidBootloader::programFlash()
{
    if (!m_hexFile) {
        return false;
    }
    int lineCount = 0;
    char lineBuffer[512];
    m_hexFile->reset();
    while (m_hexFile->readLine(lineBuffer, 512) > 0) {
        ++lineCount;
    }
    if (lineCount == 0) {
        return false;
    }
    m_hexFile->reset();
    int currentLine = 0;
    while (m_hexFile->readLine(lineBuffer, 512) > 0) {
        if (parseHexRecord(lineBuffer)) {
            int len = processOutput();
            m_link->WriteDevice(processedBuffer, len);
            m_link->ReadDevice(transferBuffer, 200);
            bufferLen = 64;
            bufferLen = processInput();
            if (bufferLen != 1 || processedBuffer[0] != PROGRAM_FLASH) {
                return false;
            }
        }
        ++currentLine;
        emit progress((currentLine * 100) / lineCount);
    }
    emit progress(100);
    emit message("Programming complete");
    emit finished();
    return true;
}

uint16_t HidBootloader::readCRC()
{

}

void HidBootloader::jumpToApp()
{
    transferBuffer[0] = JMP_TO_APP;
    bufferLen = 1;
    processOutput();
    m_link->WriteDevice(processedBuffer, 1);
    m_link->ReadDevice(transferBuffer);

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

bool HidBootloader::parseHexRecord(char *hexRec)
{
    bufferLen = 0;
    transferBuffer[bufferLen++] = PROGRAM_FLASH;
    if (*hexRec != ':') {
        return false;
    }
    ++hexRec;
    while (*hexRec) {
        if (*hexRec == '\n') {
            break;
        }
        transferBuffer[bufferLen] = hexCharToInt(*hexRec++) << 4;
        transferBuffer[bufferLen++] += hexCharToInt(*hexRec++);
    }
    return true;
}

uint8_t HidBootloader::hexCharToInt(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    return 0;
}
