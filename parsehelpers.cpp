#include "parsehelpers.h"

#include <QFile>

QByteArray readFile(const QString &filename)
{
    QFile file(filename);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw QString("read error, couldn't open file");
    }

    QByteArray data = file.readAll();
    if (data.isNull())
    {
        throw QString("read error, couldn't read file");
    }

    return data;
}

int indexOrThrow(QByteArray const &data, const char *target, int from)
{
    int result = data.indexOf(target, from);
    if (result < 0)
        throw QString("QByteArray: Target not found");
    return result;
}

QString const &atOrThrow(QStringList const &data, int index)
{
    if (index >= data.size() || index < 0)
        throw QString("Index out of range");
    return data.at(index);
}

int toIntOrThrow(QString const &str) {
    bool ok = true;
    int result = str.toInt(&ok);
    if (!ok)
        throw QString("Could not convert QString to integer:") + str;
    return result;
}

double toDoubleOrThrow(QString const &str) {
    bool ok = true;
    double result = str.toDouble(&ok);
    if (!ok)
        throw QString("Could not convert QString to double:") + str;
    return result;
}

int intFromList(QStringList const &data, int index)
{
    return toIntOrThrow(atOrThrow(data, index));
}

double doubleFromList(QStringList const &data, int index)
{
    return toDoubleOrThrow(atOrThrow(data, index));
}

QStringList splitFixedWidth(QString &str, QList<int> sizes)
{
    QStringList result;
    int last = 0;
    for (int len: sizes)
    {
        result.push_back(str.mid(last, len));
        last += len;
    }
    return result;
}
