#include <QDebug>
#include <QTextStream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lbl_Voltage->setText(QString("0.0V"));
    ui->lbl_Current->setText(QString("0.0A"));
    ui->lbl_Rdiff->setText("Rdiff:");

    m_port.setBaudRate(QSerialPort::Baud115200);
    connect(&m_port, &QSerialPort::readyRead, this, &MainWindow::new_data);
    connect(&m_port, &QSerialPort::errorOccurred, this, &MainWindow::serial_error);

    auto ports =  QSerialPortInfo::availablePorts();

    for( auto port: ports)
    {
        ui->comboPort->addItem(port.portName());
    }


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::serial_error(QSerialPort::SerialPortError error)
{
    if( error != QSerialPort::SerialPortError::NotOpenError &&
        error != QSerialPort::SerialPortError::NoError)
    {
        qDebug() << error;
        ui->statusbar->showMessage(m_port.errorString(), 10*1000);
        m_port.close();
        ui->btnOpen->setChecked(false);
    }
}

void MainWindow::new_data()
{
    static QByteArray data;
    data.append(m_port.readAll());
    int p;
    if( (p = data.indexOf("\r\n"))>0)
    {
        QTextStream strm(data);
        QString s_data(strm.readLine());
        data.remove(0,p+2);

        parse_serial(s_data);
    }
}

void MainWindow::parse_serial(QString data)
{
    QString cmd = data.section(':',0,0);
    if( cmd == "meas")
    {
        QString measurements = data.section(':',1,1);
        float voltage = measurements.section(',',0,0).toFloat();
        float current = measurements.section(',',1,1).toFloat();

        ui->lbl_Voltage->setText(QString("%1V").arg(voltage,2,'f',2));
        ui->lbl_Current->setText(QString("%1A").arg(current,2,'f',2));
    }
    else if( cmd == "Rdiff")
    {
        QString m = data.section(':', 1,1);
        float r = m.toFloat();
        ui->lbl_Rdiff->setText(QString("Rdiff:%1 Ohm").arg(r, 2,'f',4));
    }
    else if( cmd == "Error")
    {
        ui->statusbar->showMessage(data,4000);
    }
}

void MainWindow::on_pushButton_toggled(bool checked)
{
    QByteArray cmd = checked ? "on_1a":"off_1a";
    send_cmd(cmd);
}

void MainWindow::on_pushButton_2_toggled(bool checked)
{
    QByteArray cmd = checked ? "on_2a":"off_2a";
    send_cmd(cmd);
}

void MainWindow::on_pushButton_3_clicked()
{
    send_cmd("btest");
}

void MainWindow::send_cmd(QByteArray cmd)
{
    if( m_port.isOpen() )
    {
        cmd.append('\r');
        m_port.write(cmd);
    }
}

void MainWindow::on_btnOpen_clicked()
{
    if( ui->btnOpen->isChecked())
    {
        m_port.setPortName(ui->comboPort->currentText());

        if( m_port.open(QIODevice::ReadWrite))
        {
            ui->btnOpen->setChecked(true);
        }
    }
    else
    {
        m_port.close();
        ui->btnOpen->setChecked(false);
    }
}
