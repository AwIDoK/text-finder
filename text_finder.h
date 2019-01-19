#ifndef FINDER_H
#define FINDER_H
#include <QSet>
#include <QPair>
#include <QList>
#include <QFileInfoList>

class text_finder : public QObject {
    Q_OBJECT

signals:
    void progress_changed(int progress);
    void occurrence_finded(QString const& file, QString const& text);
    void search_finished();
    void indexing_finished();

public slots:
    void kill();

private:
    QList<QPair<QSet<QString>, QFileInfo>> all_trigrams;
    bool alive;
    void add_file_info(QFileInfo const& file_info);
    void search_in_file(QFileInfo const& file_info, QString const& text);
public:
    void find_text(QString const& text);
    void index_directory(QString dir);

};

#endif // FINDER_H
