#include "LogbookView.h"
#include "ui_LogbookView.h"

#include <QMessageBox>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "mainwindow.h"

LogbookView::LogbookView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogbookView),
    mMainWindow(0)
{
    ui->setupUi(this);

    QSqlDatabase db = QSqlDatabase::database("flysight");
    QSqlQuery query(db);

    if (!query.exec("select * from jump"))
    {
        QSqlError err = query.lastError();
        QMessageBox::critical(0, tr("Query failed"), err.text());
    }

    ui->tableWidget->setColumnCount(query.record().count());

    int index = 0;
    while (query.next())
    {
        ui->tableWidget->insertRow(index);
        for (int j = 0; j < query.record().count(); ++j)
        {
            ui->tableWidget->setItem(index, j, new QTableWidgetItem(query.value(j).toString()));
        }
        ++index;
    }
}

LogbookView::~LogbookView()
{
    delete ui;
}

void LogbookView::setMainWindow(
        MainWindow *mainWindow)
{
    mMainWindow = mainWindow;
}
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
