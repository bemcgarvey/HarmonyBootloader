#include "workerthread.h"

WorkerThread::WorkerThread(Bootloader *boot) : QThread(), bootloader(boot)
{

}


void WorkerThread::run()
{
    if (bootloader->eraseFlash()) {
        if (!bootloader->isAborted()) {
            if (bootloader->programFlash()) {
                if (!bootloader->isAborted()) {
                    if (bootloader->verify()) {
                        bootloader->jumpToApp();
                    }
                }
            }
        }
    }
}
