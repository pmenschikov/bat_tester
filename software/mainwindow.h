#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTextStream>
#include <QTimer>

#include <qwt_plot_curve.h>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void new_data();
    void serial_error(QSerialPort::SerialPortError);

private slots:
    void on_pushButton_toggled(bool checked);

    void on_pushButton_2_toggled(bool checked);

    void on_pushButton_3_clicked();

    void on_btnOpen_clicked();

    void on_sldr_PWMvalue_valueChanged(int value);

    void onTimer();

private:
    void parse_serial(QString data);
    void send_cmd(QByteArray);
    void addCurve();
    void addSamples(float v, float i);

private:
    Ui::MainWindow *ui;
    QSerialPort m_port;

    QwtPlotCurve *m_curve_volt;
    QwtPlotCurve *m_curve_current;
    QTimer m_timer;
    QPolygonF m_points_voltage;
    QPolygonF m_points_current;
};
#endif // MAINWINDOW_H
