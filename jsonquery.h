#ifndef JSONQUERY_H
#define JSONQUERY_H
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

class JSONQuery {
public:
    JSONQuery(const QJsonValue &v) : val(v) {}
    JSONQuery(const QJsonDocument &d);

    JSONQuery byKey(const QString &key);
    JSONQuery byIndex(int index);

    QJsonArray toArray();
    QJsonObject toObject();
    QString toString();
    int toInt();
    double toDouble();

    QJsonValue val;
};

#endif // JSONQUERY_H
