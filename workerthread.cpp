#include "workerthread.h"

WorkerThread::WorkerThread(Bootloader *boot) : QThread(), bootloader(boot)
{

}


void WorkerThread::run()
{
    //TODO make this more sequential without the deeply nested ifs
    //use a result variable and returns
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
