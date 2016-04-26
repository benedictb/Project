#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QShortcut>
#include <iostream>
#include "histstack.h"
#include "bookmarkdialog.h"
#include <iostream>

const QString PATH = "../Project/";

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    icognito = false;
    ui->setupUi(this);
    homepage = "http://nd.edu";

    this->setWindowTitle("Browser");
    currentTab = 0;
    QShortcut * quitShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this, SLOT(close()));
    QShortcut * refreshShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_R), this, SLOT(on_refreshButton_clicked()));
    QShortcut * addressShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_L), this,SLOT(addressBarHighlighter()));
    QShortcut * newTabShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_T), this, SLOT(newTab()));
    QShortcut * switchTabShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Tab),this,SLOT(nextTab()));
    QShortcut * deleteCurrentTabShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W),this,SLOT(deleteTab()));
    QShortcut * autoComplete = new QShortcut(QKeySequence(Qt::Key_Tab),this,SLOT(autoComplete()));
    QShortcut * backShortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Left),ui->backButton,SLOT(click()));
    QShortcut * forwardShortcut = new QShortcut(QKeySequence(Qt::ALT + Qt::Key_Right),ui->forwardButton,SLOT(click()));
    QShortcut * toggle_icognito = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_I),this,SLOT(toggle_icognito()));
    QShortcut * addBookmarkShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_D),this,SLOT(add_bookmark()));

    connect(ui->lineEdit, SIGNAL(returnPressed()),ui->goButton,SIGNAL(clicked()));
    ui->lineEdit->setText(homepage);
    ui->webView->load(ui->lineEdit->text());
    ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), ui->lineEdit->text()); // sets name of tab to website
    load_visited();
    load_bookmarks();
    webViews.push_back(ui->webView);

    HistStack newHist;
    newHist.add(homepage.toStdString());
    histories.push_back(newHist);

    connect(ui->webView,SIGNAL(loadFinished(bool)),this,SLOT(link_set_text(bool)));

}

MainWindow::~MainWindow()
{
    if (!icognito) {
        QFile file(PATH + "visited.txt");
        file.open(QIODevice::ReadWrite | QIODevice::Text);
        QTextStream stream(&file);
        for (it = visited.begin(); it != visited.end(); ++it)
           stream << QString::fromStdString(*it) << endl; // writes new visited file before it exits
        file.close();
    }
    delete ui;
}

void MainWindow::on_goButton_clicked()
{
    QString url = ui->lineEdit->text();
    std::string stdUrl = url.toStdString();
    std::size_t found = stdUrl.find("http://");

    if (found != 0) {
        url = "http://" + url;
    }

    webViews[ui->tabWidget->currentIndex()]->load(url);
    visited.insert(url.toStdString());

    histories[ui->tabWidget->currentIndex()].add(url.toStdString());

    ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), url); // sets name of tab to website

}

void MainWindow::on_backButton_clicked()
{
    int current = ui->tabWidget->currentIndex();
    if (histories[current].canGoBack()){
        QString url = QString::fromStdString(histories[current].backStep());
        ui->lineEdit->setText(url);
        webViews[current]->load(url);

    }


}

void MainWindow::on_forwardButton_clicked()
{
    int current = ui->tabWidget->currentIndex();
    if (histories[current].canGoForward()){
        QString url = QString::fromStdString(histories[current].forwardStep());
        ui->lineEdit->setText(url);
        webViews[current]->load(url);
    }

}

void MainWindow::on_refreshButton_clicked() {
    webViews[ui->tabWidget->currentIndex()]->load(QString::fromStdString(histories[ui->tabWidget->currentIndex()].getPresent()));
}

void MainWindow::on_bookmarkButton_clicked() {
    BookmarkDialog * dialog = new BookmarkDialog();
    dialog->show();
    connect(dialog,SIGNAL(loadBookmark(QString)),this,SLOT(load_bookmark(QString)));
}

void MainWindow::addressBarHighlighter() {
    ui->lineEdit->selectAll();
    ui->lineEdit->setFocus();
}

void MainWindow::newTab(QString str) {
    QWebView *newView = new QWebView(); // creates new tab
    HistStack newHist;
    newHist.add(homepage.toStdString());

    webViews.push_back(newView);
    histories.push_back(newHist);

    ui->tabWidget->addTab(newView, str);
    ui->tabWidget->setTabText(ui->tabWidget->count()-1,homepage);
    newView->load(homepage);

    connect(webViews[ui->tabWidget->count()-1],SIGNAL(loadFinished(bool)),this,SLOT(link_set_text(bool)));

}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    currentTab = index;
}

void MainWindow::nextTab(){
    if (ui->tabWidget->currentIndex() >= ui->tabWidget->count()-1){
        ui->tabWidget->setCurrentIndex(0);
    } else {
        ui->tabWidget->setCurrentIndex((ui->tabWidget->currentIndex() + 1));
    }

    ui->lineEdit->setText(QString::fromStdString(histories[ui->tabWidget->currentIndex()].getPresent()));
}

void MainWindow::deleteTab(){
    ui->tabWidget->removeTab(ui->tabWidget->currentIndex());
    webViews.erase(webViews.begin()+ui->tabWidget->currentIndex());
    histories.erase(histories.begin()+ui->tabWidget->currentIndex());
}

void MainWindow::autoComplete() {
    std::string current = ui->lineEdit->text().toStdString();
    for (it = visited.begin(); it != visited.end(); ++it)
        if (it->find(current) != std::string::npos)
            ui->lineEdit->setText(QString::fromStdString(*it));
}

void MainWindow::load_visited() {
    QFile file(PATH + "visited.txt");
    file.open(QIODevice::ReadWrite);
    if (!file.exists())
        ui->lineEdit->setText("No History Found");
    else {
        QTextStream in(&file);
        QString line = in.readLine();
        while (!in.atEnd())
            visited.insert(in.readLine().toStdString());
        file.close();
    }
}

void MainWindow::load_bookmarks(){
    QFile file(PATH + "bookmarks.txt");
    file.open(QIODevice::ReadWrite);
    if (!file.exists())
        ui->lineEdit->setText("No Bookmarks Found");
    else {
        QTextStream in(&file);
        QString line = in.readLine();
        while (!in.atEnd())
            bookmarks.push_back(in.readLine().toStdString());
        file.close();
    }
}

void MainWindow::add_bookmark(){
    QString bkmk = QString::fromStdString(histories[ui->tabWidget->currentIndex()].getPresent());
//    QString bkmk = ui->lineEdit->text();
    QFile file(PATH + "bookmarks.txt");
    file.open(QIODevice::Append | QIODevice::Text);
    QTextStream stream(&file);
    stream<<bkmk << endl;
    file.close();

}

void MainWindow::toggle_icognito() {
    if (icognito) {
        icognito = false;
        this->setWindowTitle("Browser");
    } else {
        icognito = true;
        this->setWindowTitle("Browser (Incognito)");
    }
}

void MainWindow::load_bookmark(QString url){
    webViews[ui->tabWidget->currentIndex()]->load(url);

    visited.insert(url.toStdString());
    ui->lineEdit->setText(url);
    histories[ui->tabWidget->currentIndex()].add(url.toStdString());

    ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), url); // sets name of tab to website
}

void MainWindow::link_set_text(bool OK){
    if (OK){
        ui->lineEdit->setText(webViews[ui->tabWidget->currentIndex()]->url().toString());
        ui->tabWidget->setTabText(ui->tabWidget->currentIndex(), webViews[ui->tabWidget->currentIndex()]->url().toString()); // sets name of tab to website

    }
}


