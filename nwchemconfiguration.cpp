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

namespace {
    static const QSet<QString> cartesianBasisSets = QSet<QString>(
                {"3-21g", "3-21++g", "3-21gs", "3-21++gs", "3-21gsp", "3-21gs_polarization",
                 "4-22gsp", "4-31g", "6-31g-blaudeau", "6-31++g", "6-31gs-blaudeau", "6-31+gs",
                 "6-31++gs", "6-31gs_polarization", "6-31gss", "6-31++gss", "6-31gss_polarization",
                 "blaudeau_polarization", "demon_coulomb_fitting", "dgauss_a1_dft_coulomb_fitting",
                 "dgauss_a1_dft_exchange_fitting", "dgauss_a2_dft_coulomb_fitting",
                 "dgauss_a2_dft_exchange_fitting", "dhms_polarization", "dunning-hay_diffuse",
                 "dunning-hay_double_rydberg", "dunning-hay_rydberg", "dz_+_double_rydberg_dunning-hay",
                 "dz_dunning", "dzp_+_diffuse_dunning", "dzp_dunning", "dzp_+_rydberg_dunning",
                 "dz_+_rydberg_dunning", "dzvp2_dft_orbital", "dzvp_dft_orbital", "gamess_pvtz",
                 "gamess_vtz", "hondo7_polarization", "huzinaga_polarization", "iglo-ii", "iglo-iii",
                 "mclean_chandler_vtz", "midi_huzinaga", "mini_huzinaga", "mini_scaled", "sbkjc_vdz_ecp",
                 "sto-2g", "sv_+_double_rydberg_dunning-hay", "sv_dunning-hay", "svp_+_diffuse_dunning-hay",
                 "svp_+_diffuse_+_rydberg", "svp_dunning-hay", "svp_+_rydberg_dunning-hay",
                 "sv_+_rydberg_dunning-hay", "tzvp_dft_orbital"});
}

QString NWChemConfiguration::generateBasisSection() const
{
    /* Gaussian basis sets can be defined using either spherical or Cartesian
     * coordinates, and to match the expected energy values calculations using
     * them should be performed using the same coordinate systems. Unfortunately
     * NWChem defaults to Cartesian coordinates for all basis sets, and we have
     * to manually specify which type to use.
     *
     * A more correct way of doing this would be to inspect the basis set file
     * itself at runtime, but for now we just hard code a list of the Cartesian
     * basis sets included in NWChem 7.0.2.
     */

    bool isCartesian = cartesianBasisSets.contains(basis.toLower().trimmed());

    return QStringLiteral("basis %1\n  * library %2\nend\n")
            .arg(isCartesian ? "cartesian" : "spherical").arg(basis);
}
