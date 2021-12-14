#include "hidbootloader.h"

HidBootloader::HidBootloader(uint16_t vid, uint16_t pid):
    Bootloader(), m_link(std::unique_ptr<BootLoaderUSBLink>(new BootLoaderUSBLink())),
    m_sectionCRC(0), m_sectionStartAddress(0), m_currentAddress(0),
    m_linAddress(0), m_segAddress(0)
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
    m_transferBuffer[0] = READ_BOOT_INFO;
    m_bufferLen = 1;
    processOutput();
    m_link->WriteDevice(m_processedBuffer, 1);
    m_link->ReadDevice(m_transferBuffer);
    m_bufferLen = 64;
    m_bufferLen = processInput();
    if (m_bufferLen != 3) {
        version = 0;
    } else {
        version = (m_processedBuffer[1] << 8) + m_processedBuffer[2];
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
    m_transferBuffer[0] = ERASE_FLASH;
    m_bufferLen = 1;
    processOutput();
    m_link->WriteDevice(m_processedBuffer, 1);
    m_link->ReadDevice(m_transferBuffer, 30000);
    m_bufferLen = 64;
    m_bufferLen = processInput();
    if (m_bufferLen != 1 || m_processedBuffer[0] != ERASE_FLASH) {
        emit finished(false);
        return false;
    } else {
        emit message("Device Erased");
        emit progress(50);
        return true;
    }
}

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
    emit message("Programming flash");
    m_regionList.clear();
    while (m_hexFile->readLine(lineBuffer, 512) > 0) {
        if (m_abort) {
            emit finished(false);
            return false;
        }
        if (parseHexRecord(lineBuffer)) {
            int len = processOutput();
            m_link->WriteDevice(m_processedBuffer, len);
            m_link->ReadDevice(m_transferBuffer, 200);
            m_bufferLen = 64;
            m_bufferLen = processInput();
            if (m_bufferLen != 1 || m_processedBuffer[0] != PROGRAM_FLASH) {
                emit finished(false);
                return false;
            }
        }
        ++currentLine;
        emit progress((currentLine * 100) / lineCount);
    }
    emit progress(100);
    emit finished(true);
    return true;
}

uint16_t HidBootloader::readCRC(uint32_t address, uint32_t len)
{
    m_transferBuffer[0] = READ_CRC;
    *(uint32_t *)&m_transferBuffer[1] = address;
    *(uint32_t *)&m_transferBuffer[5] = len;
    m_bufferLen = 9;
    int outLen = processOutput();
    m_link->WriteDevice(m_processedBuffer, outLen);
    m_link->ReadDevice(m_transferBuffer, 500);
    m_bufferLen = 64;
    m_bufferLen = processInput();
    if (m_bufferLen != 3 || m_processedBuffer[0] != READ_CRC) {
        return -1;
    }
    return *(uint16_t *)&m_processedBuffer[1];
}

void HidBootloader::jumpToApp()
{
    m_transferBuffer[0] = JMP_TO_APP;
    m_bufferLen = 1;
    int len = processOutput();
    m_link->WriteDevice(m_processedBuffer, len);
    m_link->ReadDevice(m_transferBuffer);
}

int HidBootloader::processOutput()
{
    int outPos = 0;
    //append crc to buffer
    uint16_t crc = calculateCRC(m_transferBuffer, m_bufferLen);
    m_transferBuffer[m_bufferLen++] = crc & 0xff;
    m_transferBuffer[m_bufferLen++] = (crc >> 8) & 0xff;
    //add header and escape special values
    m_processedBuffer[outPos++] = SOH;
    for (int i = 0; i < m_bufferLen; ++i) {
        if (m_transferBuffer[i] == SOH || m_transferBuffer[i] == EOT || m_transferBuffer[i] == DLE) {
            m_processedBuffer[outPos++] = DLE;
        }
        m_processedBuffer[outPos++] = m_transferBuffer[i];
    }
    m_processedBuffer[outPos++] = EOT;
    return outPos;
}

int HidBootloader::processInput()
{
    int outPos = 0;
    if (m_transferBuffer[0] != SOH) {
        return 0;
    }
    int i = 1;
    while (i < m_bufferLen) {
        if (m_transferBuffer[i] == EOT) {
            break;
        }
        if (m_transferBuffer[i] == DLE) {
            ++i;
        }
        m_processedBuffer[outPos++] = m_transferBuffer[i++];
    }
    if (m_transferBuffer[i] != EOT) {
        return 0;
    }
    uint16_t calculatedCrc = calculateCRC(m_processedBuffer, outPos - 2);
    uint16_t receivedCrc = m_processedBuffer[outPos - 2] + (m_processedBuffer[outPos - 1] << 8);
    if (calculatedCrc != receivedCrc) {
        return 0;
    } else {
        return outPos - 2;
    }
}

bool HidBootloader::parseHexRecord(char *hexRec)
{
    m_bufferLen = 0;
    m_transferBuffer[m_bufferLen++] = PROGRAM_FLASH;
    if (*hexRec != ':') {
        return false;
    }
    ++hexRec;
    while (*hexRec) {
        if (*hexRec == '\n') {
            break;
        }
        m_transferBuffer[m_bufferLen] = hexCharToInt(*hexRec++) << 4;
        m_transferBuffer[m_bufferLen++] += hexCharToInt(*hexRec++);
    }
    uint8_t recType = m_transferBuffer[4];
    switch (recType) {
    case HEX_LIN_ADDRESS:
        m_linAddress = m_transferBuffer[5];
        m_linAddress <<= 8;
        m_linAddress += m_transferBuffer[6];
        m_linAddress <<= 16;
        break;
    case HEX_SEG_ADDRESS:
        m_segAddress = m_transferBuffer[5];
        m_segAddress <<= 8;
        m_segAddress += m_transferBuffer[6];
        m_linAddress <<= 4;
        break;
    case HEX_DATA: {
        uint32_t recordAddress = m_transferBuffer[2];
        recordAddress <<= 8;
        recordAddress += m_transferBuffer[3];
        recordAddress += m_segAddress + m_linAddress;
        if (recordAddress != m_currentAddress) {
            //save last region
            FlashRegion lastRegion
                    = {m_sectionStartAddress, m_currentAddress - m_sectionStartAddress, m_sectionCRC};
            m_regionList.append(lastRegion);
            m_sectionStartAddress = recordAddress;
            m_currentAddress = recordAddress;
            m_sectionCRC = 0;
        }
        int dataLen = m_transferBuffer[1];
        m_sectionCRC = calculateCRC(&m_transferBuffer[5], dataLen, m_sectionCRC);
        m_currentAddress += dataLen;
    }
        break;
    case HEX_EOF: {
        //save last region
        FlashRegion lastRegion
                = {m_sectionStartAddress, m_currentAddress - m_sectionStartAddress, m_sectionCRC};
        m_regionList.append(lastRegion);
    }
        break;
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

bool HidBootloader::verify()
{
    for (auto &i : m_regionList) {
        if (i.length > 0) {
            uint16_t crc = readCRC(i.startAddress, i.length);
            if (crc != i.crc) {
                emit message("Flash verify failed");
                return false;
            }
        }
    }
    emit message("Flash verified");
    return true;
}

uint16_t HidBootloader::calculateCRC(uint8_t *data, uint32_t len, uint16_t crc)
{
    static const uint16_t crc_table[16] =
    {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
    };
    uint32_t i;

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
