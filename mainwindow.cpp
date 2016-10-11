/*
 * Copyright 2016 OSVR and contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <string>
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <fstream>

#include <QFile>
#include <QFileDialog>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QList>
#include <QMessageBox>
#include <QProcess>
#include <QDir>
#include <QtSerialPort>

#include "json/json.h"

// Helper function for validating numerical input
class myValidator : public QDoubleValidator
{
    public:
    myValidator(double bottom, double top, int decimals, QObject * parent) :
    QDoubleValidator(bottom, top, decimals, parent)
    {
    }

    QValidator::State validate(QString &s, int &i) const
    {
        if (s.isEmpty() || s == "-") {
            return QValidator::Intermediate;
        }

        QLocale locale;

        QChar decimalPoint = locale.decimalPoint();
        int charsAfterPoint = s.length() - s.indexOf(decimalPoint) -1;

        if (charsAfterPoint > decimals() && s.indexOf(decimalPoint) != -1) {
            return QValidator::Invalid;
        }

        bool ok;
        double d = locale.toDouble(s, &ok);

        if (ok && d >= bottom() && d <= top()) {
            return QValidator::Acceptable;
        } else {
            return QValidator::Invalid;
        }
    }
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->standingHeight->setValidator( new myValidator(0, 300, 2, this) );
    ui->seatedHeight->setValidator( new myValidator(0, 300, 2, this) );
    ui->ipd->setValidator(new myValidator(0,100,2,0));

    ui->dOsSpherical->setValidator( new myValidator(-100, 100, 2, this) );
    ui->dOsCylindrical->setValidator( new myValidator(-100, 100, 2, this) );
    ui->dOsAxis->setValidator( new myValidator(-100, 300, 2, this) );
    ui->nOsAdd->setValidator( new myValidator(-100, 100, 2, this) );

    ui->dOdSpherical->setValidator( new myValidator(-100, 100, 2, this) );
    ui->dOdCylindrical->setValidator( new myValidator(-100, 100, 2, this) );
    ui->dOdAxis->setValidator( new myValidator(-100, 300, 2, this) );
    ui->nOdAdd->setValidator( new myValidator(-100, 100, 2, this) );

    ui->tabWidget->setCurrentIndex(0);

    // Temporarily remove the USER Settings tab
    ui->tabWidget->removeTab(3);

    QProcess process;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString programPath = "";
    programPath = env.value("PROGRAMDATA","C:/ProgramData");
    m_osvrUserConfigFilename = QString(programPath+"/OSVR/osvr_user_settings.json");
    loadConfigFile(m_osvrUserConfigFilename);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_aboutButton_clicked()
{
    QMessageBox::information(0, QString("OSVR Control Panel Interface"), QString("This application is a lightweight utility to help you configure your personal OSVR settings. For more information, visit <a href=http://www.osvr.org>osvr.org</a>."), QMessageBox::Ok);
    // loadConfigFile(QString("start.json"));
}

void MainWindow::on_exitButton_clicked()
{
    QApplication::quit();
}

// User setting functions-------------------------------------------------------------
bool MainWindow::loadConfigFile(QString filename)
{
    char* cstr;
    std::string fname = filename.toStdString();
    cstr = new char [fname.size()+1];
    strcpy( cstr, fname.c_str() );

    std::ifstream file_id;
    file_id.open(cstr);

    Json::Reader reader;
    Json::Value value;
    if (!reader.parse (file_id, value)){
         qWarning("Couldn't open save file; creating file.");
        // new file just has default values
        saveConfigFile(filename);
    }else{
        m_osvrUser.read(value);
    }
    updateFormValues();
    return true;
}

void MainWindow::updateFormValues(){
    if ("Male" == m_osvrUser.gender())
        ui->gender->setCurrentIndex(0);
    else
        ui->gender->setCurrentIndex(1);
    ui->standingHeight->setText(QString::number(m_osvrUser.standingEyeHeight()));
    ui->seatedHeight->setText(QString::number(m_osvrUser.seatedEyeHeight()));
    ui->ipd->setText(QString::number(m_osvrUser.pupilDistance(OS)+m_osvrUser.pupilDistance(OD)));

    ui->dOsSpherical->setText(QString::number(m_osvrUser.spherical(OS)));
    ui->dOsCylindrical->setText(QString::number(m_osvrUser.cylindrical(OS)));
    ui->dOsAxis->setText(QString::number(m_osvrUser.axis(OS)));
    ui->nOsAdd->setText(QString::number(m_osvrUser.addNear(OS)));
    ui->OSdominant->setChecked(m_osvrUser.dominant(OS));

    ui->dOdSpherical->setText(QString::number(m_osvrUser.spherical(OD)));
    ui->dOdCylindrical->setText(QString::number(m_osvrUser.cylindrical(OD)));
    ui->dOdAxis->setText(QString::number(m_osvrUser.axis(OD)));
    ui->nOdAdd->setText(QString::number(m_osvrUser.addNear(OD)));
    ui->ODdominant->setChecked(m_osvrUser.dominant(OD));
}

void MainWindow::loadValuesFromForm(OSVRUser *oo)
{
    double ipd;

    oo->setGender(ui->gender->currentText().toStdString());

    // IPD settings
    if (!ui->ipd->text().isEmpty()){
        ipd = ui->ipd->text().toDouble();
        oo->setPupilDistance(OS,ipd/2);
        oo->setPupilDistance(OD,ipd/2);
    }

    //Left eye settings
    if (!ui->dOsSpherical->text().isEmpty())
        oo->setSpherical(OS,ui->dOsSpherical->text().toDouble());
    if (!ui->dOsCylindrical->text().isEmpty())
        oo->setCylindrical(OS,ui->dOsCylindrical->text().toDouble());
    if (!ui->dOsAxis->text().isEmpty())
        oo->setAxis(OS,ui->dOsAxis->text().toDouble());
    if (!ui->nOsAdd->text().isEmpty())
        oo->setAddNear(OS,ui->nOsAdd->text().toDouble());

    // right eye settings
    if (!ui->dOdSpherical->text().isEmpty())
        oo->setSpherical(OD,ui->dOdSpherical->text().toDouble());
    if (!ui->dOdCylindrical->text().isEmpty())
        oo->setCylindrical(OD,ui->dOdCylindrical->text().toDouble());
    if (!ui->dOdAxis->text().isEmpty())
        oo->setAxis(OD,ui->dOdAxis->text().toDouble());
    if (!ui->nOdAdd->text().isEmpty())
        oo->setAddNear(OD,ui->nOdAdd->text().toDouble());

    if(ui->ODdominant->isChecked())
        oo->setDominant(OD);
    else
        oo->setDominant(OS);

    // anthropometric settings
    if (!ui->standingHeight->text().isEmpty())
        oo->setStandingEyeHeight(ui->standingHeight->text().toDouble());
    if (!ui->seatedHeight->text().isEmpty())
        oo->setSeatedEyeHeight(ui->seatedHeight->text().toDouble());
}

void MainWindow::saveConfigFile(QString filename)
{
    char* cstr;
    std::string fname = filename.toStdString();
    cstr = new char [fname.size()+1];
    strcpy( cstr, fname.c_str() );

    Json::Value ooo;
    m_osvrUser.write(ooo);

    std::ofstream out_file;
    out_file.open(cstr);
    Json::StyledWriter styledWriter;
    out_file <<styledWriter.write(ooo);
    out_file.close();
}

void MainWindow::on_resetButton_clicked()
{
    updateFormValues();
}

void MainWindow::on_saveButton_clicked()
{
    loadValuesFromForm(&m_osvrUser);
    saveConfigFile(m_osvrUserConfigFilename);
}

// SW Tab-------------------------------------------------------------
// Recenter HMD
// Direct mode toggles

void MainWindow::on_recenterButton_clicked()
{
    QProcess *process = new QProcess(this);
    QString file = "reset_yaw.bat";
    process->start(file);
}

void MainWindow::on_GPUType_currentIndexChanged(const QString &gpu_type)
{
    m_GPU_type = gpu_type;
}

void MainWindow::on_directModeButton_clicked()
{
    QProcess *process = new QProcess(this);
    if (m_GPU_type.contains("NVIDIA"))
        process->start("enableOSVRDirectMode.exe");
    else if (m_GPU_type.contains("AMD"))
        process->start("enableOSVRDirectModeAMD.exe");
    else{
        // do nothing for now
    }
    return;
}

void MainWindow::on_extendedModeButton_clicked()
{
    QProcess *process = new QProcess(this);
    if (m_GPU_type.contains("NVIDIA"))
        process->start("disableOSVRDirectMode.exe");
    else if (m_GPU_type.contains("AMD"))
        process->start("disableOSVRDirectModeAMD.exe");
    else{
        // do nothing for now
    }
    return;
}

// HMD Tab-------------------------------------------------------------
void MainWindow::on_checkFWButton_clicked()
{
    QString versions = getFirmwareVersionsString();
    if (versions == QString::null) {
        QMessageBox::critical(0, QString("Error"), QString("Error: Cannot read firmware version. Ensure all cables are connected according to the manual."), QMessageBox::Ok);
        return;
    } else {
        QMessageBox::information(0, "Firmware Version Information", versions, QMessageBox::Ok);
    }
}

// FW Update button
void MainWindow::on_updateFWButton_clicked()
{

    QMessageBox::StandardButton reply;
    QString hexFile;
    QString fileFilter = "*.hex";

    // find the OSVR HDK and get current FW version
    QString firmware_versions = getFirmwareVersionsString();
    if (firmware_versions != QString::null) {
        reply = QMessageBox::question(this,tr("Ready To Update Firmware Versions"),
                "<b>Current Firmware Versions:</b><br>" + firmware_versions + "<br><br>Do you wish to proceed with the firmware update?",
                QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::No)
            return;
    }
    else
    {
        QMessageBox::critical(0, QString("Error"), QString("Error: Cannot read firmware version. Ensure all cables are connected as shown in the manual."), QMessageBox::Ok);
        return;
    }

    // ask User for the HEX file to update with
    hexFile = QFileDialog::getOpenFileName(this,QString("Select Firmware Update File"),fileFilter,0);
    if (hexFile == ""){
         return; // user pressed cancel
    }

    QMessageBox msgBox;
    msgBox.setText("Firmware hex file: " + hexFile);
    msgBox.setWindowTitle("Firmware Update In Progress...");
    msgBox.exec();

    sendCommandNoResult("#?b1948\n");

    msgBox.setText("This application uses the open source <a href=\"dfu-programmer.github.io\">dfu-programmer project</a>.\n\nAt this time your device should now be in ATMEL bootloader mode. If you haven't loaded the ATMEL drivers yet, you should do so now. You can find the drivers in this distribution in the dfu-prog-usb-1.2.2 folder.\n\nRight click on the device in the Device Manager and select Update Driver Software.");
    msgBox.setWindowTitle("Firmware Update Complete!");
    msgBox.exec();

    atmel_erase();
    atmel_load(hexFile);
    atmel_launch();

    // Verify FW version
    firmware_versions = getFirmwareVersionsString();
    if (firmware_versions ==  QString::null) {
        QMessageBox::critical(0, QString("Error"), QString("Error: Cannot read firmware version. Ensure all cables are connected according to the manual."), QMessageBox::Ok);
    } else {
        QMessageBox::information(0, QString("New Firmware Versions"), firmware_versions, QMessageBox::Ok);
    }
}

void MainWindow::atmel_erase()
{
    QProcess *process = new QProcess(this);
    QString file = "dfu-programmer.exe atxmega256a3bu erase";
    process->start(file);
    QThread::sleep(3);
}

void MainWindow::atmel_load(QString fwFile)
{
    QProcess *process = new QProcess(this);
    QString file = "dfu-programmer.exe atxmega256a3bu flash \"" + fwFile + "\"";
    process->start(file);
    QThread::sleep(3);
}

void MainWindow::atmel_launch()
{
    QProcess *process = new QProcess(this);
    QString file = "dfu-programmer.exe atxmega256a3bu launch";
    process->start(file);
    QThread::sleep(5);
}


QString MainWindow::findSerialPort(int VID, int PID)
{
    QString outputString = "";

    QString description;
    QString manufacturer;
    QString serialNumber;
    QString portName;
    bool deviceFound = false;

   portName = "Not found";
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if (info.vendorIdentifier()==VID && info.productIdentifier()==PID){
            description = info.description();
            manufacturer = info.manufacturer();
            serialNumber = info.serialNumber();
            outputString += "\n";
            outputString += QObject::tr("Port: ") + info.portName() + "\n"
                    + QObject::tr("Location: ") + info.systemLocation() + "\n"
                    + QObject::tr("Description: ") + info.description() + "\n"
                    + QObject::tr("Manufacturer: ") + info.manufacturer() + "\n"
                    + QObject::tr("Serial number: ") + info.serialNumber() + "\n"
                    + QObject::tr("Vendor Identifier: ") + (info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString()) + "\n"
                    + QObject::tr("Product Identifier: ") + (info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16) : QString()) + "\n"
                    + QObject::tr("Busy: ") + (info.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) + "\n";
            deviceFound = true;
            portName = info.portName();
         }
    }

    if (m_verbose) {
        if (deviceFound) {
            QMessageBox::information(0, "Device Located", "COM port: " + outputString);
        } else {
            QMessageBox::warning(0, "Unable To Locate Device", "Unable to find device. Ensure it is connected as shown in the manual.");
        }
    }

    return portName;
}

QSerialPort *MainWindow::openSerialPort(QString portName)
{
    QSerialPort *thePort = new QSerialPort(this);

    thePort->setPortName(portName);
    thePort->setBaudRate(QSerialPort::Baud57600);
    thePort->setDataBits(QSerialPort::Data8);
    thePort->setParity(QSerialPort::NoParity);
    thePort->setStopBits(QSerialPort::OneStop);
    thePort->setFlowControl(QSerialPort::NoFlowControl);
    if (thePort->open(QIODevice::ReadWrite)) {
        return thePort;
    } else {
        if (m_verbose)
            QMessageBox::critical(this,tr("Unable To Open Port"),"Unable to open serial port " + portName + ".\nPlease ensure the device is connected as shown in the manual and try again.\n");
        return NULL;
    }
}

void MainWindow::writeSerialData(QSerialPort *thePort, const QByteArray &data)
{
    if (thePort->write(data)== -1){
        if (m_verbose)
            QMessageBox::critical(this,tr("Error Writing To Serial Port"),"Unable to write to serial port.\nPlease ensure the device is connected as shown in the manual and try again.\n");
    }
    thePort->flush();
    thePort->waitForBytesWritten(5000);
    QThread::sleep(1);
}

QString MainWindow::sendCommandWaitForResults(QByteArray theCommand){
    QByteArray theResult="";
    QSerialPort *thePort;
    QString portName;

    // find the OSVR HDK and get current FW version
    portName = findSerialPort(0x1532, 0x0B00);
    if (portName != "Not found"){
        thePort = openSerialPort(portName);
        if (thePort){
            writeSerialData(thePort,theCommand);
            if (thePort->waitForReadyRead(5000)){
                theResult = thePort->readAll();
            }
            thePort->close();
        }
    }else{
        if (m_verbose)
           QMessageBox::critical(this,tr("Error Sending Command"),"Unable to retrieve results from command '" + theCommand + "'. Try reconnecting and power cycling your HMD.");
    }
    return theResult;
}

void MainWindow::sendCommandNoResult(QByteArray theCommand){
    QSerialPort *thePort;
    QString portName;

    // find the OSVR HDK and get current FW version
    portName = findSerialPort(0x1532, 0x0B00);
    if (portName != "Not found"){
        thePort = openSerialPort(portName);
        if (thePort){
            writeSerialData(thePort,theCommand);
        }
        thePort->close();
    }
}

QString MainWindow::getFirmwareVersionsString() {
    QString response = sendCommandWaitForResults("#?v\n");
    QString result = QString::null;

    if (response !=  "") {
        response = response.replace("\r", "");
        QStringList split = response.split("\n");

        QStringList fw_version_split = split.at(1).split(" ");
        QStringList tracker_version_split = split.at(2).split(":");

        result = "<u>HMD Main Board:</u> " + fw_version_split.at(1) +
                          " (" + fw_version_split.at(3) +
                          " " + fw_version_split.at(4) +
                          ", " + fw_version_split.at(5) +
                          ")<br>" +
                          "<u>IR LED Control Board:</u> " +
                          tracker_version_split.at(1);
    }

    return result;
}

void MainWindow::on_enableDisplayButton_clicked()
{
    sendCommandNoResult("#Hi\n");
}

void MainWindow::on_toggleSBSButton_clicked()
{
    // Toggle side by side mode for 1.x HMDs
    sendCommandNoResult("#f1s\n");
}

void MainWindow::on_screenPersistenceFull_clicked()
{
    sendCommandNoResult("#sp03C50");
}

void MainWindow::on_screenPersistenceMedium_clicked()
{
    sendCommandNoResult("#sp03C32");
}

void MainWindow::on_screenPersistenceLow_clicked()
{
    sendCommandNoResult("#sp03C0A");
}
