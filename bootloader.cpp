#include "bootloader.h"

Bootloader::Bootloader() : m_abort(false), m_isIdle(true)
{

}

Bootloader::~Bootloader() {

}

void Bootloader::abort()
{
    m_abort = true;
}

bool Bootloader::isIdle()
{
    return m_isIdle;
}

uint16_t Bootloader::calculateCRC(uint8_t *data, uint32_t len)
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
