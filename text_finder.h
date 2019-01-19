#ifndef FINDER_H
#define FINDER_H
#include <QSet>
#include <QPair>
#include <QList>
#include <QFileInfoList>
#include <QMutex>
#include <QFileSystemWatcher>

class text_finder : public QObject {
    Q_OBJECT

signals:
    void progress_changed(int progress);
    void occurrence_finded(QString const& file, QList<QString> const& occurences);
    void search_finished();
    void indexing_finished();

public slots:
    void kill();
    void change_file(QString dir);

private:
    QList<QPair<QSet<QString>, QFileInfo>> all_trigrams;
    QFileSystemWatcher watcher;
    std::atomic_bool alive;
    void add_file_info(QFileInfo const& file_info);
    void search_in_file(QFileInfo const& file_info, QString const& text);
    QMutex mutex;
    QString current_dir;
public:
    void find_text(QString const& text);
    void index_directory(QString dir);
    text_finder();

};

#endif // FINDER_H
