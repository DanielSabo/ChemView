#ifndef PARSEHELPERS_H
#define PARSEHELPERS_H

#include <QString>
#include <QList>


QByteArray readFile(QString const &filename);

int indexOrThrow(QByteArray const &data, const char *target, int from);
QString const &atOrThrow(QStringList const &data, int index);

int toIntOrThrow(QString const &str);
double toDoubleOrThrow(QString const &str);

int intFromList(QStringList const &data, int index);
double doubleFromList(QStringList const &data, int index);

QStringList splitFixedWidth(QString &str, QList<int> sizes);

#endif // PARSEHELPERS_H
