#include <QDebug>
#include <QTextStream>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QSerialPortInfo>

#include <qwt_plot.h>
#include <qwt_plot_grid.h>

#include <qwt_legend.h>

#include <qwt_plot_curve.h>
#include <qwt_symbol.h>

#include <qwt_plot_magnifier.h>

#include <qwt_plot_panner.h>

#include <qwt_plot_picker.h>
#include <qwt_picker_machine.h>


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

    ui->lbl_PWMvalue->setText("");

    QwtPlotGrid *grid = new QwtPlotGrid();

    grid->setMajorPen(QPen(Qt::gray, 1));
    grid->attach(ui->qwtPlot);

    addCurve();
    ui->qwtPlot->setAutoReplot();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onTimer()
{
}

void MainWindow::serial_error(QSerialPort::SerialPortError error)
{
    if( error != QSerialPort::SerialPortError::NotOpenError &&
        error != QSerialPort::SerialPortError::NoError)
    {
        qDebug() << error;
        ui->statusbar->showMessage(m_port.errorString(), 100);
        m_port.close();
        ui->btnOpen->setChecked(false);
    }
}

void MainWindow::new_data()
{
    static QByteArray data;
    auto d = m_port.readAll();
    //qDebug() << "new data:" << d;
    data.append(d);
    int p;
    while((p = data.indexOf("\r\n"))>0)
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
    qDebug() << "serial data: " << data;
    if( cmd == "meas")
    {
        QString measurements = data.section(':',1,1);
        float voltage = measurements.section(',',0,0).toFloat();
        float current = measurements.section(',',1,1).toFloat();

        ui->lbl_Voltage->setText(QString("%1V").arg(voltage,2,'f',2));
        ui->lbl_Current->setText(QString("%1A").arg(current,2,'f',2));

        addSamples(voltage, current);
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
    else if( cmd == "pwma" )
    {
        auto arg = data.section(':', 1, 1);
        ui->lbl_PWMvalue->setText(arg);
    }
    else if( cmd == "R1A" || cmd == "R2A" )
    {
        QString measurements = data.section(':',1,1);
        float voltage = measurements.section(',',0,0).toFloat();
        float current = measurements.section(',',1,1).toFloat();

        addSamples(voltage, current);
    }
}

void MainWindow::addSamples(float voltage, float current)
{

    static float x=0.f;
    m_points_voltage << QPointF(x, voltage);
    m_points_current << QPointF(x, current);

    m_curve_current->setSamples(m_points_current);
    m_curve_volt->setSamples(m_points_voltage);
    x+=0.1;
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
        cmd.append("\r\n");
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

void MainWindow::on_sldr_PWMvalue_valueChanged(int value)
{
    QString s_val(QString("%1").arg(value));

    QByteArray cmd;
    cmd.append("pwm:");
    cmd.append(s_val.toLocal8Bit());

    send_cmd(cmd);

}

void MainWindow::addCurve()
{
    m_curve_volt = new QwtPlotCurve;

    m_curve_volt->setTitle("curve voltage");
    m_curve_volt->setPen(Qt::blue, 2);

    m_curve_volt->attach(ui->qwtPlot);

    m_curve_current = new QwtPlotCurve;
    m_curve_current->setTitle("curve current");
    m_curve_current->setPen(Qt::red, 2);


    m_curve_current->attach(ui->qwtPlot);
}
