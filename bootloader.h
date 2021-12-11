#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QObject>

class Bootloader : public QObject
{
    Q_OBJECT
public:
    Bootloader();
    virtual ~Bootloader() = 0;
    virtual bool isConnected() = 0;
    virtual int readBootInfo() = 0;
    virtual bool setFile(QString fileName) = 0;
    virtual bool eraseFlash() = 0;
    virtual bool programFlash() = 0;
    virtual uint16_t readCRC() = 0;
    virtual void jumpToApp() = 0;
    virtual void abort();
    bool isAborted() {return m_abort;}
protected:
    virtual uint16_t calculateCRC(uint8_t *data, uint32_t len);
    bool m_abort;
signals:
    void finished();
    void progress(int p);
    void message(QString m);
};

#endif // BOOTLOADER_H
