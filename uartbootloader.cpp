#include "uartbootloader.h"
#include "hexfile.h"

UARTBootloader::UARTBootloader(QString portName, int baud, uint32_t startAddress, uint16_t eraseBlockSize) :
    Bootloader(), m_portName(portName), m_baud(baud)
  , m_connected(false), m_flashStart(startAddress), m_eraseBlockSize(eraseBlockSize)
{
    m_txHeader.guard = BTL_GUARD;
    if (m_portName != "") {
        m_connected = true;
    }
    m_flashCRC = 0xffffffff;
}


bool UARTBootloader::isConnected()
{
    return m_connected;
}

bool UARTBootloader::setFile(QString fileName)
{
    if (fileName.endsWith(".hex", Qt::CaseInsensitive)) {
        m_binFile.reset(HexFile::hexToBinFile(fileName));
        if (m_binFile) {
            return m_binFile->open(QIODevice::ReadOnly);
        } else {
            return false;
        }
    } else if (fileName.endsWith(".bin", Qt::CaseInsensitive)) {
        m_binFile.reset(new QFile());
        m_binFile->setFileName(fileName);
        return m_binFile->open(QIODevice::ReadOnly);
    }
    return false;
}

bool UARTBootloader::programFlash()
{
    uint32_t flashLen = 0;
    int blocks = 0;
    int currentBlock = 0;
    uint32_t data[m_eraseBlockSize / 4 + 1];  //extra word for address
    int bytesRead;

    emit message("Programming flash");
    m_flashCRC = generateCRC(flashLen);
    blocks = flashLen / m_eraseBlockSize;
    m_port.reset(new QSerialPort(nullptr));
    m_port->setPortName(m_portName);
    m_port->setBaudRate(m_baud);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_connected = m_port->open(QIODevice::ReadWrite);
    if (!m_connected) {
        emit finished(false);
        return false;
    }
    char result;
    m_txHeader.size = 8;
    m_txHeader.command = BL_CMD_UNLOCK;
    m_port->write(m_txHeader.bytes, 9);
    m_port->flush();
    uint32_t unlock[2] = {m_flashStart, flashLen};
    m_port->write((char *)&unlock[0], 8);
    m_port->flush();
    m_port->waitForReadyRead(1000);
    if (m_port->bytesAvailable() < 1) {
        emit finished(false);
        return false;
    }
    m_port->read(&result, 1);
    if (result != BL_RESP_OK) {
        emit finished(false);
        return false;
    }
    m_binFile->reset();
    uint32_t currentAddress = m_flashStart;
    while (currentBlock < blocks) {
        if (m_abort) {
            emit finished(false);
            return false;
        }
        data[0] = currentAddress;
        bytesRead = m_binFile->read((char *)&data[1], m_eraseBlockSize);
        if (bytesRead < m_eraseBlockSize) {
            memset((char *)&data[1] + bytesRead, 0xff, m_eraseBlockSize - bytesRead);
        }
        m_txHeader.size = m_eraseBlockSize + 4;
        m_txHeader.command = BL_CMD_DATA;
        m_port->write(m_txHeader.bytes, 9);
        m_port->flush();
        m_port->write((char *)data, sizeof(data));
        m_port->flush();
        m_port->waitForReadyRead(1000);
        if (m_port->bytesAvailable() < 1) {
            emit finished(false);
            return false;
        }
        m_port->read(&result, 1);
        if (result != BL_RESP_OK) {
            emit finished(false);
            return false;
        }
        ++currentBlock;
        currentAddress += m_eraseBlockSize;
        emit progress(currentBlock * 100 / blocks);
    }
    m_binFile->close();
    return true;
}

void UARTBootloader::jumpToApp()
{
    m_txHeader.size = 1;
    m_txHeader.command = BL_CMD_RESET;
    m_port->write(m_txHeader.bytes, 9);
    m_port->flush();
    m_port->write(m_txHeader.bytes, 1);  //need a dummy byte
    m_port->flush();
    char result = 0;
    m_port->waitForReadyRead(1000);
    if (m_port->bytesAvailable() < 1) {
        emit finished(false);
        return;
    }
    m_port->read(&result, 1);
    m_port->close();
    emit finished(true);
}

bool UARTBootloader::verify()
{
    m_txHeader.size = 4;
    m_txHeader.command = BL_CMD_VERIFY;
    m_port->write(m_txHeader.bytes, 9);
    m_port->flush();
    m_port->write((char *)&m_flashCRC, 4);
    m_port->flush();
    char result = 0;
    m_port->waitForReadyRead(1000);
    if (m_port->bytesAvailable() < 1) {
        return false;
    }
    m_port->read(&result, 1);
    if (result != BL_RESP_CRC_OK) {
        emit message("Flash verify failed");
        return false;
    }
    emit message("Flash verified");
    return true;
}

uint32_t UARTBootloader::generateCRC(uint32_t &flashLen)
{
    uint32_t   value;
    uint32_t   crc_tab[256];
    uint32_t   crc = 0xffffffff;
    uint8_t    data[m_eraseBlockSize];
    int bytesRead;
    flashLen = 0;
    for (int i = 0; i < 256; i++)
    {
        value = i;
        for (int j = 0; j < 8; j++)
        {
            if (value & 1)
            {
                value = (value >> 1) ^ 0xEDB88320;
            }
            else
            {
                value >>= 1;
            }
        }
        crc_tab[i] = value;
    }
    m_binFile->reset();
    do {
        bytesRead = m_binFile->read((char *)data, m_eraseBlockSize);
        if (bytesRead > 0) {
            flashLen += m_eraseBlockSize;
        }
        for (int i = 0; i < m_eraseBlockSize; ++i) {
            if (i >= bytesRead) {
                data[i] = 0xff;
            }
            crc = crc_tab[(crc ^ data[i]) & 0xff] ^ (crc >> 8);
        }
    } while (bytesRead == m_eraseBlockSize);
    return crc;
}

