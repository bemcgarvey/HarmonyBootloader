#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include "bootloaderusblink.h"
#include "hidbootloader.h"
#include <memory>
#include <QtSerialPort/QSerialPort>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void on_connectionTypeComboBox_currentTextChanged(const QString &arg1);
    void on_browseButton_clicked();
    void on_connectButton_clicked();

private:
    QString fileName;
    QLabel *connectLabel;
    Ui::MainWindow *ui;
    std::unique_ptr<QSerialPort> port;
    std::unique_ptr<BootLoaderUSBLink> usbLink;
    std::unique_ptr<HidBootloader> bootloader;
protected:
    virtual void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
