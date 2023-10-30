#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ps2000Con.h"



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
}

