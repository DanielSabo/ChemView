#include "linebuffer.h"
#include <QDebug>

LineBuffer::LineBuffer()
{
}

int LineBuffer::append(QByteArray data)
{
    QList<QByteArray> splitData = data.split('\n');
    // Note: QByteArray splits "\n" as [[],[]]

    int linesAdded = lines.isEmpty() ? 0 : -1;
    linesAdded += splitData.length();

    auto iter = splitData.begin();

    if (!lines.isEmpty())
        lines.last().append(*iter++);

    while (iter != splitData.end())
        lines.append(*iter++);

    return linesAdded;
}

QByteArray LineBuffer::joined() const
{
    return joinRange(0, lines.size());
}

QByteArray LineBuffer::joinLast(int count) const
{
    int start = std::max(0, (int)lines.size() - count);
    return joinRange(start, count);
}

QByteArray LineBuffer::joinRange(int start, int count) const
{
    if (start >= lines.size())
        return {};

    // Ensure count doesn't go past the end of the list
    count = std::min(start + count, (int)lines.size()) - start;

    int targetLength = -1;
    for (int i = start; i < start + count; ++i)
        targetLength += lines[i].size() + 1;

    QByteArray result;
    result.reserve(targetLength);
//    qDebug() << "CStart" << result.capacity();

    result.append(lines[start]);
    for (int i = start + 1; i < start + count; ++i)
    {
        result.append('\n');
        result.append(lines[i]);
    }
//    qDebug() << "CEnd" << result.capacity() << result.size();

    return result;
}
