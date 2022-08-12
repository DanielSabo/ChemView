#ifndef NWCHEMCONFIGURATION_H
#define NWCHEMCONFIGURATION_H

#include <QString>
#include <optional>

class NWChemConfiguration
{
public:
    NWChemConfiguration() = default;
    explicit NWChemConfiguration(QByteArray data);

    enum Method {
        MethodSCF,
        MethodDFT
    };

    Method method = MethodSCF;
    QString basis = "6-311G**";
    QString functional = "B3LYP";

    int driverMaxIter = -1;
    int scfMaxIter = -1;

    bool taskOpt = true;
    bool taskFreq = false;

    std::optional<int> spin;

    bool hasSpin() const;

    QByteArray serialize() const;
    QString generateConfig() const;
    QString generateBasisSection() const;
};

#endif // NWCHEMCONFIGURATION_H
