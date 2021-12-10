#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSettings>
#include <QFileDialog>
#include <QCloseEvent>
#include <QMessageBox>
#include "hidbootloader.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow), bootloader(nullptr)
{
    QSettings settings;
    ui->setupUi(this);
    ui->vidEdit->setText(settings.value("last_vid", "0x4d63").toString());
    ui->pidEdit->setText(settings.value("last_pid", "0x1234").toString());
    ui->fileNameEdit->setText(settings.value("last_file", "").toString());
    ui->baudComboBox->setCurrentIndex(settings.value("last_baud", 0).toInt());
    ui->connectionTypeComboBox->setCurrentIndex(settings.value("last_connection_type", 0).toInt());
    connectLabel = new QLabel("Not connected");
    ui->statusbar->addWidget(connectLabel);
}

MainWindow::~MainWindow()
{
    delete ui;
    if (bootloader) {
        delete bootloader;
    }
}


void MainWindow::on_connectionTypeComboBox_currentTextChanged(const QString &arg1)
{
    if (arg1 == "USB") {
        ui->pidEdit->setEnabled(true);
        ui->vidEdit->setEnabled(true);
        ui->portComboBox->setEnabled(false);
        ui->baudComboBox->setEnabled(false);
    } else if (arg1 == "UART") {
        ui->pidEdit->setEnabled(false);
        ui->vidEdit->setEnabled(false);
        ui->portComboBox->setEnabled(true);
        ui->baudComboBox->setEnabled(true);
    }
}


void MainWindow::on_browseButton_clicked()
{
    fileName = QFileDialog::getOpenFileName(this, "Open hex/bin file", "", "bin/hex files (*.hex *.bin)");
    if (fileName != "") {
        ui->fileNameEdit->setText(fileName);
    }
}



void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue("last_vid", ui->vidEdit->text());
    settings.setValue("last_pid", ui->pidEdit->text());
    settings.setValue("last_file", ui->fileNameEdit->text());
    settings.setValue("last_baud", ui->baudComboBox->currentIndex());
    settings.setValue("last_connection_type", ui->connectionTypeComboBox->currentIndex());
    event->accept();
}

void MainWindow::on_connectButton_clicked()
{
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
        if (bootloader) {
            delete bootloader;
        }
        bootloader = new HidBootloader(vid, pid);
        if (bootloader->isConnected()) {
            connectLabel->setText(QString("Connected: VID = %1 PID = %2")
                                  .arg(ui->vidEdit->text(), ui->pidEdit->text()));
            ui->programButton->setEnabled(true);
            int version = bootloader->getHardwareVersion();
            connectLabel->setText(QString("Connected: VID = %1 PID = %2 Bootloader Version = %3.%4")
                                  .arg(ui->vidEdit->text(), ui->pidEdit->text())
                                  .arg(version >> 8).arg(version & 0xff));
        } else {
            connectLabel->setText("Not connected");
            ui->programButton->setEnabled(false);
            QMessageBox::critical(this, QApplication::applicationName()
                                  , QString("Unable to open device with vid=%1, pid=%2")
                                    .arg(ui->vidEdit->text(), ui->pidEdit->text()));
        }
    }
}

