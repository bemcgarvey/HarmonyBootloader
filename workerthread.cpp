#include "workerthread.h"

WorkerThread::WorkerThread(Bootloader *boot) : QThread(), bootloader(boot)
{

}


void WorkerThread::run()
{
    if (bootloader->eraseFlash()) {
        if (bootloader->programFlash()) {
            bootloader->jumpToApp();
        }
    }
}
