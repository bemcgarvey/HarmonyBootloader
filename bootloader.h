#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QObject>

class Bootloader : public QObject
{
    Q_OBJECT
public:
    Bootloader();
    //TODO some of these can have a default behavior and not be pure
    //TODO add const to functions where appropriate
    virtual ~Bootloader() = 0;
    virtual bool isConnected() = 0;
    virtual int readBootInfo();
    virtual bool setFile(QString fileName) = 0;
    virtual bool eraseFlash();
    virtual bool programFlash() = 0;
    virtual void jumpToApp() = 0;
    virtual bool verify() = 0;
    virtual void abort();
    bool isAborted() {return m_abort;}
protected:
    bool m_abort;
signals:
    void finished(bool success);
    void progress(int p);
    void message(QString m);
};

#endif // BOOTLOADER_H
