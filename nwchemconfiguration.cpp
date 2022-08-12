#include "nwchemconfiguration.h"

#include <QJsonDocument>
#include <QJsonObject>

NWChemConfiguration::NWChemConfiguration(QByteArray data)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (doc.isNull())
        throw err.errorString();
    if (!doc.isObject())
        throw QString("Data is not a JSON object");
    QJsonObject obj = doc.object();
    method = Method(obj.value("method").toInt(0));
    if (method != MethodSCF && method != MethodDFT)
        throw QString("Invalid method");
    basis = obj.value("basis").toString();
    functional = obj.value("functional").toString();
    driverMaxIter = obj.value("driverMaxIter").toInt(-1);
    scfMaxIter = obj.value("scfMaxIter").toInt(-1);
    taskOpt = obj.value("taskOpt").toBool(true);
    taskFreq = obj.value("taskFreq").toBool(false);
    if (obj.contains("spin"))
        *spin = obj.value("spin").toInt(0);
}

bool NWChemConfiguration::hasSpin() const
{
    return spin.has_value() && *spin > 0;
}

QByteArray NWChemConfiguration::serialize() const
{
    QJsonObject obj;
    obj["method"] = int(method);
    obj["basis"] = basis;
    obj["functional"] = functional;
    obj["driverMaxIter"] = driverMaxIter;
    obj["scfMaxIter"] = scfMaxIter;
    obj["taskOpt"] = taskOpt;
    obj["taskFreq"] = taskFreq;
    if (spin.has_value())
        obj["spin"] = *spin;
    QJsonDocument doc;
    doc.setObject(obj);
    return doc.toJson();
}

QString NWChemConfiguration::generateConfig() const
{
    QString result;

    result += generateBasisSection();

    QStringList scfParams;

    if (scfMaxIter != -1)
        scfParams.append(QStringLiteral("  maxiter %1").arg(scfMaxIter));

    if (hasSpin())
    {
        scfParams.append("  sym off");
        scfParams.append("  adapt off");

        if (MethodSCF == method)
            scfParams.append(QStringLiteral("  nopen %1").arg(*spin - 1));
    }

    if (!scfParams.isEmpty())
        result += "scf\n" + scfParams.join("\n") + "\nend\n";

    if (MethodDFT == method)
    {
        QStringList dftParams;
        dftParams.append("dft");
        dftParams.append(QStringLiteral("  xc %1").arg(functional));

        if (hasSpin())
            dftParams.append(QStringLiteral("  mult %1").arg(*spin));

        dftParams.append("end\n");

        result += dftParams.join("\n");
    }

    if (taskOpt && driverMaxIter != -1)
        result += QStringLiteral("driver\n  maxiter %1\nend\n").arg(driverMaxIter);

    //TODO: Configure memory
    result += QStringLiteral("memory total 4 gb\n");

    QString task;
    if (taskOpt)
        task = QStringLiteral("task %1 optimize\n");
    else
        task = QStringLiteral("task %1 energy\n");
    if (taskFreq)
        task += QStringLiteral("task %1 freq\n");


    if (MethodSCF == method)
        result += task.arg("scf");
    else if (MethodDFT == method)
        result += task.arg("dft");
    else
        throw QString("NWChemConfiguration: Invalid method");

    return result;
}

QString NWChemConfiguration::generateBasisSection() const
{
    return QStringLiteral("basis\n  * library %1\nend\n").arg(basis);
}
