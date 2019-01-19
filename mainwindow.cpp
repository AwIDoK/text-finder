#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "text_finder.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QDirIterator>
#include <QtConcurrent>

main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry(this)));
    qRegisterMetaType<QFileInfoList>("QFileInfoList");
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    QCommonStyle style;
    ui->actionIndex_Directory->setIcon(style.standardIcon(QCommonStyle::SP_DialogOpenButton));
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionSearch->setIcon(style.standardIcon(QCommonStyle::SP_FileDialogContentsView));
    ui->actionCancel->setIcon(style.standardIcon(QCommonStyle::SP_DialogCancelButton));

    connect(ui->actionIndex_Directory, &QAction::triggered, this, &main_window::index_directory);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionSearch, &QAction::triggered, this, &main_window::search_text);
    connect(ui->actionCancel, &QAction::triggered, this, &main_window::finish_search);
    connect(ui->actionCancel, &QAction::triggered, &fnd, &text_finder::kill);

    connect(&fnd, &text_finder::progress_changed, this, &main_window::set_progress_bar);
    connect(&fnd, &text_finder::occurrence_finded, this, &main_window::add_occurrence);
    connect(&fnd, &text_finder::search_finished, this, &main_window::finish_search);
    connect(&fnd, &text_finder::indexing_finished, this, &main_window::finish_indexing);
    connect(this, &main_window::killed, &fnd, &text_finder::kill);
}

main_window::~main_window() {
    emit killed();
    future1.waitForFinished();
    future2.waitForFinished();
}

void main_window::index_directory() {
    current_dir = QFileDialog::getExistingDirectory(this, "Select Directory for Indexing");
    if (current_dir.size() == 0) {
        return;
    }
    ui->treeWidget->clear();
    ui->actionIndex_Directory->setEnabled(false);
    ui->actionSearch->setEnabled(false);
    ui->actionCancel->setEnabled(true);
    ui->progressBar->setValue(0);
    setWindowTitle(QString("Indexing Directory - %1").arg(current_dir));
    future1 = QtConcurrent::run(&fnd, &text_finder::index_directory, current_dir);
}

void main_window::search_text() {
    QString text = ui->lineEdit->text();
    if (text.size() < 5) {
        QMessageBox Msgbox;
        Msgbox.setText("Enter at least 5 symbols");
        Msgbox.exec();
    } else {
        ui->treeWidget->clear();
        ui->actionIndex_Directory->setEnabled(false);
        ui->actionSearch->setEnabled(false);
        ui->actionCancel->setEnabled(true);
        ui->progressBar->setValue(0);
        future2 = QtConcurrent::run(&fnd, &text_finder::find_text, text);
    }
}

void main_window::set_progress_bar(int progress) {
    ui->progressBar->setValue(progress);
}

void main_window::add_occurrence(QString const& file, QString const& text) {
    QDir d(current_dir);
    QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, file);
    item->setText(1, text);
    ui->treeWidget->addTopLevelItem(item);
}

void main_window::finish_indexing() {
    setWindowTitle(QString("Indexing of %1 was finished").arg(current_dir));
    ui->actionIndex_Directory->setEnabled(true);
    ui->actionSearch->setEnabled(true);
    ui->actionCancel->setEnabled(false);
}

void main_window::finish_search() {
    if (ui->treeWidget->topLevelItemCount() == 0) {
        QTreeWidgetItem* item = new QTreeWidgetItem(ui->treeWidget);
        item->setText(0, "Nothing was found");
        ui->treeWidget->addTopLevelItem(item);
    }
    setWindowTitle(QString("Occurrences in %1").arg(current_dir));
    ui->actionIndex_Directory->setEnabled(true);
    ui->actionSearch->setEnabled(true);
    ui->actionCancel->setEnabled(false);
}

