#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ps2000Con.h"
#include <algorithm>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
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


void MainWindow::on_startButton_clicked()
{
    qDebug() << ui->comboBox->currentIndex();
    int16_t waveform;
    int32_t frequency;
    //int16_t waveformSize = 0;

    frequency = ui->frecBox->value();
    qDebug("Signal generator On");

    printf("0:\tSINE\n");
    printf("1:\tSQUARE\n");
    printf("2:\tTRIANGLE\n");
    printf("3:\tRAMP UP\n");
    printf("4:\tRAMP DOWN\n");

    waveform = ui->comboBox->currentIndex();

    ps2000_set_sig_gen_built_in (unitOpened.handle,
                                    0,
                                    1000000, // 1 volt
                                    (PS2000_WAVE_TYPE) waveform,
                                    (float)frequency,
                                    (float)frequency,
                                    0,
                                    0,
                                    PS2000_UPDOWN, 0);
}


void MainWindow::on_pushButton_clicked()
{
    QFile data("data.txt");
    data.open(QIODevice::WriteOnly);
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
        data.write( (char*)(&yA[i]),sizeof(yA[i]));
        qDebug("%lf, %lf \n", x[i], yA[i]);
    }
    data.close();
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
        /*kiss_fftr_cfg cfg = kiss_fftr_alloc(BUFFER_SIZE, 0, NULL, NULL);
        kiss_fft_scalar cx_in[BUFFER_SIZE];
        kiss_fft_cpx cx_out[BUFFER_SIZE/2+1];*/
        double DATA[BUFFER_SIZE];
        double uselesstime[BUFFER_SIZE];
        double uselesschB[BUFFER_SIZE];
        //QVector<double> chA_V(BUFFER_SIZE);
        //FOR GENERATOR
        int16_t waveform;
        int32_t frequency;
        //int16_t waveformSize = 0;
        // configure right and top axis to show ticks but no labels:
        // (see QCPAxisRect::setupFullAxesBox for a quicker method to do this)
        ui->deltaPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
        ui->deltaPlot->legend->setVisible(false);
        ui->deltaPlot->yAxis->setLabel("");
        ui->deltaPlot->xAxis->setLabel("Frequency");
        ui->deltaPlot->xAxis->setRange(RANGE_START, RANGE_END);
        ui->deltaPlot->yAxis->setRange(0.0, 500.0);
        ui->deltaPlot->clearGraphs();
        ui->deltaPlot->addGraph();

        ui->deltaPlot->graph()->setPen(QPen(Qt::black));
        ui->deltaPlot->graph()->setName("fft");
        // make left and bottom axes always transfer their ranges to right and top axes:
        connect(ui->deltaPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), ui->deltaPlot->xAxis2, SLOT(setRange(QCPRange)));
        connect(ui->deltaPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), ui->deltaPlot->yAxis2, SLOT(setRange(QCPRange)));

        frequency = ui->comboBox->currentIndex();
        double freqMult = SAMPLE_FREQ / BUFFER_SIZE;


        waveform = ui->comboBox->currentIndex();

        ps2000_set_sig_gen_built_in (unitOpened.handle,
                                     0,
                                     1000000, // 1 volt
                                     (PS2000_WAVE_TYPE) waveform,
                                     (float)frequency,
                                     (float)frequency,
                                     0,
                                     0,
                                     PS2000_UPDOWN, 0);

        collect_block_immediate(uselesstime, DATA, uselesschB);

        for(int i = 0; i < BUFFER_SIZE; i++)
        {
            mFftIn[i] = DATA[i];
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

        for(int j = 0; j < 1024; j++)
        {
           qDebug("%lf \n", mFftOut[j]);
        }
        ui->deltaPlot->graph(0)->setData(mFftIndices,fftVec);
        ui->deltaPlot->replot();
        ui->deltaPlot->graph(0)->rescaleAxes();
        // Allow user to drag axis ranges with mouse, zoom with mouse wheel and select graphs by clicking:
        ui->deltaPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);


        fftw_free(mFftIn);
        fftw_free(mFftOut);
        fftw_destroy_plan(mFftPlan);
}

