#include "nwchem.h"
#include "parsehelpers.h"
#include "linebuffer.h"
#include "calculation_util.h"

#include <QFile>
#include <QSet>
#include <QTextStream>
#include <QDebug>
#include <regex>

namespace {
    static const double KCAL_PER_HARTREE = 627.5093314;

    int findLineAfterNewlines(LineBuffer const &buffer, int idx, int numNewlines)
    {
        int blanks = 0;
        int last = idx;
        for (; idx < buffer.lines.size(); ++idx)
        {
            if (buffer.lines[idx].isEmpty())
                blanks++;
            else
            {
                if (blanks >= numNewlines)
                    return idx;
                else
                {
                    blanks = 0;
                    last = idx;
                }
            }
        }
        if (idx == buffer.lines.size())
            return idx;
        return -last;
    }

    std::cmatch nextMatch(LineBuffer const &buffer, int &idx, int endIdx, const std::regex &re)
    {
        std::cmatch match;

        for (; idx < endIdx; idx++)
        {
            const QByteArray &line = buffer.lines.at(idx);
            if (std::regex_match(line.data(), match, re))
                return match;
        }

        return match;
    }
}

QByteArray NWChem::molToOptimize(MolStruct mol, QString name, NWChemConfiguration configuration)
{
    QString result;
    QTextStream stream(&result);

    if (!configuration.hasSpin())
    {
        // Calculate spin if not specified
        int spin = calc_util::overallSpin(mol);

        if (spin < 0)
            spin = -spin;

        if (spin > 1)
            configuration.spin = spin;
    }

    stream << "echo\n";
    stream << "start " << name << "\n";

    int charge = calc_util::overallCharge(mol);
    if (charge != 0)
        stream << "charge " << charge << "\n";

    if (configuration.hasSpin())
        stream << "geometry noautosym\n";
    else
        stream << "geometry\n";

    for (Atom const &a : mol.atoms)
    {
        stream << "  " << a.element << " " << a.x << " " << a.y << " " << a.z << "\n";
    }
    stream << "end\n";

    stream << configuration.generateConfig();
    return result.toUtf8();
}

MolDocument NWChem::molFromOutputPath(QString path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        throw QString("NWChem read error, couldn't open file");
    }

    QByteArray source = file.readAll();
    if (source.isNull())
    {
        throw QString("NWChem read error, couldn't read file");
    }

    return molFromOutput(source);
}

MolDocument NWChem::molFromOutput(QByteArray data)
{
    LineBuffer buffer;
    Parser parser(buffer);

    buffer.append(data);
    parser.parse();
    return parser.document;
}

void NWChem::Parser::parse() {
    findSections();
    findTasks();

#if 0
    for (auto &s: sections)
        qDebug() << int(s.type);

    qDebug() << "Tasks:";
    for (auto &t: tasks)
        qDebug() << int(t.type);
#endif
    for (auto &t: tasks)
    {
        if (t.type == SectionType::ModuleSCF ||
            t.type == SectionType::ModuleDFT ||
            t.type == SectionType::GeometryOpt)
        {
            parseGeomEnergyTask(t.section, t.end);
        }
        else if (t.type == SectionType::Frequency)
        {
            parseFrequencyTask(t.section, t.end);
        }
    }
}

bool NWChem::Parser::parseIncremental(QByteArray const &data)
{
    // We can't parse the last line because we don't know if it's been compleatly read yet
    int startLine = std::max((int)buffer.lines.size() - 2, 0);
    buffer.append(data);
    int endLine = std::max((int)buffer.lines.size() - 2, 0);

    // Determine if any new sections were found
    int lastSize = int(sections.size());
    findSections(startLine, endLine);
    if (lastSize != int(sections.size()) && sections.size() > 1)
    {
        int startSection = std::max(lastSize - 1, 0);
        // The last section is either incomplete or a terminator we don't need to parse
        int endSection = int(sections.size()) - 1;

        tasks.clear();
        findTasks();
        int taskIdx = taskForSection(startSection);
        if (taskIdx < 0)
        {
            qWarning() << "Failed to find task for section" << startSection;
            return false;
        }

        // Continue parsing the task that contains startSection
        auto t = tasks[taskIdx++];
        if (t.type == SectionType::ModuleSCF || t.type == SectionType::ModuleDFT || t.type == SectionType::GeometryOpt)
            parseGeomEnergyTask(startSection, std::min(t.end, endSection));
        else if (t.type == SectionType::Frequency)
            parseFrequencyTask(startSection, t.end);

        // Parse any new tasks
        for (;taskIdx < int(tasks.size()); taskIdx++)
        {
            t = tasks[taskIdx];
            if (t.type == SectionType::ModuleSCF || t.type == SectionType::ModuleDFT || t.type == SectionType::GeometryOpt)
                parseGeomEnergyTask(t.section, std::min(t.end, endSection));
            else if (t.type == SectionType::Frequency)
                parseFrequencyTask(t.section, t.end);
        }
        return true;
    }
    return false;
}

bool NWChem::Parser::findSections(int startLine, int endLine) {
    // Find out what kind of section this is

    static const std::vector<SectionStartQuery> queries = {
        {" +NWChem Nuclear Hessian and Frequency Analysis *", SectionType::Frequency},
        {" +NWChem Geometry Optimization *", SectionType::GeometryOpt},
        {" +NWChem DFT Module *", SectionType::ModuleDFT},
        {" +NWChem SCF Module *", SectionType::ModuleSCF},
        {" +NWChem Input Module *", SectionType::ModuleInput},
        {" +NWChem DFT Gradient Module *", SectionType::GradientDFT},
        {" +NWChem Gradients Module *", SectionType::GradientSCF},
        {" +NWChem CPHF Module *", SectionType::ModuleCPHF},
        {" Task  times .*", SectionType::Terminate},
        {" +NWChem .* Module *", SectionType::Invalid},
    };

    if (endLine < 0)
        endLine = buffer.lines.size() - 1;

    for (int currentLine = startLine;
         currentLine >= 0 && currentLine <= endLine;
         currentLine = findLineAfterNewlines(buffer, currentLine, 1))
    {
        const auto &line = buffer.lines.at(currentLine);
        for (const auto &q: queries)
        {
            if (std::regex_match(line.data(), q.re))
            {
                SectionInfo info;
                info.start = currentLine;
                info.type = q.type;
                sections.push_back(info);

                if (q.type == SectionType::Invalid)
                    qWarning() << "NWChem: Unknown section type:" << line.trimmed();

                break;
            }
        }
    }

    return true;
}

void NWChem::Parser::findTasks() {
    auto iter = sections.begin();
    while (iter != sections.end())
    {
        // To determine what task was run find the first non-input section
        auto last = iter;
        iter = std::find_if(iter, sections.end(), [](SectionInfo &info) { return info.type != SectionType::ModuleInput; });

        SectionType taskType = SectionType::Invalid;

        if ((iter != sections.end()) &&
                ((iter->type == SectionType::Frequency) ||
                 (iter->type == SectionType::GeometryOpt) ||
                 (iter->type == SectionType::ModuleDFT) ||
                 (iter->type == SectionType::ModuleSCF)))
        {
            taskType = iter->type;
        }

        iter = std::find_if(iter, sections.end(), [](SectionInfo &info) { return info.type == SectionType::Terminate; });

        TaskInfo info;
        info.type = taskType;
        info.section = last - sections.begin();
        info.end = iter - sections.begin();
        tasks.push_back(info);

        // If we found a task times line (SectionType::Terminate) at the end then look for another task after this in on the next loop
        if (iter != sections.end())
            iter++;
    }

    /* The output log ends with an input section followed by a " Total times" line we don't parse so
         * the list should always end a SectionType::Invalid task.
         */
}

int NWChem::Parser::taskForSection(int sectionIdx)
{
    for (auto iter = tasks.begin(); iter != tasks.end(); iter++)
    {
        if (sectionIdx >= iter->section && sectionIdx <= iter->end)
            return iter - tasks.begin();
    }

    return -1;
}

void NWChem::Parser::parseGeomEnergyTask(int start, int end)
{
    for (int i = start; i < end; ++i)
    {
        if (sections[i].type == SectionType::ModuleDFT)
            parseEnergySection(i);
        else if (sections[i].type == SectionType::ModuleSCF)
            parseEnergySection(i);
        else if (sections[i].type == SectionType::ModuleInput)
            parseInputSection(i);
        else if (sections[i].type == SectionType::GradientDFT)
            parseGradientSection(i);
        else if (sections[i].type == SectionType::GradientSCF)
            parseGradientSection(i);
    }
}

void NWChem::Parser::parseFrequencyTask(int start, int end)
{
    (void)start;

    // Only the final section for this task will have the frequency information
    if (sections[end].type != SectionType::Terminate)
        return;

    if ((sections[end-1].type == SectionType::ModuleCPHF) ||
        (sections[end-1].type == SectionType::ModuleDFT) ||
        (sections[end-1].type == SectionType::ModuleSCF))
    {
        parseFrequencySection(end-1);
    }
}

bool NWChem::Parser::parseGeometry(int start, int end)
{
    /* The header of the Geometry block will look like this, where "geometry" is the stucture name:
                         Geometry "geometry" -> "geometry"
                         ---------------------------------

 Output coordinates in angstroms (scale by  1.889725989 to convert to a.u.)

  No.       Tag          Charge          X              Y              Z
 ---- ---------------- ---------- -------------- -------------- --------------
    1 O                    8.0000    -0.51726868    -1.57191296     0.00512917
     */
    static const std::regex geometry_re("\\s*Geometry \".*\" -> \".*\"\\s*");
    static const std::regex geometry_start_re("\\s+No\\.\\s+Tag\\s+Charge\\s+X\\s+Y\\s+Z\\s*");

    int curLine = start;
    std::cmatch match;
    match = nextMatch(buffer, curLine, end, geometry_re);
    if (match.empty())
        return false;
    //        throw QString("NWChem read error, no Geometry section");

    MolStruct result;

    try {
        //TODO: Parse scale (currently assumes angstroms)

        // Find the geometry table headers
        match = nextMatch(buffer, curLine, end, geometry_start_re);
        if (match.empty())
            throw QString("Missing goemetry data");

        curLine += 2; // Skip the separator line

        for (;curLine < buffer.lines.size(); curLine++)
        {
            QString line = QString::fromUtf8(buffer.lines.at(curLine));
            if (line.isEmpty())
                break;

            Atom a;
            auto lineData = line.split(" ", Qt::SkipEmptyParts);
            a.element = lineData.at(1).trimmed();

            a.x = doubleFromList(lineData, 3);
            a.y = doubleFromList(lineData, 4);
            a.z = doubleFromList(lineData, 5);
            result.atoms.append(a);
        }
    } catch (QString e) {
        throw QString("NWChem read error, invalid Geometry section");
    }

    /* There will be multiple geometry sections as the optimization progresses but connectivity is
     * only given at the start and the end of the process so we want to keep the bond information
     * unchanged.
     */

    document.molecule.atoms = result.atoms;
//    qDebug() << ".geom.";
    return true;
}

bool NWChem::Parser::parseConnectivity(int start, int end)
{
    static const std::regex internuclear_distances_re("\\s*internuclear distances\\s*");
    static const std::regex any_separator_re("\\s*[\\s-]+\\s*");

    int curLine = start;
    std::cmatch match;
    match = nextMatch(buffer, curLine, end, internuclear_distances_re);
    if (match.empty())
        return false;

    curLine += 4; // Skip the separator lines & headers
    //TODO: Validate headers

    MolStruct result = document.molecule;

    try {
        for (;curLine < buffer.lines.size(); curLine++)
        {
            if (std::regex_match(buffer.lines.at(curLine).data(), any_separator_re))
                break;

            QString line = QString::fromUtf8(buffer.lines.at(curLine));

            Bond b;
            auto lineData = line.split("|");

            b.from = intFromList(atOrThrow(lineData, 0).split(" ", Qt::SkipEmptyParts), 0) - 1;
            b.to = intFromList(atOrThrow(lineData, 1).split(" ", Qt::SkipEmptyParts), 0) - 1;
            result.bonds.append(b);
        }
    }  catch (QString e) {
        throw QString("NWChem read error, invalid internuclear distances section data");
    }

    document.molecule = result;
//    qDebug() << ".conn.";
    return true;
}

bool NWChem::Parser::parseOrbitals(int curLine, int endLine)
{
    static const std::regex mo_analysis_re("\\s*(\\w+) Final (\\w+ )?Molecular Orbital Analysis\\s*");
    static const std::regex orbital_entry_re("\\s*Vector\\s+(\\d+)\\s+Occ=(\\s?[^\\s]*)\\s+E=(\\s?[^\\s]*)\\s*(?:\\s+Symmetry=(.*))?");

    QList<MolDocument::MolecularOrbital> orbitals;

    try {
        QString alphaBeta;
        std::cmatch match;
        match = nextMatch(buffer, curLine, endLine, mo_analysis_re);
        while (!match.empty())
        {
            QString suffix;
            alphaBeta = QString::fromStdString(match.str(2)).trimmed();
            if (alphaBeta == QStringLiteral("Alpha"))
                suffix = QStringLiteral("a");
            else if (alphaBeta == QStringLiteral("Beta"))
                suffix = QStringLiteral("b");
            else if (!alphaBeta.isEmpty())
                throw QString("Unknown orbital type: ") + alphaBeta;

            int orbitalsEndLine = findLineAfterNewlines(buffer, curLine, 2);

            match = nextMatch(buffer, curLine, orbitalsEndLine, orbital_entry_re);
            while (!match.empty())
            {
                MolDocument::MolecularOrbital orbital;
                orbital.id = QString::number(toIntOrThrow(QString::fromStdString(match.str(1)))) + suffix;
                QString occupancy = QString::fromStdString(match.str(2));
                occupancy.replace('D', 'e');
                occupancy.replace('E', 'e');
                orbital.occupancy = toDoubleOrThrow(occupancy);
                QString energy = QString::fromStdString(match.str(3));
                energy.replace('D', 'e');
                energy.replace('E', 'e');
                orbital.energy = toDoubleOrThrow(energy);
                orbital.symmetry = QString::fromStdString(match.str(4));

                orbitals.append(orbital);

                curLine++;
                match = nextMatch(buffer, curLine, orbitalsEndLine, orbital_entry_re);
            }

            match = nextMatch(buffer, curLine, endLine, mo_analysis_re);
        }

        // If we have both alpha & beta orbitals we sort them by energy
        if (!alphaBeta.isEmpty())
        {
            std::sort(orbitals.begin(), orbitals.end(), [](MolDocument::MolecularOrbital a, MolDocument::MolecularOrbital b) {
                return a.energy < b.energy;
            });
        }

        if (orbitals.isEmpty())
            throw QString("Missing orbitals section");

    } catch (QString e) {
        //TODO: Return a warning with the document
        qDebug() << "Invalid orbitals section:" << e;
        orbitals = {};
        return false;
    }

    document.orbitals = orbitals;
    return true;
}

void NWChem::Parser::parseInputSection(int sectionIdx) {
    int endLine = buffer.lines.size();
    if (sectionIdx + 1 < int(sections.size()))
        endLine = sections[sectionIdx + 1].start;

    parseGeometry(sections[sectionIdx].start, endLine);
    parseConnectivity(sections[sectionIdx].start, endLine);
}

void NWChem::Parser::parseEnergySection(int sectionIdx) {
    int endLine = buffer.lines.size();
    if (sectionIdx + 1 < int(sections.size()))
        endLine = sections[sectionIdx + 1].start;

    static const std::regex geometry_re("\\s*Geometry \".*\" -> \".*\"\\s*");
    // TODO: Parse unrestricted orbitals:
    static const std::regex orbital_re("\\s*(\\w+) Final (Alpha )?Molecular Orbital Analysis\\s*");
    //static const std::regex orbital_re("\\s*(\\w+) Final Molecular Orbital Analysis\\s*");

    static const std::regex charge_re("\\s+Charge\\s*:\\s*(-?\\d+)\\s*");
    static const std::regex spin_re("\\s+Spin multiplicity\\s*:\\s*(-?\\d+)\\s*");

    static const std::regex scf_energy_re("\\s+Total SCF energy\\s*=\\s*(-?\\d+\\.*\\d*)\\s*");
    static const std::regex dft_energy_re("\\s+Total DFT energy\\s*=\\s*(-?\\d+\\.*\\d*)\\s*");

    std::regex energy_re;
    QString energyType;
    if (sections[sectionIdx].type == SectionType::ModuleDFT)
    {
        energy_re = dft_energy_re;
        energyType = "Total DFT Energy";
        document.calculatedProperties.remove("Total SCF Energy");
    }
    else
    {
        energy_re = scf_energy_re;
        energyType = "Total SCF Energy";
        document.calculatedProperties.remove("Total DFT Energy");
    }

    for (int i = sections[sectionIdx].start; i < endLine; i++)
    {
        const QByteArray &line = buffer.lines.at(i);
        std::cmatch match;
        if (std::regex_match(line.data(), match, charge_re))
        {
            bool ok = false;
            QString s = QString::fromStdString(match.str(1));
            s.toInt(&ok);
            if (!ok)
                qWarning() << "Failed to parse charge:" << match.str(0).c_str();
            else
                document.calculatedProperties["Charge"] = s;
        }
        else if (std::regex_match(line.data(), match, spin_re))
        {
            bool ok = false;
            QString s = QString::fromStdString(match.str(1));
            s.toInt(&ok);
            if (!ok)
                qWarning() << "Failed to parse spin:" << match.str(0).c_str();
            else
                document.calculatedProperties["Spin"] = s;
        }
        else if (std::regex_match(line.data(), match, energy_re))
        {
            bool ok = false;
            QString s = QString::fromStdString(match.str(1));
            s.toDouble(&ok);
            if (!ok)
                qWarning() << "Failed to parse energy:" << match.str(0).c_str();
            else
                document.calculatedProperties[energyType] = s;
        }
        else if (std::regex_match(line.data(), geometry_re))
        {
            parseGeometry(i, endLine);
        }
        else if (std::regex_match(line.data(), orbital_re))
        {
            parseOrbitals(i, endLine);
        }
        else if (line.startsWith(" solvent parameters"))
        {
            while (i++ < endLine)
            {
                const QByteArray &line = buffer.lines.at(i);
                if (line.isEmpty())
                    break;

                static const std::regex solvent_name_re("\\s+solvname_long\\s*:\\s*(\\S+)\\s*");
                static const std::regex solvent_dielec_re("\\s+dielec\\s*:\\s*(-?\\d+.\\d+)\\s*");
                if (std::regex_match(line.data(), match, solvent_name_re))
                {
                    QString s = QString::fromStdString(match.str(1));
                    if (s.isEmpty())
                        qWarning() << "Failed to parse solvent name:" << match.str(0).c_str();
                    else
                        document.calculatedProperties["Solvent"] = s;
                }
                else if (std::regex_match(line.data(), match, solvent_dielec_re))
                {
                    bool ok = false;
                    QString s = QString::fromStdString(match.str(1));
                    s.toDouble(&ok);
                    if (!ok)
                        qWarning() << "Failed to parse solvent dielectric:" << match.str(0).c_str();
                    else
                        document.calculatedProperties["Solvent dielectric"] = s;
                }
            }
        }
    }
}

void NWChem::Parser::parseGradientSection(int sectionIdx)
{
    //qDebug() << ".grad.";
    int endLine = buffer.lines.size();
    if (sectionIdx + 1 < int(sections.size()))
        endLine = sections[sectionIdx + 1].start;

    for (int i = sections[sectionIdx].start; i < endLine; i++)
    {
        const QByteArray &line = buffer.lines.at(i);
        if (line.startsWith("@"))
            optimizationSteps.append(QString::fromUtf8(line));
    }

}

void NWChem::Parser::parseFrequencySection(int sectionIdx)
{
//    qDebug() << ".freq.";
    //TODO: Fix error handling (make warnings vs. exceptions consistant)

    int endLine = buffer.lines.size();
    if (sectionIdx + 1 < int(sections.size()))
        endLine = sections[sectionIdx + 1].start;

    // I don't know the purpose of the first set of eigenvector/eigenvalue printouts but
    // the ones we want come after the temperature/energy values
    static const std::regex temperature_re("\\s+Temperature\\s*=\\s*(\\d+\\.*\\d*)K\\s*");
    static const std::regex enthalpy_re("\\s+Thermal correction to Enthalpy\\s*=\\s*(-?\\d+\\.*\\d*) kcal\\/mol\\s+\\(\\s*(-?\\d+\\.\\d*) au\\)\\s*");
    static const std::regex entropy_re("\\s+Total Entropy\\s*=\\s*(-?\\d+\\.*\\d*) cal/mol-K\\s*");
    static const std::regex heat_cap_re("\\s+Cv \\(constant volume heat capacity\\)\\s*=\\s*(-?\\d+\\.*\\d*) cal\\/mol-K\\s*");
    static const std::regex eigenvector_header("\\s+NORMAL MODE EIGENVECTORS IN CARTESIAN COORDINATES\\s*");
    static const std::regex eigenvector_freq("\\s+P\\.Frequency .*");
    static const std::regex ir_header("\\s+Normal Eigenvalue\\s+\\|\\|\\s+Projected Infra Red Intensities\\s*");


    int curLine = sections[sectionIdx].start;
    std::cmatch match;
    match = nextMatch(buffer, curLine, endLine, temperature_re);
    if (match.empty())
        throw QString("Frequency: Missing temperature");
    else
    {
        bool ok = false;
        QString s = QString::fromStdString(match.str(1));
        s.toDouble(&ok);
        if (!ok)
            qWarning() << "Failed to parse temperature:" << match.str(0).c_str();
        else
            document.calculatedProperties["Temperature (K)"] = s;
    }

    match = nextMatch(buffer, curLine, endLine, enthalpy_re);
    if (match.empty())
        throw QString("Frequency: Missing enthalpy");
    else
    {
        // Parse the 2nd value which is in au
        bool ok = false;
        QString s = QString::fromStdString(match.str(2));
        double enthalpyCorrectionAU = s.toDouble(&ok);
        if (!ok)
            qWarning() << "Failed to parse enthalpy:" << match.str(0).c_str();
        else
        {
            QString totalEnergyStr;
            totalEnergyStr = document.calculatedProperties.value("Total SCF Energy");
            if (totalEnergyStr.isEmpty())
                totalEnergyStr = document.calculatedProperties.value("Total DFT Energy");
            if (!totalEnergyStr.isEmpty())
            {
                double totalEnergyAU = totalEnergyStr.toDouble(&ok);
                if (!ok)
                    qWarning() << "Can't calculate enthalpy, no total energy available";
                else
                {
                    double enthalpyKCalMol = (totalEnergyAU+enthalpyCorrectionAU)*KCAL_PER_HARTREE;
                    document.calculatedProperties["Enthalpy (kcal/mol)"] = QString::number(enthalpyKCalMol, 'f', -1);
                }
            }
        }
    }

    match = nextMatch(buffer, curLine, endLine, entropy_re);
    if (match.empty())
        throw QString("Frequency: Missing entropy");
    else
    {
        QString s = QString::fromStdString(match.str(1));
        document.calculatedProperties["Total Entropy (cal/mol-K)"] = s;
    }

    match = nextMatch(buffer, curLine, endLine, heat_cap_re);
    if (match.empty())
        throw QString("Frequency: Missing heat capacity");
    else
    {
        QString s = QString::fromStdString(match.str(1));
        document.calculatedProperties["Cv heat capacity (cal/mol-K)"] = s;
    }

    if (document.calculatedProperties.contains("Temperature (K)") &&
        document.calculatedProperties.contains("Enthalpy (kcal/mol)") &&
        document.calculatedProperties.contains("Total Entropy (cal/mol-K)"))
    {
        double temperatureK = toDoubleOrThrow(document.calculatedProperties["Temperature (K)"]);
        double enthalpyKCalMol = toDoubleOrThrow(document.calculatedProperties["Enthalpy (kcal/mol)"]);
        double entropyCalMol = toDoubleOrThrow(document.calculatedProperties["Total Entropy (cal/mol-K)"]);
        double gibbsFreeEnergyKCalMol = enthalpyKCalMol - temperatureK*entropyCalMol/1000.0;
        document.calculatedProperties["Gibbs Free Energy (kcal/mol)"] = QString::number(gibbsFreeEnergyKCalMol, 'f', -1);
    }


    /* The eigenvectors come in blocks line this:

 Frequency        -12.60       -5.22       -4.48        3.94        4.26        6.59

           1     0.00004    -0.21337    -0.00122    -0.04514    -0.00044    -0.09467
           2     0.00008    -0.02094     0.00071     0.17022     0.00290    -0.04279
           3     0.07235    -0.00108     0.20030     0.00087    -0.11014    -0.00002
           4     0.00006    -0.28566    -0.00153    -0.04395    -0.00029    -0.06415

     * Where each the number on the left is one axis of one atom. We combine all the
     * frequencies into a single row for each component then split them between
     * frequencies.
     */

    QList<QList<float>> eigenByAtom;
    match = nextMatch(buffer, curLine, endLine, eigenvector_header);
    if (match.empty())
        throw QString("Frequency: Missing eigenvector section");
    else
    {
        int eigenvectorEndLine = findLineAfterNewlines(buffer, curLine, 2);
        while (curLine++ < eigenvectorEndLine)
        {
            match = nextMatch(buffer, curLine, eigenvectorEndLine, eigenvector_freq);
            curLine += 2;
            if (curLine >= eigenvectorEndLine)
                break;
            int rowIdx = 0;
            while (curLine < eigenvectorEndLine && !buffer.lines[curLine].isEmpty())
            {
                if (rowIdx >= eigenByAtom.size())
                    eigenByAtom.push_back({});
                QStringList vectorsLine = QString::fromUtf8(buffer.lines[curLine]).split(" ", Qt::SkipEmptyParts);
                if (intFromList(vectorsLine, 0) != rowIdx + 1)
                    throw QString("Bad eigenvector row: %1").arg(rowIdx);
                for (int i = 1; i < vectorsLine.size(); i++)
                    eigenByAtom[rowIdx].push_back(doubleFromList(vectorsLine, i));

                rowIdx++;
                curLine++;
            }

            if (eigenByAtom.size() % 3 != 0)
                throw QString("Frequency: Number of eigenvector componenets is not divisible by 3!");
        }

        if (eigenByAtom.isEmpty())
            throw QString("Frequency: Failed to parse eigenvector section");
    }

    /* The IR section looks something like this:
 ----------------------------------------------------------------------------
 Normal Eigenvalue ||           Projected Infra Red Intensities
  Mode   [cm**-1]  || [atomic units] [(debye/angs)**2] [(KM/mol)] [arbitrary]
 ------ ---------- || -------------- ----------------- ---------- -----------
    1       -0.000 ||    0.004683           0.108         4.565       1.275
    2       -0.000 ||    0.030686           0.708        29.914       8.355
...
 ----------------------------------------------------------------------------
    */

    match = nextMatch(buffer, curLine, endLine, ir_header);
    if (match.empty())
        throw QString("Frequency: Missing IR section");
    else
    {
        QList<MolDocument::Frequency> frequencies;

        curLine += 3;
        while (curLine < endLine)
        {
            const QByteArray &line = buffer.lines.at(curLine);
            if (line.startsWith(" ---"))
                break;
            QStringList splitLine = QString::fromUtf8(line).split(" ", Qt::SkipEmptyParts);

            MolDocument::Frequency freq;
            freq.wavenum = doubleFromList(splitLine, 1);
            freq.intensity = doubleFromList(splitLine, 6);

            frequencies.push_back(freq);

            curLine++;
        }

        if (eigenByAtom.isEmpty() || (frequencies.size() != eigenByAtom[0].size()))
            throw QString("Number of eigenvectors does not match number of IR frequencies");

        for (int i = 0; i < frequencies.size(); i++)
            for (int j = 0; j < eigenByAtom.size(); j += 3)
            {
                QVector3D v(eigenByAtom[j+0][i], eigenByAtom[j+1][i], eigenByAtom[j+2][i]);
                frequencies[i].eigenvector.push_back(v);
            }

        document.frequencies = frequencies;
    }
}
