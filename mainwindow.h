#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qcustomplot.h"
#include "ps2000.h"
#include <fftw3.h>

#define RANGE_START 0
#define RANGE_END   1024000 /* very optimistic */
#define SAMPLE_FREQ 12500000
#define BUFFER_SIZE 1024

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QVector<double> mFftIndices;

    fftw_plan mFftPlan;
    double *mFftIn;
    double *mFftOut;

private slots:
    void on_startButton_clicked();

    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
