#include "uartbootloader.h"

UARTBootloader::UARTBootloader(QString portName, int baud) :
    Bootloader(), m_portName(portName), m_baud(baud), m_connected(false)
{
    m_flashStart = 0x402000;
    m_eraseBlockSize = 8192;
    m_txHeader.guard = BTL_GUARD;
    if (m_portName != "") {
        m_connected = true;
    }
}


bool UARTBootloader::isConnected()
{
    return m_connected;
}

int UARTBootloader::readBootInfo()
{
    return 0;
}

bool UARTBootloader::setFile(QString fileName)
{
    if (!fileName.endsWith(".bin", Qt::CaseInsensitive)) {
        return false;
    }
    m_binFile = std::make_unique<QFile>(new QFile());
    m_binFile->setFileName(fileName);
    if (m_binFile->open(QIODevice::ReadOnly)) {
        return true;
    } else {
        m_binFile = nullptr;
        return false;
    }
}

bool UARTBootloader::eraseFlash()
{
    return true;
}

bool UARTBootloader::programFlash()
{
    emit message("Programming flash");
    uint32_t flashLen = 0;
    int blocks = 0;
    int currentBlock = 0;
    uint32_t data[m_eraseBlockSize / 4 + 1];  //extra word for address
    int bytesRead;
    do {
        bytesRead = m_binFile->read((char *)data, m_eraseBlockSize);
        if (bytesRead > 0) {
            flashLen += bytesRead;
            ++blocks;
        }

    } while (bytesRead == m_eraseBlockSize);
    m_binFile->reset();
    m_port = std::make_unique<QSerialPort>(new QSerialPort(nullptr));
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
    uint32_t currentAddress = m_flashStart;
    char result;
    while (currentBlock < blocks) {
        if (m_abort) {
            emit finished(false);
            return false;
        }
        m_txHeader.size = 8;
        m_txHeader.command = BL_CMD_UNLOCK;
        m_port->write(m_txHeader.bytes, 9);
        m_port->flush();
        uint32_t unlock[2] = {currentAddress, m_eraseBlockSize};
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
    return true;
}

uint16_t UARTBootloader::readCRC()
{
    return 0;
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
