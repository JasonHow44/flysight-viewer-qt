#include "dataplot.h"
#include "mainwindow.h"

DataPlot::DataPlot(QWidget *parent) :
    QCustomPlot(parent),
    mMainWindow(0),
    m_dragging(false)
{
    setMouseTracking(true);
}

void DataPlot::mousePressEvent(
        QMouseEvent *event)
{
    if (axisRect()->rect().contains(event->pos()))
    {
        m_beginPos = event->pos();
        m_dragging = true;
        update();
    }

    QCustomPlot::mousePressEvent(event);
}

void DataPlot::mouseReleaseEvent(
        QMouseEvent *event)
{
    QPoint endPos = event->pos();

    MainWindow::Tool tool = mMainWindow->tool();
    if (m_dragging && tool == MainWindow::Zoom)
    {
        QCPRange range(qMin(xAxis->pixelToCoord(m_beginPos.x()),
                            xAxis->pixelToCoord(endPos.x())),
                       qMax(xAxis->pixelToCoord(m_beginPos.x()),
                            xAxis->pixelToCoord(endPos.x())));

        setRange(range);
    }
    if (m_dragging && tool == MainWindow::Zero)
    {
        DataPoint dpEnd = interpolateDataX(xAxis->pixelToCoord(endPos.x()));
        mMainWindow->setZero(dpEnd.t);
    }
    if (m_dragging && tool == MainWindow::Ground)
    {
        DataPoint dpEnd = interpolateDataX(xAxis->pixelToCoord(endPos.x()));
        mMainWindow->setGround(dpEnd.t);
    }

    if (m_dragging)
    {
        m_dragging = false;
        replot();
    }

    QCustomPlot::mouseReleaseEvent(event);
}

void DataPlot::mouseMoveEvent(
        QMouseEvent *event)
{
    m_cursorPos = event->pos();

    MainWindow::Tool tool = mMainWindow->tool();
    if (m_dragging && tool == MainWindow::Pan)
    {
        QCPRange range = xAxis->range();

        double diff = xAxis->pixelToCoord(m_beginPos.x())
                - xAxis->pixelToCoord(m_cursorPos.x());
        range = QCPRange(range.lower + diff, range.upper + diff);

        setRange(range);

        m_beginPos = m_cursorPos;
    }

    if (axisRect()->rect().contains(event->pos()))
    {
        if (m_dragging && tool == MainWindow::Measure)
        {
            DataPoint dpStart = interpolateDataX(xAxis->pixelToCoord(m_beginPos.x()));
            DataPoint dpEnd = interpolateDataX(xAxis->pixelToCoord(m_cursorPos.x()));
            mMainWindow->setMark(dpStart.t, dpEnd.t);
        }
        else
        {
            DataPoint dpEnd = interpolateDataX(xAxis->pixelToCoord(m_cursorPos.x()));
            mMainWindow->setMark(dpEnd.t);
        }
    }
    else
    {
        mMainWindow->clearMark();
    }

    update();

    QCustomPlot::mouseMoveEvent(event);
}

void DataPlot::wheelEvent(
        QWheelEvent *event)
{
    if (axisRect()->rect().contains(event->pos()))
    {
        double multiplier = exp((double) -event->angleDelta().y() / 500);

        double x = xAxis->pixelToCoord(event->pos().x());

        QCPRange range = xAxis->range();

        range = QCPRange(
                    x + (range.lower - x) * multiplier,
                    x + (range.upper - x) * multiplier);

        setRange(range);
    }
}

void DataPlot::leaveEvent(
        QEvent *)
{
    mMainWindow->clearMark();
    m_cursorPos = QPoint();
    update();
}

void DataPlot::paintEvent(
        QPaintEvent *event)
{
    QCustomPlot::paintEvent(event);

    MainWindow::Tool tool = mMainWindow->tool();
    if (m_dragging && (tool == MainWindow::Zoom || tool == MainWindow::Measure))
    {
        QPainter painter(this);

        painter.setPen(QPen(Qt::black));
        painter.drawLine(m_beginPos.x(), axisRect()->rect().top(), m_beginPos.x(), axisRect()->rect().bottom());
        if (axisRect()->rect().left() <= m_cursorPos.x() && m_cursorPos.x() <= axisRect()->rect().right())
        {
            painter.drawLine(m_cursorPos.x(), axisRect()->rect().top(), m_cursorPos.x(), axisRect()->rect().bottom());
        }

        QRect shading(
                    qMin(m_beginPos.x(), m_cursorPos.x()),
                    axisRect()->rect().top(),
                    qAbs(m_beginPos.x() - m_cursorPos.x()),
                    axisRect()->rect().height());

        painter.fillRect(shading & axisRect()->rect(), QColor(181, 217, 42, 64));
    }
    else
    {
        if (axisRect()->rect().contains(m_cursorPos))
        {
            QPainter painter(this);

            painter.setPen(QPen(Qt::black));
            painter.drawLine(m_cursorPos.x(), axisRect()->rect().top(), m_cursorPos.x(), axisRect()->rect().bottom());
            painter.drawLine(axisRect()->rect().left(), m_cursorPos.y(), axisRect()->rect().right(), m_cursorPos.y());
        }
    }
}

void DataPlot::setRange(
        const QCPRange &range)
{
    DataPoint dpLower = interpolateDataX(range.lower);
    DataPoint dpUpper = interpolateDataX(range.upper);

    mMainWindow->setRange(dpLower.t, dpUpper.t);
}

void DataPlot::setRange(
        double lower,
        double upper)
{
    DataPoint dpLower = mMainWindow->interpolateDataT(lower);
    DataPoint dpUpper = mMainWindow->interpolateDataT(upper);

    xAxis->setRange(QCPRange(mMainWindow->xValue()->value(dpLower, mMainWindow->units()),
                             mMainWindow->xValue()->value(dpUpper, mMainWindow->units())));

    updateYRanges();
}

void DataPlot::updateYRanges()
{
    const QCPRange &range = xAxis->range();

    int k = 0;
    for (int j = 0; j < MainWindow::yaLast; ++j)
    {
        if (!mMainWindow->yValue(j)->visible()) continue;

        double yMin, yMax;
        bool first = true;

        for (int i = 0; i < mMainWindow->dataSize(); ++i)
        {
            const DataPoint &dp = mMainWindow->dataPoint(i);

            if (range.contains(mMainWindow->xValue()->value(dp, mMainWindow->units())))
            {
                double y = mMainWindow->yValue(j)->value(dp, mMainWindow->units());

                if (first)
                {
                    yMin = yMax = y;
                    first = false;
                }
                else
                {
                    if (y < yMin) yMin = y;
                    if (y > yMax) yMax = y;
                }
            }
        }

        if (!first)
            axisRect()->axis(QCPAxis::atLeft, k++)->setRange(yMin, yMax);
    }

    replot();
}

void DataPlot::updatePlot()
{
    xAxis->setLabel(mMainWindow->xValue()->title(mMainWindow->units()));

    QVector< double > x;
    for (int i = 0; i < mMainWindow->dataSize(); ++i)
    {
        const DataPoint &dp = mMainWindow->dataPoint(i);
        x.append(mMainWindow->xValue()->value(dp, mMainWindow->units()));
    }

    clearPlottables();
    while (axisRect()->axisCount(QCPAxis::atLeft) > 0)
    {
        axisRect()->removeAxis(axisRect()->axis(QCPAxis::atLeft, 0));
    }

    for (int j = 0; j < MainWindow::yaLast; ++j)
    {
        if (!mMainWindow->yValue(j)->visible()) continue;

        QVector< double > y;
        for (int i = 0; i < mMainWindow->dataSize(); ++i)
        {
            const DataPoint &dp = mMainWindow->dataPoint(i);
            y.append(mMainWindow->yValue(j)->value(dp, mMainWindow->units()));
        }

        QCPGraph *graph = addGraph(
                    axisRect()->axis(QCPAxis::atBottom),
                    mMainWindow->yValue(j)->addAxis(this, mMainWindow->units()));
        graph->setData(x, y);
        graph->setPen(QPen(mMainWindow->yValue(j)->color()));
    }

    if (mMainWindow->markActive())
    {
        const DataPoint &dpEnd = mMainWindow->interpolateDataT(mMainWindow->markEnd());

        QVector< double > xMark, yMark;

        xMark.append(mMainWindow->xValue()->value(dpEnd, mMainWindow->units()));

        int k = 0;
        for (int j = 0; j < MainWindow::yaLast; ++j)
        {
            if (!mMainWindow->yValue(j)->visible()) continue;

            yMark.clear();
            yMark.append(mMainWindow->yValue(j)->value(dpEnd, mMainWindow->units()));

            QCPGraph *graph = addGraph(
                        xAxis,
                        axisRect()->axis(QCPAxis::atLeft, k++));

            graph->setData(xMark, yMark);
            graph->setPen(QPen(Qt::black));
            graph->setLineStyle(QCPGraph::lsNone);
            graph->setScatterStyle(QCPScatterStyle::ssDisc);
        }
    }

    updateYRanges();
}

DataPoint DataPlot::interpolateDataX(
        double x)
{
    const int i1 = findIndexBelowX(x);
    const int i2 = findIndexAboveX(x);

    if (i1 < 0)
    {
        return mMainWindow->dataPoint(0);
    }
    else if (i2 >= mMainWindow->dataSize())
    {
        return mMainWindow->dataPoint(mMainWindow->dataSize() - 1);
    }
    else
    {
        const DataPoint &dp1 = mMainWindow->dataPoint(i1);
        const DataPoint &dp2 = mMainWindow->dataPoint(i2);
        const double x1 = mMainWindow->xValue()->value(dp1, mMainWindow->units());
        const double x2 = mMainWindow->xValue()->value(dp2, mMainWindow->units());
        return interpolate(dp1, dp2, (x - x1) / (x2 - x1));
    }
}

int DataPlot::findIndexBelowX(
        double x)
{
    for (int i = 0; i < mMainWindow->dataSize(); ++i)
    {
        const DataPoint &dp = mMainWindow->dataPoint(i);
        if (mMainWindow->xValue()->value(dp, mMainWindow->units()) > x)
            return i - 1;
    }

    return mMainWindow->dataSize() - 1;
}

int DataPlot::findIndexAboveX(
        double x)
{
    for (int i = 0; i < mMainWindow->dataSize(); ++i)
    {
        const DataPoint &dp = mMainWindow->dataPoint(i);
        if (mMainWindow->xValue()->value(dp, mMainWindow->units()) <= x)
            return i + 1;
    }

    return 0;
}
