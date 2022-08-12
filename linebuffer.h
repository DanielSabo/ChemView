#ifndef LINEBUFFER_H
#define LINEBUFFER_H

#include <QList>
#include <QByteArray>

class LineBuffer
{
public:
    LineBuffer();

    int append(QByteArray data);
    QByteArray joined() const;
    QByteArray joinLast(int count) const;
    QByteArray joinRange(int start, int count) const;

    QList<QByteArray> lines;
};

#endif // LINEBUFFER_H
