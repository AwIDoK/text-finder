#include "text_finder.h"
#include <QtDebug>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QQueue>
#include <QtDebug>
#include <QtConcurrent>


text_finder::text_finder() : mutex(QMutex::Recursive) {
    connect(&watcher, &QFileSystemWatcher::directoryChanged, this, &text_finder::change_file);
}

void text_finder::find_text(QString const& text) {
    alive = true;
    if (!alive) {
        return;
    }
    if (all_trigrams.empty()) {
        emit progress_changed(100);
        emit search_finished();
        return;
    }
    QVector<qint64> trigrams;
    QChar trigram[3];
    for (int i = 0; i + 2 < text.size(); i++) {
        trigram[0] = text[i];
        trigram[1] = text[i + 1];
        trigram[2] = text[i + 2];
        trigrams.push_back(get_trigram_code(trigram));
    }
    qint64 cnt = 0;
    for (auto const& file_info : all_trigrams) {
        if (!alive) {
            break;
        }
        bool flag = true;
        for (auto const& trigram : trigrams) {
            if (!std::binary_search(file_info.first.begin(), file_info.first.end(),trigram)) {
                flag = false;
                break;
            }
        }
        if (flag) {
            search_in_file(file_info.second, text);
        }
        emit progress_changed(static_cast<int>(100 * (++cnt) / all_trigrams.size()));
    }
    emit search_finished();
}

void text_finder::index_directory(QString dir) {
    QMutexLocker locker(&mutex);
    watcher.directories().clear();
    current_dir = dir;
    alive = true;
    all_trigrams.clear();
    QDirIterator it(dir, QStringList() << "*", QDir::Files | QDir::Dirs, QDirIterator::Subdirectories);
    QFileInfoList all;
    while (it.hasNext()) {
        if (!alive) {
            break;
        }
        it.next();
        if (it.fileInfo().isSymLink()) {
            continue;
        }
        if (it.fileInfo().isDir()) {
            watcher.addPath(it.fileInfo().filePath());
        } else {
            all.append(it.fileInfo());
        }
    }
    if (all.empty()) {
        emit progress_changed(100);
    }
    qint64 cnt = 0;
    for (auto const& file_info : all) {
        if(!alive) {
            break;
        }
        add_file_info(file_info);
        emit progress_changed(static_cast<int>(100 * (++cnt) / all.size()));
    }
    emit indexing_finished();
}

void text_finder::kill() {
    alive = false;
}

void text_finder::add_file_info(QFileInfo const& file_info) {
    if (file_info.size() < 3) {
        return;
    }
    QFile file(file_info.filePath());
    QSet<qint64> trigram_set;
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream in(&file);
        QChar trigram[3];
        for (int i = 0; i < 3; i++) {
            QChar tmp;
            in >> tmp;
            trigram[i] = tmp;
            if (!tmp.isPrint() && !tmp.isSpace()) {
                return;
            }
        }
        trigram_set.insert(get_trigram_code(trigram));
        while (!in.atEnd()) {
            if (!alive) {
                break;
            }
            for (int i = 0; i < 2; i++) {
                trigram[i] = trigram[i + 1];
            }
            QChar tmp;
            in >> tmp;
            if (!tmp.isPrint() && !tmp.isSpace()) {
                return;
            }
            trigram[2] = tmp;
            trigram_set.insert(get_trigram_code(trigram));
            if (trigram_set.size() > 500000) {
                return;
            }
        }
    }
    QVector<qint64> trigrams;
    for (auto trigram : trigram_set) {
        trigrams.append(trigram);
    }
    std::sort(trigrams.begin(), trigrams.end());
    all_trigrams.push_back(qMakePair(trigrams, file_info));
}

void text_finder::search_in_file(const QFileInfo &file_info, const QString &text) {
    QMutexLocker locker(&mutex);
    QVector<int> prefix_function(text.size());
    for (int i = 1; i < text.size(); i++) {
        int j = prefix_function[i - 1];
        while (j > 0 && text.at(i) != text.at(j)) {
            j = prefix_function[j - 1];
        }
        if (text.at(i) == text.at(j)) {
            j++;
        }
        prefix_function[i] = j;
    }
    prefix_function.append(0);
    int prev = 0;
    QFile file(file_info.filePath());
    QList<QChar> before;
    QQueue<QString> occurences;
    qint64 pos = 0;
    QList<QString> result ;
    if (file.open(QFile::ReadOnly)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            QChar tmp;
            in >> tmp;
            before += tmp;
            if (before.size() > text.size() + 20) {
                before.pop_front();
            }
            for (auto i = occurences.size() - 1; i >= 0; i--) {
                if (occurences[i].size() < text.size() + 40) {
                    occurences[i] += tmp;
                }
            }
            int j = prev;
            while (j > 0 && (j == text.size() || tmp != text.at(j))) {
                j = prefix_function[j - 1];
            }
            if (j < text.size() && tmp == text.at(j)) {
                j++;
            }
            if (j  == text.size()) {
                QString tmp;
                for (auto c : before) {
                    tmp += c;
                }
                occurences.append(tmp);
            }
            prev = j;
            pos++;
        }
    }
    if (occurences.size()) {
        emit occurrence_finded(file_info.filePath(), occurences);
    }
}

void text_finder::change_file(QString dir) {
    futures.push_back(QtConcurrent::run(this, &text_finder::index_directory, current_dir));
}

qint64 text_finder::get_trigram_code(QChar const trigram[3]) {
    qint64 res = 0;
    for (int i = 0; i < 3; i++) {
        res += trigram[i].unicode();
        res <<= 16;
    }
    return res;
}

text_finder::~text_finder() {
    for (auto & future : futures) {
        future.waitForFinished();
    }
}
