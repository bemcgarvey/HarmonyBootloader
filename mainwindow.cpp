#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include "hidbootloader.h"
#include "uartbootloader.h"
#include "workerthread.h"
#include <QtSerialPort/QSerialPortInfo>
#include "aboutdialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), bootloader(nullptr), worker(nullptr)
{
    QSettings settings;
    ui->setupUi(this);
    ui->vidEdit->setText(settings.value("last_vid", "0x04d8").toString());
    ui->pidEdit->setText(settings.value("last_pid", "0x003c").toString());
    ui->baudComboBox->setCurrentIndex(settings.value("last_baud", 0).toInt());
    ui->connectionTypeComboBox->setCurrentIndex(settings.value("last_connection_type", 0).toInt());
    readDevices();
    ui->familyComboBox->setCurrentIndex(
                settings.value("last_family_index", 0).toInt());
    ui->eraseSizeEdit->setText(
                settings.value("last_erase_block_size", "8192").toString());
    ui->appStartEdit->setText(
                settings.value("last_start_address", "0x402000").toString());
    std::unique_ptr<QFile> test(new QFile(settings.value("last_file", "").toString()));
    if (test->exists()) {
        ui->fileNameEdit->setText(settings.value("last_file", "").toString());
    }
    ui->statusbar->clearMessage();
    connectLabel = new QLabel("Not connected");
    ui->statusbar->addWidget(connectLabel);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_connectionTypeComboBox_currentTextChanged(const QString &arg1)
{
    if (arg1 == "USB") {
        ui->pidEdit->setEnabled(true);
        ui->vidEdit->setEnabled(true);
        ui->portComboBox->setEnabled(false);
        ui->baudComboBox->setEnabled(false);
        ui->appStartEdit->setEnabled(false);
        ui->eraseSizeEdit->setEnabled(false);
        ui->familyComboBox->setEnabled(false);
    } else if (arg1 == "UART") {
        ui->pidEdit->setEnabled(false);
        ui->vidEdit->setEnabled(false);
        ui->portComboBox->setEnabled(true);
        ui->baudComboBox->setEnabled(true);
        ui->appStartEdit->setEnabled(true);
        ui->eraseSizeEdit->setEnabled(true);
        ui->familyComboBox->setEnabled(true);
        QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
        ui->portComboBox->clear();
        for (auto&& i : portList) {
            ui->portComboBox->addItem(i.portName());
        }
    }
}


void MainWindow::on_browseButton_clicked()
{
    QString filter;
    if (ui->connectionTypeComboBox->currentText() == "USB") {
        filter = "hex files (*.hex)";
    } else if (ui->connectionTypeComboBox->currentText() == "UART"){
        filter = "hex or bin files (*.hex *.bin)";
    }
    QFileInfo fileInfo(ui->fileNameEdit->text());
    fileName = QFileDialog::getOpenFileName(this, "Open firmware file", fileInfo.absolutePath(), filter);
    if (fileName != "") {
        ui->fileNameEdit->setText(fileName);
    }
}



void MainWindow::closeEvent(QCloseEvent *event)
{
    if (worker) {
        if (worker->isRunning()) {
            if (QMessageBox::warning(this, QApplication::applicationName(),
                                     "Programming in progress.  Are you sure you want to terminate"
                                     " the programming?", QMessageBox::Yes | QMessageBox::Cancel)
                    == QMessageBox::Cancel) {
                event->ignore();
                return;
            }
            bootloader->abort();
            worker->wait();
        }
    }
    QSettings settings;
    settings.setValue("last_vid", ui->vidEdit->text());
    settings.setValue("last_pid", ui->pidEdit->text());
    settings.setValue("last_file", ui->fileNameEdit->text());
    settings.setValue("last_baud", ui->baudComboBox->currentIndex());
    settings.setValue("last_connection_type", ui->connectionTypeComboBox->currentIndex());
    settings.setValue("last_erase_block_size", ui->eraseSizeEdit->text());
    settings.setValue("last_start_address", ui->appStartEdit->text());
    settings.setValue("last_family_index", ui->familyComboBox->currentIndex());
    event->accept();
}

void MainWindow::on_connectButton_clicked()
{
    ui->statusbar->clearMessage();
    ui->progressBar->setValue(0);
    if (ui->connectionTypeComboBox->currentText() == "USB") {
        bool ok = false;
        uint16_t vid = ui->vidEdit->text().toInt(&ok, 16);
        if (!ok) {
            QMessageBox::critical(this, QApplication::applicationName(), "Invalid vid - Enter in hex");
            return;
        }
        uint16_t pid = ui->pidEdit->text().toInt(&ok, 16);
        if (!ok) {
            QMessageBox::critical(this, QApplication::applicationName(), "Invalid pid - Enter in hex");
            return;
        }
        bootloader.reset(new HidBootloader(vid, pid));
        if (bootloader->isConnected()) {
            int version = bootloader->readBootInfo();
            connectLabel->setText(QString("Connected: VID = %1 PID = %2 Bootloader Version = %3.%4")
                                  .arg(ui->vidEdit->text(), ui->pidEdit->text())
                                  .arg(version >> 8).arg(version & 0xff));
        } else {
            QMessageBox::critical(this, QApplication::applicationName()
                                  , QString("Unable to open device with vid=%1, pid=%2")
                                    .arg(ui->vidEdit->text(), ui->pidEdit->text()));
        }
    } else if (ui->connectionTypeComboBox->currentText() == "UART") {
        int baud = ui->baudComboBox->currentText().toInt();
        bool ok;
        uint32_t startAddress = ui->appStartEdit->text().toUInt(&ok, 16);
        if (!ok && ui->fileNameEdit->text().endsWith(".bin", Qt::CaseInsensitive)) {
            QMessageBox::critical(this, QApplication::applicationName(), "Invalid start address - Enter in hex");
            return;
        }
        uint32_t eraseBlockSize = ui->eraseSizeEdit->text().toUInt(&ok, 10);
        if (!ok) {
            QMessageBox::critical(this, QApplication::applicationName(), "Invalid erase block size - Enter in decimal");
            return;
        }
        bootloader.reset(new UARTBootloader(ui->portComboBox->currentText(), baud, startAddress, eraseBlockSize));
        if (bootloader->isConnected()) {
            connectLabel->setText(QString("Connected: %1 %2 baud")
                                  .arg(ui->portComboBox->currentText()).arg(baud));
        } else {
            QMessageBox::critical(this, QApplication::applicationName()
                                  , QString("Unable to open port: %1")
                                  .arg(ui->portComboBox->currentText()));
        }
    }
    if (bootloader->isConnected()) {
        connect(bootloader.get(), &Bootloader::message, this, &MainWindow::onMessage);
        connect(bootloader.get(), &Bootloader::progress, this, &MainWindow::onProgress);
        connect(bootloader.get(), &Bootloader::finished, this, &MainWindow::onBootloaderFinished);
        ui->programButton->setEnabled(true);
    } else {
        connectLabel->setText("Not connected");
        ui->programButton->setEnabled(false);
    }
}

void MainWindow::onMessage(QString msg)
{
    ui->statusbar->showMessage(msg, 0);
}

void MainWindow::onProgress(int progress)
{
    ui->progressBar->setValue(progress);
}

void MainWindow::onBootloaderFinished(bool success)
{
    if (worker) {
        worker->wait();
        worker = nullptr;
    }
    if (success) {
        ui->statusbar->showMessage("Programming completed", 0);
        connectLabel->setText("Not Connected");
    } else {
        ui->programButton->setEnabled(true);
        ui->statusbar->showMessage("Programming failed", 0);
    }
}


void MainWindow::on_programButton_clicked()
{
    ui->programButton->setEnabled(false);
    if (!bootloader->setFile(ui->fileNameEdit->text())) {
        QMessageBox::critical(this, QApplication::applicationName()
                              , "Unable to open firmware file.  "
                                "Make sure the file exists and is "
                                "the correct type");
        ui->programButton->setEnabled(true);
        return;
    }
    ui->progressBar->setValue(0);
    worker.reset(new WorkerThread(bootloader.get()));
    worker->start();
}


void MainWindow::on_actionExit_triggered()
{
    close();
}


void MainWindow::on_actionOpen_hex_file_triggered()
{
    on_browseButton_clicked();
}


void MainWindow::on_actionAbout_triggered()
{
    std::unique_ptr<AboutDialog> dlg(new AboutDialog(this));
    dlg->exec();
}


void MainWindow::on_fileNameEdit_textChanged(const QString &arg1)
{
    if (ui->connectionTypeComboBox->currentText() == "UART") {
        if (arg1.endsWith(".hex", Qt::CaseInsensitive)) {
            ui->statusbar->showMessage("Using hex file for flash start address", 3000);
        }
    }
}

void MainWindow::readDevices()
{
    QString val;
    QFile file;
    file.setFileName("devices.json");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    val = file.readAll();
    file.close();
    QJsonDocument d = QJsonDocument::fromJson(val.toUtf8());
    QJsonObject jobj = d.object();
    familiesArray = jobj["families"].toArray();
    for (int i = 0; i < familiesArray.size(); ++i) {
        ui->familyComboBox->addItem(familiesArray[i].toObject()["name"].toString());
    }
}


void MainWindow::on_familyComboBox_currentIndexChanged(int index)
{
    ui->appStartEdit->setText(familiesArray[index].toObject()["app start address"].toString());
    ui->eraseSizeEdit->setText(QString::number(familiesArray[index].toObject()["erase block size"].toInt()));
    if (ui->fileNameEdit->text().endsWith(".hex", Qt::CaseInsensitive)) {
        ui->statusbar->showMessage("Using hex file for flash start address", 3000);
    }
}

