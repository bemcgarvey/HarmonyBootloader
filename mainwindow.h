#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QJsonArray>
#include "bootloader.h"
#include "workerthread.h"
#include <memory>

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
    void onMessage(QString msg);
    void onProgress(int progress);
    void onBootloaderFinished(bool success);
    void on_programButton_clicked();
    void on_actionExit_triggered();
    void on_actionOpen_hex_file_triggered();
    void on_actionAbout_triggered();
    void on_fileNameEdit_textChanged(const QString &arg1);

    void on_familyComboBox_currentIndexChanged(int index);

private:
    QString fileName;
    QLabel *connectLabel;
    Ui::MainWindow *ui;
    std::unique_ptr<Bootloader> bootloader;
    std::unique_ptr<WorkerThread> worker;
    void readDevices();
    QJsonArray familiesArray;
protected:
    virtual void closeEvent(QCloseEvent *event) override;
};
#endif // MAINWINDOW_H
