#ifndef FILEHANDLERS_H
#define FILEHANDLERS_H

#include <QString>


namespace FileHandlers
{
    bool canOpenPath(QString path);
    bool canSavePath(QString path);

    const QString &getOpenFilters();
    const QString &getSaveFilters();
};

#endif // FILEHANDLERS_H
