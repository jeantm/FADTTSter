#include "Plot.h"
#include <QDebug>

/***************************************************************/
/********************** Public functions ***********************/
/***************************************************************/
Plot::Plot( QObject *parent ) :
    QObject( parent )
{
    m_chart = vtkSmartPointer<vtkChartXY>::New();
}


void Plot::SetQVTKWidget( QVTKWidget *qvtkWidget )
{
    m_qvtkWidget = qvtkWidget;
    m_view = vtkSmartPointer<vtkContextView>::New();
    m_view->SetInteractor( m_qvtkWidget->GetInteractor() );
    m_table = vtkSmartPointer<vtkTable>::New();
    m_qvtkWidget->SetRenderWindow( m_view->GetRenderWindow() );
}

void Plot::SetDirectory( QString directory )
{
    m_directory = directory;
    m_matlabOutputDir = directory + "/MatlabOutputs";

    SetRawDataFiles();
    SetOmnibusFiles();
    SetPostHocFiles();
}

void Plot::SelectPlot( QString plotSelected )
{
    m_plotSelected = plotSelected;
}

void Plot::SelectOutcome( QString outcome )
{
    m_outcomeSelected = outcome;
}

void Plot::SelectCovariate( QString covariateSelected )
{
    m_covariateSelected = covariateSelected;
}

void Plot::SetTitle( QString title )
{
    m_title = title;
    qDebug() << "title: " << title;
}

void Plot::SetxName( QString xName )
{
    m_xName = xName;
    qDebug() << "xName: " << xName;
}

void Plot::SetyName( QString yName )
{
    m_yName = yName;
    qDebug() << "yName: " << yName;
}

void Plot::SetYMin( bool yMinChecked, double yMin )
{
    m_yMinChecked = yMinChecked;
    m_yMinGiven = yMin;

    qDebug() << "yMin: " << yMin;
}

void Plot::SetYMax( bool yMaxChecked, double yMax )
{
    m_yMaxChecked = yMaxChecked;
    m_yMaxGiven = yMax;

    qDebug() << "yMax: " << yMax;
}

void Plot::ResetDataFile()
{
    m_csvRawDataFile.clear();
    m_csvBetas.clear();
    m_csvOmnibus.clear();
    m_csvPostHoc.clear();
}


void Plot::DisplayVTKPlot()
{
    // Resetting scene
    m_view->GetScene()->ClearItems();
    m_view->GetScene()->AddItem( m_chart );

    // Loading data
    ResetLoadData();
    LoadAbscissa();
    bool dataAvailable = LoadData();

    if( dataAvailable )
    {
        qDebug() << "m_abscissa.size: " << m_abscissa.size();
        qDebug() << "m_ordinate.size: " << m_ordinate.size() << " x " << m_ordinate.first().size();
        qDebug() << "m_nbrPlot: " << m_nbrPlot;

        // Creating a table with 2 named arrays
        AddAxis();

        // Adding data
        AddData();

        // Adding plots

        float red = 0;
        float green = 0;
        float blue = 255;
        float opacity = 255;
        AddPlots( red, green, blue, opacity );

        // Setting chart properties
        SetChartProperties();
    }
}

void Plot::ResetPlot()
{
    m_chart->ClearPlots();
}


/***************************************************************/
/************************ Public  slots ************************/
/***************************************************************/
void Plot::SavePlot()
{
//    QRect rectangle;
//    QPixmap pixmap( rectangle.size() );
//    QRect rectangle;
//    QPixmap pixmap;
//    m_qvtkWidget->render( &pixmap, QPoint(), QRegion( rectangle ) );
//    pixmap.save( m_matlabOutputDir + "/example.png" );
}


/***************************************************************/
/********************** Private functions **********************/
/***************************************************************/
void Plot::ProcessCovariates()
{
    QStringList firstRaw = m_csvRawData.value( 4 ).first();
    QStringList firstDataRaw = m_csvRawData.value( 4 ).at( 1 );
    QStringList covariates;

    for( int i = 1; i < firstRaw.size(); i++ )
    {
        QPair< int, QString > currentPair;
        currentPair.first = i ;
        currentPair.second = firstRaw.at( i );
        bool currentBool = ( firstDataRaw .at( i ).toInt() == 0 ) | ( firstDataRaw.at( i ).toInt() == 1 );
        m_covariates.insert( currentPair, currentBool );

        covariates.append( currentPair.second );
    }
    emit CovariateUsed( covariates );
}

void Plot::SetRawData()
{
    foreach( QString path, m_csvRawDataFile )
    {
        QString currentPath = m_directory + "/" + path;
        int index;
        if( path.contains( "_ad_", Qt::CaseInsensitive ) )
        {
            index = 0;
        }
        if( path.contains( "_md_", Qt::CaseInsensitive ) )
        {
            index = 1;
        }
        if( path.contains( "_rd_", Qt::CaseInsensitive ) )
        {
            index = 2;
        }
        if( path.contains( "_fa_", Qt::CaseInsensitive ) )
        {
            index = 3;
        }
        if( path.contains( "_comp_", Qt::CaseInsensitive ) )
        {
            index = 4;
        }
        m_csvRawData.insert( index, m_process.GetDataFromFile( currentPath ) );
    }
    ProcessCovariates();
}

void Plot::SetRawDataFiles()
{
    QStringList nameFilterRawData( "*RawData*.csv" );
    m_csvRawDataFile = QDir( m_directory ).entryList( nameFilterRawData );
    SetRawData();
}

void Plot::SetOmnibusFiles()
{
    QStringList nameFilterOmnibus( "*Omnibus*.csv" );
    m_csvOmnibus = QDir( m_matlabOutputDir ).entryList( nameFilterOmnibus );
}

void Plot::SetPostHocFiles()
{
    QStringList nameFilterPostHoc( "*PostHoc*.csv" );
    m_csvPostHoc = QDir( m_matlabOutputDir ).entryList( nameFilterPostHoc );
}


void Plot::ResetLoadData()
{
    m_abscissa.clear();
    m_ordinate.clear();
    m_axis.clear();
    m_plot.clear();
    m_table->SetNumberOfRows( 0 );
    m_nbrPlot = 0;
    m_yMin = 0;
    m_yMax = 0;
}

void Plot::IsCovariateBinary()
{
    m_isCovariateBinary = false;
    m_indexColumn = 0;
    QMap< QPair< int, QString >, bool >::ConstIterator iterCovariates = m_covariates.begin();
    while( iterCovariates != m_covariates.end() )
    {
        if( iterCovariates.key().second == m_covariateSelected )
        {
            m_isCovariateBinary = iterCovariates.value();
            m_indexColumn = iterCovariates.key().first;
        }
        ++iterCovariates;
    }
}


void Plot::FindyMinMax()
{
    foreach( QList < double > rowData, m_ordinate )
    {
        foreach( double data, rowData )
        {
            if( m_yMin == 0 )
            {
                m_yMin = data;
            }
            else
            {
                if( data < m_yMin )
                {
                    m_yMin = data;
                }
            }
            if( m_yMax == 0 )
            {
                m_yMax = data;
            }
            else
            {
                if( data > m_yMax )
                {
                    m_yMax = data;
                }
            }
        }
    }
    qDebug() << "m_yMin: " << m_yMin;
    qDebug() << "m_yMax: " << m_yMax;
}

void Plot::LoadAbscissa()
{
    QList<QStringList> data = m_process.GetDataFromFile( m_directory + "/" + m_csvRawDataFile.first() );
    data.removeFirst();
    foreach( QStringList rowData, data )
    {
        m_abscissa.append( rowData.first().toDouble() );
    }
    m_nbrPoint = m_abscissa.size();
}

void Plot::LoadRawData()
{
    QStringList rawDataFile = m_csvRawDataFile.filter( "_" + m_outcomeSelected + "_", Qt::CaseInsensitive );
    if( !rawDataFile.isEmpty() )
    {
        QList<QStringList> data = m_process.GetDataFromFile( m_directory + "/" + rawDataFile.first() );
        data.removeFirst();
        foreach( QStringList rowData, data )
        {
            rowData.removeFirst();
            m_ordinate.append( DataToDouble( rowData ) );
        }
    }
}

QList < double > Plot::DataToDouble( QStringList rowData )
{
    QList < double > newRowData;
    foreach ( QString data, rowData )
    {
        newRowData.append( data.toDouble() );
    }
    return newRowData;
}

void Plot::SeparateData( QList< QList < double > > &temp0Bin, QList< QList < double > > &temp1Bin )
{
    for( int i = 0; i < m_ordinate.size(); i++ )
    {
        QList < double > temp0Subj;
        QList < double > temp1Subj;
        for( int j = 0; j < m_ordinate.first().size(); j++ )
        {
            if( m_isCovariateBinary && ( m_csvRawData.value( 4 ).at( j ).at( m_indexColumn ).toInt() == 0 ) )
            {
                temp0Subj.append( m_ordinate.at( i ).at( j ) );
            }
            else
            {
                temp1Subj.append( m_ordinate.at( i ).at( j ) );
            }
        }
        if( !temp0Subj.isEmpty() )
        {
            temp0Bin.append( temp0Subj );
        }
        if( !temp1Subj.isEmpty() )
        {
            temp1Bin.append( temp1Subj );
        }
    }
}

void Plot::GetMeanAndStdDv( QList< QList < double > > tempBin, QList < double > &tempMean, QList < double > &tempStdDv )
{
    int nbrSubjects = tempBin.first().size();
    for( int i = 0; i < m_nbrPoint; i++ )
    {
        float currentMean = 0;
        for( int j = 0; j < nbrSubjects; j++ )
        {
            currentMean += tempBin.at( i ).at( j );
        }
        currentMean = currentMean / nbrSubjects;
        tempMean.append( currentMean );

        float currentStdDv = 0;
        for( int j = 0; j < nbrSubjects; j++ )
        {
            currentStdDv += std::sqrt( std::pow( currentMean - tempBin.at( i ).at( j ), 2 ) );
        }
        tempStdDv.append( currentStdDv / nbrSubjects );
    }
}

void Plot::ProcessRawStats( QList< QList < double > > tempBin, QList < double > &tempBinMean, QList < double > &tempBinUp, QList < double > &tempBinDown )
{
    QList < double > tempBinStdDv;
    if( !tempBin.isEmpty() )
    {
        GetMeanAndStdDv( tempBin, tempBinMean, tempBinStdDv );
        for( int i = 0; i < tempBin.size(); i++ )
        {
            tempBinUp.append( tempBinMean.at( i ) + tempBinStdDv.at( i ) );
            tempBinDown.append( tempBinMean.at( i ) - tempBinStdDv.at( i ) );
        }
    }
}

void Plot::SetRawStatsData( QList < double > &temp0BinUp, QList < double > &temp0BinMean, QList < double > &temp0BinDown,
                            QList < double > &temp1BinUp, QList < double > &temp1BinMean, QList < double > &temp1BinDown )
{
    m_ordinate.clear();
    for( int i = 0; i < m_nbrPoint; i++ )
    {
        QList < double > row;
        if( !temp0BinUp.isEmpty() )
        {
            row.append( temp0BinUp.at( i ) );
            row.append( temp0BinMean.at( i ) );
            row.append( temp0BinDown.at( i ) );
        }
        row.append( temp1BinUp.at( i ) );
        row.append( temp1BinMean.at( i ) );
        row.append( temp1BinDown.at( i ) );
        m_ordinate.append( row );
    }
}

void Plot::LoadRawStats()
{
    if( !m_ordinate.isEmpty() )
    {
        QList< QList < double > > temp0Bin;
        QList< QList < double > > temp1Bin;
        SeparateData( temp0Bin, temp1Bin );

        QList < double > temp0BinUp;
        QList < double > temp0BinMean;
        QList < double > temp0BinDown;
        ProcessRawStats( temp0Bin, temp0BinMean, temp0BinUp, temp0BinDown );

        QList < double > temp1BinUp;
        QList < double > temp1BinMean;
        QList < double > temp1BinDown;
        ProcessRawStats( temp1Bin, temp1BinMean, temp1BinUp, temp1BinDown );

        SetRawStatsData( temp0BinUp, temp0BinMean, temp0BinDown, temp1BinUp, temp1BinMean, temp1BinDown );
    }
}

bool Plot::LoadData()
{
    if( ( m_plotSelected == "Raw Data" ) || ( m_plotSelected == "Raw Stats" ) )
    {
        if( !m_outcomeSelected.isEmpty() )
        {
            IsCovariateBinary();
            LoadRawData();

            if( m_plotSelected == "Raw Stats" )
            {
                LoadRawStats();
            }
            m_nbrPlot = m_ordinate.first().size();
        }
        FindyMinMax();
        return !( m_abscissa.isEmpty() | m_ordinate.isEmpty() );
    }
    if( m_plotSelected == "Omnibus" )
    {
//        QString AD_MatlabInputFile = "/work/jeantm/Project/FADTTS_Origin/DataShaili/CT_L_Parietal-AllOutput/ad_CT_L_Parietal.txt";
//        QList<QStringList> AD_MatlabInputData = m_process.GetDataFromFile( AD_MatlabInputFile );
//        foreach( QStringList rowData, AD_MatlabInputData )
//        {
//            m_abscissa.append( rowData.first().toFloat() );
//        }

//        QString AD_BetasFile = "/work/jeantm/Project/FADTTS_Origin/DataShaili/CT_L_Parietal-AllOutput/CT-L-Parietal_AD_betas.csv";
//        ordinate = m_process.GetDataFromFile( AD_BetasFile );
        return false;
    }
    if( m_plotSelected == "Post-Hoc" )
    {
//        QString AD_MatlabInputFile = "/work/jeantm/Project/FADTTS_Origin/DataShaili/CT_L_Parietal-AllOutput/ad_CT_L_Parietal.txt";
//        QList<QStringList> AD_MatlabInputData = m_process.GetDataFromFile( AD_MatlabInputFile );
//        foreach( QStringList rowData, AD_MatlabInputData )
//        {
//            m_abscissa.append( rowData.first().toFloat() );
//        }

//        QString AD_BetasFile = "/work/jeantm/Project/FADTTS_Origin/DataShaili/CT_L_Parietal-AllOutput/CT-L-Parietal_AD_betas.csv";
//        ordinate = m_process.GetDataFromFile( AD_BetasFile );
        return false;
    }
    return false;
}


void Plot::AddAxis()
{
    QPair< int, QString > abscissaPair;
    abscissaPair.first = 0;
    abscissaPair.second = "Arclength";
    m_axis.insert( abscissaPair, vtkSmartPointer<vtkFloatArray>::New() );

    if( m_plotSelected == "Raw Data" )
    {
        for( int i = 0; i < m_nbrPlot; i++ )
        {
            QPair< int, QString > currentPair;
            currentPair.first = i + 1;
            currentPair.second = "Subject " + QString::number( i + 1 );
            m_axis.insert( currentPair, vtkSmartPointer<vtkFloatArray>::New() );
        }
    }
    if( m_plotSelected == "Raw Stats" )
    {
        if( m_nbrPlot == 6 )
        {
            QPair< int, QString > up0Pair;
            up0Pair.first = 1;
            up0Pair.second = m_covariateSelected + " up : 0";
            m_axis.insert( up0Pair, vtkSmartPointer<vtkFloatArray>::New() );

            QPair< int, QString > mean0Pair;
            mean0Pair.first = 2;
            mean0Pair.second = m_covariateSelected + " mean : 0";
            m_axis.insert( mean0Pair, vtkSmartPointer<vtkFloatArray>::New() );

            QPair< int, QString > down0Pair;
            down0Pair.first = 3;
            down0Pair.second = m_covariateSelected + " down : 0";
            m_axis.insert( down0Pair, vtkSmartPointer<vtkFloatArray>::New() );

            QPair< int, QString > up1Pair;
            up1Pair.first = 4;
            up1Pair.second = m_covariateSelected + " up : 1";
            m_axis.insert( up1Pair, vtkSmartPointer<vtkFloatArray>::New() );

            QPair< int, QString > mean1Pair;
            mean1Pair.first = 5;
            mean1Pair.second = m_covariateSelected + " mean : 1";
            m_axis.insert( mean1Pair, vtkSmartPointer<vtkFloatArray>::New() );

            QPair< int, QString > down1Pair;
            down1Pair.first = 6;
            down1Pair.second = m_covariateSelected + " down : 1";
            m_axis.insert( down1Pair, vtkSmartPointer<vtkFloatArray>::New() );
        }
        else
        {
            QPair< int, QString > upPair;
            upPair.first = 1;
            upPair.second = m_covariateSelected + " up";
            m_axis.insert( upPair, vtkSmartPointer<vtkFloatArray>::New() );

            QPair< int, QString > meanPair;
            meanPair.first = 2;
            meanPair.second = m_covariateSelected + " mean";
            m_axis.insert( meanPair, vtkSmartPointer<vtkFloatArray>::New() );

            QPair< int, QString > downPair;
            downPair.first = 3;
            downPair.second = m_covariateSelected + " down";
            m_axis.insert( downPair, vtkSmartPointer<vtkFloatArray>::New() );
        }
    }
    if( m_plotSelected == "Omnibus" )
    {
        //    m_axis.insert( "Arclength", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "Intercept", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "SSRIExposure", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "Sex", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "GestAgeBirth", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "DaysSinceBirth", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "DTIDirection", vtkSmartPointer<vtkFloatArray>::New() );
    }
    if( m_plotSelected == "Post-Hoc" )
    {
        //    m_axis.insert( "Arclength", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "Intercept", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "SSRIExposure", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "Sex", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "GestAgeBirth", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "DaysSinceBirth", vtkSmartPointer<vtkFloatArray>::New() );
        //    m_axis.insert( "DTIDirection", vtkSmartPointer<vtkFloatArray>::New() );
    }

    QMap< QPair< int, QString >, vtkSmartPointer<vtkFloatArray> >::ConstIterator iterAxis = m_axis.begin();
    while( iterAxis != m_axis.end() )
    {
        iterAxis.value()->SetName( iterAxis.key().second.toUtf8().constData() );
        m_table->AddColumn( iterAxis.value().GetPointer() );
        ++iterAxis;
    }
}

void Plot::AddData()
{
    m_table->SetNumberOfRows( m_nbrPoint );
    for( int i = 0; i < m_nbrPoint; i++ )
    {
        m_table->SetValue( i, 0, m_abscissa.at( i ) );
//        m_table->SetValue( 0, i, m_abscissa.at( i ) );
        for( int j = 0; j < m_nbrPlot; j++ )
        {
            m_table->SetValue( i, j + 1, m_ordinate.at( i ).at( j ) );
        }
    }
}

void Plot::AddPlots( float red, float green, float blue, float opacity )
{
    if( m_plotSelected == "Raw Data" )
    {
        for( int i = 0; i < m_nbrPlot; i++ )
        {
            QPair< int, QString > currentPair;
            currentPair.first = i;
            currentPair.second = "Subject " + QString::number( i + 1 );
            m_plot.insert( currentPair, m_chart->AddPlot( vtkChart::LINE ) );
        }

        QMap< QPair< int, QString >, vtkPlot* >::ConstIterator iterPlot = m_plot.begin();
        int i = 1;
        while( iterPlot != m_plot.end() )
        {
            iterPlot.value()->SetInputData( m_table.GetPointer(), 0, i );

            if( m_isCovariateBinary && ( m_csvRawData.value( 4 ).at( i ).at( m_indexColumn ).toInt() == 0 ) )
            {
                iterPlot.value()->SetColor( 0, 0, 255, 255 );
            }
            else
            {
                iterPlot.value()->SetColor( 255, 0, 0, 255 );
            }
            iterPlot.value()->SetWidth( 1.0 );
            ++iterPlot;
            i++;
        }
    }
    if( m_plotSelected == "Raw Stats" )
    {
        if( m_nbrPlot == 6 )
        {
            QPair< int, QString > up0Pair;
            up0Pair.first = 0;
            up0Pair.second = m_covariateSelected + " up : 0";
            m_plot.insert( up0Pair, m_chart->AddPlot( vtkChart::LINE ) );

            QPair< int, QString > mean0Pair;
            mean0Pair.first = 1;
            mean0Pair.second = m_covariateSelected + " mean : 0";
            m_plot.insert( mean0Pair, m_chart->AddPlot( vtkChart::LINE ) );

            QPair< int, QString > down0Pair;
            down0Pair.first = 2;
            down0Pair.second = m_covariateSelected + " down : 0";
            m_plot.insert( down0Pair, m_chart->AddPlot( vtkChart::LINE ) );

            QPair< int, QString > up1Pair;
            up1Pair.first = 3;
            up1Pair.second = m_covariateSelected + " up : 1";
            m_plot.insert( up1Pair, m_chart->AddPlot( vtkChart::LINE ) );

            QPair< int, QString > mean1Pair;
            mean1Pair.first = 4;
            mean1Pair.second = m_covariateSelected + " mean : 1";
            m_plot.insert( mean1Pair, m_chart->AddPlot( vtkChart::LINE ) );

            QPair< int, QString > down1Pair;
            down1Pair.first = 5;
            down1Pair.second = m_covariateSelected + " down : 1";
            m_plot.insert( down1Pair, m_chart->AddPlot( vtkChart::LINE ) );
        }
        else
        {
            QPair< int, QString > upPair;
            upPair.first = 1;
            upPair.second = m_covariateSelected + " up";
            m_plot.insert( upPair, m_chart->AddPlot( vtkChart::LINE ) );

            QPair< int, QString > meanPair;
            meanPair.first = 2;
            meanPair.second = m_covariateSelected + " mean";
            m_plot.insert( meanPair, m_chart->AddPlot( vtkChart::LINE ) );

            QPair< int, QString > downPair;
            downPair.first = 3;
            downPair.second = m_covariateSelected + " down";
            m_plot.insert( downPair, m_chart->AddPlot( vtkChart::LINE ) );
        }

        QMap< QPair< int, QString >, vtkPlot* >::ConstIterator iterPlot = m_plot.begin();
        int i = 1;
        while( iterPlot != m_plot.end() )
        {
            iterPlot.value()->SetInputData( m_table.GetPointer(), 0, i );
            if( !( i == 2 || i == 5 ) )
            {
                iterPlot.value()->GetPen()->SetLineType(vtkPen::DASH_LINE);
            }

            if( i < 4 )
            {
                iterPlot.value()->SetColor( 0, 0, 255, 255 );
//                vtkPlotPoints::SafeDownCast( iterPlot.value() )->SetMarkerStyle( vtkPlotPoints::CERCLE );
            }
            else
            {
                iterPlot.value()->SetColor( 255, 0, 0, 255 );
            }
            iterPlot.value()->SetWidth( 1.0 );
            ++iterPlot;
            i++;
        }
    }
    if( m_plotSelected == "Omnibus" )
    {

    }
    if( m_plotSelected == "Post-Hoc" )
    {

    }

//    m_plot.insert( "Intercept", m_chart->AddPlot( vtkChart::LINE ) );
//    m_plot.insert( "SSRIExposure", m_chart->AddPlot( vtkChart::LINE ) );
//    m_plot.insert( "Sex", m_chart->AddPlot( vtkChart::LINE ) );
//    m_plot.insert( "GestAgeBirth", m_chart->AddPlot( vtkChart::LINE ) );
//    m_plot.insert( "DaysSinceBirth", m_chart->AddPlot( vtkChart::LINE ) );
//    m_plot.insert( "DTIDirection", m_chart->AddPlot( vtkChart::LINE ) );
//    QMap< QString, vtkPlot* >::ConstIterator iterPlot = m_plot.begin();
//    int i = 1;
//    while( iterPlot != m_plot.end() )
//    {
//        iterPlot.value()->SetInputData( table.GetPointer(), 0, i );
//        vtkPlotPoints::SafeDownCast( iterPlot.value() )->SetMarkerStyle( vtkPlotPoints::CIRCLE );
//        iterPlot.value()->SetColor( 255*0.3*i, 255*0.2*i, 0, 255 );
//        iterPlot.value()->SetWidth( 1.0 );
//        ++iterPlot;
//        i++;
//    }
}

void Plot::SetChartProperties()
{
    if( m_title.isEmpty() )
    {
        m_title = "Title";
    }
    if( m_xName.isEmpty() )
    {
        m_xName = "Arc Length";
    }
    if( m_yName.isEmpty() )
    {
        m_yName = "yName";
    }
    if( m_yMinChecked )
    {
        m_yMin = m_yMinGiven;
    }
    if( m_yMaxChecked )
    {
        m_yMax = m_yMaxGiven;
    }
    qDebug() << "m_yMin: " << m_yMin << " | m_yMax: " << m_yMax ;


    m_chart->GetAxis( vtkAxis::BOTTOM )->SetTitle( m_xName.toStdString() );
    m_chart->GetAxis( vtkAxis::BOTTOM )->SetRange( m_abscissa.first(), m_abscissa.last() );
    m_chart->GetAxis( vtkAxis::BOTTOM )->SetMinimumLimit( std::floor( m_abscissa.first() ) );
    m_chart->GetAxis( vtkAxis::BOTTOM )->SetMaximumLimit( std::ceil( m_abscissa.last() ) );
//    int intTempo = m_chart->GetAxis( vtkAxis::BOTTOM )->GetNumberOfTicks();
//    qDebug() << "intTempo: " << intTempo;
//    m_chart->GetAxis( vtkAxis::BOTTOM )->SetNumberOfTicks( intTempo + 1 );
    m_chart->GetAxis( vtkAxis::BOTTOM )->GetGridPen()->SetColor( 80, 80, 80, 255 );

    m_chart->GetAxis( vtkAxis::LEFT )->SetTitle( m_yName.toStdString() );
    m_chart->GetAxis( vtkAxis::LEFT )->SetRange( m_yMin, m_yMax );
    m_chart->GetAxis( vtkAxis::LEFT )->SetMinimumLimit( m_yMin );
    m_chart->GetAxis( vtkAxis::LEFT )->SetMaximumLimit( m_yMax );
    m_chart->GetAxis( vtkAxis::LEFT )->GetGridPen()->SetColor( 80, 80, 80, 255 );

//    m_chart->ForceAxesToBoundsOn();

    m_chart->SetTitle( m_title.toStdString() );
    m_chart->GetTitleProperties()->SetBold( 1 );

//    m_chart->SetShowLegend( true );
}
