#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QFileInfoList>
#include <QMainWindow>
#include <memory>
#include <text_finder.h>
#include <QFuture>
#include <QFileSystemWatcher>

namespace Ui {
class MainWindow;
}

class main_window : public QMainWindow {
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window();

signals:
    void killed();

private slots:
    void index_directory();
    void search_text();
    void set_progress_bar(int progress);
    void add_occurrence(QString const& file, QList<QString> const& occurences);
    void finish_search();
    void finish_indexing();
private:
    std::unique_ptr<Ui::MainWindow> ui;
    QString current_dir;
    text_finder fnd;
    QFileSystemWatcher watcher;
    QFuture<void> future1, future2;
};

#endif // MAINWINDOW_H
