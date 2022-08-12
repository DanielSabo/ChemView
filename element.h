#ifndef ELEMENT_H
#define ELEMENT_H

#include <QString>

class Element
{
public:
    static Element fromAtomicNumber(int num);
    static Element fromAbbr(QString abbreviation);

public:
    Element();
    explicit Element(int num);
    explicit Element(QString abbreviation);

    int number;
    QString abbr;

    QString name();
    int group(); // Returns < 0 for anthanides/actinides
    double averageMass();
    double empiricalRadius();
    double covalentRadius();

    double estimateBondLength(Element b);

    bool isValid() { return number != 0; }
};

#endif // ELEMENT_H
