#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ps2000Con.h"
#include <algorithm>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    umask(0);
    ui->setupUi(this);
    unitOpened.handle = ps2000_open_unit ();
    if ( !unitOpened.handle )
    {
        qDebug() << "Unable to open device";
        get_info ();
        while( !_kbhit() );
        exit ( 99 );
    }else{
        qDebug() << "Device opened successfully\n\n";

        get_info ();
    }
}

MainWindow::~MainWindow()
{
    ps2000_close_unit ( unitOpened.handle );
    delete ui;
}

int write_file(int fd, char *str)
{
    qDebug() << str;
    return write(fd, str, strlen(str));
}



void MainWindow::on_startButton_clicked()
{
    qDebug() << ui->comboBox->currentIndex();
    int16_t waveform;
    int32_t frequency;
    //int16_t waveformSize = 0;

    frequency = ui->frecBox->value();
    qDebug("Signal generator On");
    qDebug() << frequency;
    waveform = ui->comboBox->currentIndex();

    /*ps2000_set_sig_gen_built_in (unitOpened.handle,
                                    0,
                                    1000000, // 1 volt
                                    (PS2000_WAVE_TYPE) waveform,
                                    (float)frequency,
                                    (float)frequency,
                                    0,
                                    0,
                                    PS2000_UPDOWN, 0);*/
    //FILE *device;
    int device = open("/dev/usbtmc0", O_RDWR | O_NOCTTY);
    qDebug() << strerror(errno);
    if (device == -1)
    {
        qDebug() << "f ";

    }
    QString wave = "C1:BSWV WVTP, ";
    QString fr   = "C1:BSWV FRQ, ";
    wave.append(ui->comboBox->currentText());
    fr.append(ui->frecBox->text());
    QByteArray br = wave.toLocal8Bit();
    write_file(device, br.data());
    usleep(10000);
    br = fr.toLocal8Bit();
    write_file(device, br.data());
    usleep(10000);
    write_file(device, "C1:BSWV AMP, 2");
    usleep(10000);
    write_file(device, "C1:OUTP ON");
    usleep(10000);

}


void MainWindow::on_pushButton_clicked()
{

    double xx[BUFFER_SIZE];
    double yyA[BUFFER_SIZE];
    double yyB[BUFFER_SIZE];
    QVector<double> x(BUFFER_SIZE), yA(BUFFER_SIZE), yB(BUFFER_SIZE);
    collect_block_immediate(xx, yyA, yyB);
    for(int i = 0; i < BUFFER_SIZE; i++)
    {
        x[i] = xx[i];
        yA[i] = yyA[i];
        yB[i] = yyB[i];
        qDebug("%lf, %lf \n", x[i], yA[i]);
    }
    ui->customPlot->addGraph();
    ui->customPlot->graph(0)->setPen(QPen(Qt::blue)); // line color blue for first graph
    ui->customPlot->graph(0)->setBrush(QBrush(QColor(0, 0, 255, 20))); // first graph will be filled with translucent blue
    ui->customPlot->addGraph();
    ui->customPlot->graph(1)->setPen(QPen(Qt::red)); // line color red for second graph
    // give the axes some labels:
    ui->customPlot->xAxis2->setVisible(true);
    ui->customPlot->xAxis2->setTickLabels(false);
    ui->customPlot->yAxis2->setVisible(true);
    ui->customPlot->yAxis2->setTickLabels(false);
    ui->customPlot->xAxis->setLabel("Time");
    ui->customPlot->yAxis->setLabel("Voltage");
    // make left and bottom axes always transfer their ranges to right and top axes:
    connect(ui->customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->xAxis2, SLOT(setRange(QCPRange)));
    connect(ui->customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->customPlot->yAxis2, SLOT(setRange(QCPRange)));
    // pass data points to graphs:
    ui->customPlot->graph(0)->setData(x, yA);
    ui->customPlot->graph(1)->setData(x, yB);
    // let the ranges scale themselves so graph 0 fits perfectly in the visible area:
    ui->customPlot->graph(0)->rescaleAxes();
    // same thing for graph 1, but only enlarge ranges (in case graph 1 is smaller than graph 0):
    ui->customPlot->graph(1)->rescaleAxes(true);
    // Note: we could have also just called customPlot->rescaleAxes(); instead
    // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
    ui->customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    //finding K
    double U_sine_out = *std::max_element(yA.begin(), yA.end());
    ui->lcdK->display(U_sine_out / 0.5);
}


void MainWindow::on_pushButton_2_clicked()
{
        //FFT
        mFftIn  = fftw_alloc_real(BUFFER_SIZE);
        mFftOut = fftw_alloc_real(BUFFER_SIZE);
        //FOR DATA
        double R = ui->resister->value();
        double C = ui->capacity->value();


        QVector<double> K;
        QVector<double> FR;
        double DATA_A[BUFFER_SIZE];
        double uselesstime[BUFFER_SIZE];
        double DATA_B[BUFFER_SIZE];
        //QVector<double> chA_V(BUFFER_SIZE);

        //int16_t waveformSize = 0;
        // configure right and top axis to show ticks but no labels:
        // (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
        ui->deltaPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        ui->deltaPlot->legend->setVisible(false);
        ui->deltaPlot->yAxis->setLabel("");
        ui->deltaPlot->xAxis->setLabel("Frequency, MHz");
        ui->deltaPlot->xAxis->setRange(RANGE_START, RANGE_END);
        ui->deltaPlot->yAxis->setRange(0.0, 500.0);
        ui->deltaPlot->clearGraphs();
        ui->deltaPlot->addGraph();

        ui->deltaPlot->graph()->setPen(QPen(Qt::black));
        ui->deltaPlot->graph()->setName("fft_chA");
        // make left and bottom axes always transfer their ranges to right and top axes:
        connect(ui->deltaPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->deltaPlot->xAxis2, SLOT(setRange(QCPRange)));
        connect(ui->deltaPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->deltaPlot->yAxis2, SLOT(setRange(QCPRange)));



        collect_block_immediate(uselesstime, DATA_A, DATA_B);
        freqMult = 1000 / ( (uselesstime[2] - uselesstime[1]) * BUFFER_SIZE );
        qDebug() << uselesstime[1];
        for(int i = 0; i < BUFFER_SIZE; i++)
        {
            mFftIn[i] = DATA_A[i];
        }

        mFftOut = fftw_alloc_real(BUFFER_SIZE);
        mFftPlan = fftw_plan_r2r_1d(BUFFER_SIZE, mFftIn, mFftOut, FFTW_R2HC,FFTW_ESTIMATE);

        fftw_execute(mFftPlan);

        QVector<double> fftVec;
        for (int i = 0;
                 i < 512;
                 i += 1) {
            fftVec.append((sqrt(mFftOut[i]*mFftOut[i] + mFftOut[BUFFER_SIZE - i]*mFftOut[BUFFER_SIZE - i]))/1024);
            mFftIndices.append(i * freqMult);
        }


        ui->deltaPlot->graph(0)->setData(mFftIndices,fftVec);
        ui->deltaPlot->replot();
        ui->deltaPlot->graph(0)->rescaleAxes();
        // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
        ui->deltaPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

        //PLOT FOR INPUT FT
        mFftIn  = fftw_alloc_real(BUFFER_SIZE);
        mFftOut = fftw_alloc_real(BUFFER_SIZE);

        ui->FTI->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        ui->FTI->legend->setVisible(false);
        ui->FTI->yAxis->setLabel("");
        ui->FTI->xAxis->setLabel("Frequency, MHz");
        ui->FTI->xAxis->setRange(RANGE_START, RANGE_END);
        ui->FTI->yAxis->setRange(0.0, 500.0);
        ui->FTI->clearGraphs();
        ui->FTI->addGraph();

        ui->FTI->graph()->setPen(QPen(Qt::black));
        ui->FTI->graph()->setName("fft_chB");
        // make left and bottom axes always transfer their ranges to right and top axes:
        connect(ui->FTI->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->FTI->xAxis2, SLOT(setRange(QCPRange)));
        connect(ui->FTI->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->FTI->yAxis2, SLOT(setRange(QCPRange)));

        for(int i = 0; i < BUFFER_SIZE; i++)
        {
            mFftIn[i] = DATA_B[i];
        }

        mFftOut = fftw_alloc_real(BUFFER_SIZE);
        mFftPlan = fftw_plan_r2r_1d(BUFFER_SIZE, mFftIn, mFftOut, FFTW_R2HC,FFTW_ESTIMATE);

        fftw_execute(mFftPlan);

        QVector<double> fftIn;
        for (int i = 0;
                 i < 512;
                 i += 1) {
            fftIn.append((sqrt(mFftOut[i]*mFftOut[i] + mFftOut[BUFFER_SIZE - i]*mFftOut[BUFFER_SIZE - i]))/1024);
        }

        ui->FTI->graph(0)->setData(mFftIndices,fftIn);
        ui->FTI->replot();
        ui->FTI->graph(0)->rescaleAxes();
        // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
        ui->FTI->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);


        fftw_free(mFftIn);
        fftw_free(mFftOut);
        fftw_destroy_plan(mFftPlan);
        //SOLVING

        ui->HFunc->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        ui->HFunc->legend->setVisible(false);
        ui->HFunc->yAxis->setLabel("");
        ui->HFunc->xAxis->setLabel("Frequency, MHz");
        ui->HFunc->xAxis->setRange(RANGE_START, RANGE_END);
        ui->HFunc->yAxis->setRange(0.0, 500.0);
        ui->HFunc->clearGraphs();
        ui->HFunc->addGraph();
        ui->HFunc->addGraph();
        ui->HFunc->graph(0)->setPen(QPen(Qt::blue));
        ui->HFunc->graph()->setName("H");
        ui->HFunc->graph(1)->setPen(QPen(Qt::red));
        // make left and bottom axes always transfer their ranges to right and top axes:
        connect(ui->HFunc->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->HFunc->xAxis2, SLOT(setRange(QCPRange)));
        connect(ui->HFunc->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->HFunc->yAxis2, SLOT(setRange(QCPRange)));


        QVector<double> fftH;
        for(int i = 0;
                i < 512;
                i++)
        {
            if(fftIn[i] > 35)
            {
                fftH.append(fftVec[i] / fftIn[i]);
            }else fftH.append(0);
            K.append(0.25 / sqrt(1 + freqMult * freqMult * i * i *  R * R * C * C));
        }


        ui->HFunc->graph(0)->setData(mFftIndices,fftH);
        ui->HFunc->graph(1)->setData(mFftIndices, K);



        ui->HFunc->graph(0)->rescaleAxes();
        // same thing for graph 1, but only enlarge ranges (in case graph 1 is smaller than graph 0):
        ui->HFunc->graph(1)->rescaleAxes(true);
        // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
        ui->HFunc->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

}




