#include "jsonquery.h"

JSONQuery::JSONQuery(const QJsonDocument &d)
{
    if (d.isObject())
        val = d.object();
    else
        val = d.array();
}

JSONQuery JSONQuery::byKey(const QString &key)
{
    if (!val.isObject())
        throw QString("Invalid JSON Query, requested key \"%1\" of value that is not an object").arg(key);
    QJsonObject obj = val.toObject();
    if (!obj.contains(key))
        throw QString("Invalid JSON Query, requested key \"%1\" does not exist").arg(key);
    return obj.value(key);
}

JSONQuery JSONQuery::byIndex(int index)
{
    if (!val.isArray())
        throw QString("Invalid JSON Query, requested index \"%1\" of value that is not an array").arg(index);
    QJsonArray array = val.toArray();
    if (index >= array.count() || index < 0)
        throw QString("Invalid JSON Query, requested index \"%1\" out of range").arg(index);
    return array.at(index);
}

QJsonArray JSONQuery::toArray()
{
    if (!val.isArray())
        throw QString("Invalid JSON Query, value is not an array");

    return val.toArray();
}

QJsonObject JSONQuery::toObject()
{
    if (!val.isObject())
        throw QString("Invalid JSON Query, value is not an object");

    return val.toObject();
}

QString JSONQuery::toString()
{
    if (!val.isString())
        throw QString("Invalid JSON Query, value is not a string");

    return val.toString();
}

int JSONQuery::toInt()
{
    if (val.isDouble())
        return val.toInt();
    else if (val.isBool())
        return val.toBool() ? 1 : 0;

    throw QString("Invalid JSON Query, value is not a number");
}

double JSONQuery::toDouble()
{
    if (val.isDouble())
        return val.toDouble();
    else if (val.isBool())
        return val.toBool() ? 1 : 0;

    throw QString("Invalid JSON Query, value is not a number");
}
