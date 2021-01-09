#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include <QTextStream>

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

private slots:
    void on_pushButton_toggled(bool checked);

    void on_pushButton_2_toggled(bool checked);

    void on_pushButton_3_clicked();

    void on_btnOpen_clicked();

private:
    void parse_serial(QString data);
    void send_cmd(QByteArray);

private:
    Ui::MainWindow *ui;
    QSerialPort m_port;
    QTextStream m_serialstream;
};
#endif // MAINWINDOW_H
