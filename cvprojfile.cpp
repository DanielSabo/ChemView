#include "cvprojfile.h"
#include "parsehelpers.h"
#include "jsonquery.h"
#include "qzipwriter.h"
#include "qzipreader.h"
#include "optimizernwchem.h"

MolDocument CVJSONFile::fromPath(const QString &filename)
{
    QByteArray source;

    try {source = readFile(filename); }
    catch (QString err) { throw QStringLiteral("CVJSONFile: ") + err; }

    return fromData(source);
}

MolDocument CVJSONFile::fromData(const QByteArray &source)
{
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(source, &err);
    if (doc.isNull())
        throw err.errorString();
    JSONQuery json(doc);

    QString version = json.byKey("version").toString();

    if (QStringLiteral("alpha-0") != version)
        throw QString("Unknown version: ") + version;

    JSONQuery molJSON = json.byKey("molecule");
    QJsonArray atomsJSON = molJSON.byKey("atoms").toArray();
    QJsonArray bondsJSON = molJSON.byKey("bonds").toArray();

    MolDocument result;
    for (auto aJSON: atomsJSON)
    {
        Atom a;
        JSONQuery aQuery(aJSON);
        a.x = aQuery.byKey("x").toDouble();
        a.y = aQuery.byKey("y").toDouble();
        a.z = aQuery.byKey("z").toDouble();
        a.element = aQuery.byKey("element").toString();
        if (aJSON.toObject().contains("charge"))
            a.charge = aQuery.byKey("charge").toInt();
        result.molecule.atoms.push_back(a);
    }

    for (auto bJSON: bondsJSON)
    {
        Bond b;
        JSONQuery bQuery(bJSON);
        b.to = bQuery.byKey("to").toInt();
        b.from = bQuery.byKey("from").toInt();
        b.order = bQuery.byKey("order").toInt();
        result.molecule.bonds.push_back(b);
    }

    QJsonObject rootObj = json.toObject();
    if (rootObj.contains("calculated_properties"))
    {
        QJsonObject calculatedProps = json.byKey("calculated_properties").toObject();
        for (auto iter = calculatedProps.begin(); iter != calculatedProps.end(); iter++)
            result.calculatedProperties[iter.key()] = iter.value().toString("<invalid data>");
    }

    if (rootObj.contains("orbitals"))
    {
        QJsonArray obitalJSON = json.byKey("orbitals").toArray();
        for (auto o: obitalJSON)
        {
            MolDocument::MolecularOrbital orbital;
            JSONQuery oQuery(o);
            if(oQuery.byKey("id").val.isString())
                orbital.id = oQuery.byKey("id").toString();
            else
                orbital.id = QString::number(oQuery.byKey("id").toInt());
            orbital.occupancy = oQuery.byKey("occupancy").toDouble();
            orbital.energy = oQuery.byKey("energy").toDouble();
            if (o.toObject().contains("symmetry"))
                orbital.symmetry = oQuery.byKey("symmetry").toString();
            result.orbitals.push_back(orbital);
        }
    }

    if (rootObj.contains("frequencies"))
    {
        int eigenvectorSize = result.molecule.atoms.size() * 3;
        QJsonArray freqJSON = json.byKey("frequencies").toArray();
        for (auto f: freqJSON)
        {
            MolDocument::Frequency freq;
            JSONQuery fQuery(f);
            freq.wavenum = fQuery.byKey("wavenumber").toDouble();
            freq.intensity = fQuery.byKey("intensity").toDouble();
            QJsonArray evecJSON = fQuery.byKey("eigenvector").toArray();
            if (evecJSON.size() != eigenvectorSize)
                throw QString("Invalid eigenvector size");

            //FIXME: Validate that all the values parse as doubles
            for (int i = 0; i < eigenvectorSize; i += 3)
                freq.eigenvector.push_back(QVector3D(evecJSON.at(i+0).toDouble(),
                                                     evecJSON.at(i+1).toDouble(),
                                                     evecJSON.at(i+2).toDouble()));

            result.frequencies.push_back(freq);
        }

    }

    return result;
}

QByteArray CVJSONFile::write(const MolDocument &document)
{
    MolStruct const &mol = document.molecule;

    QJsonArray atomArray;
    for (auto a: mol.atoms)
    {
        QJsonObject atom;
        atom["x"] = a.x;
        atom["y"] = a.y;
        atom["z"] = a.z;
        atom["element"] = a.element;
        if (a.charge != 0)
            atom["charge"] = a.charge;
        atomArray.push_back(atom);
    }

    QJsonArray bondArray;
    for (auto b: mol.bonds)
    {
        QJsonObject bond;
        bond["to"] = b.to;
        bond["from"] = b.from;
        bond["order"] = b.order;
        bondArray.push_back(bond);
    }

    QJsonObject molecule;
    molecule["atoms"] = atomArray;
    molecule["bonds"] = bondArray;

    QJsonObject root;
    root["version"] = QString("alpha-0");
    root["molecule"] = molecule;

    if (!document.calculatedProperties.isEmpty())
    {
        QJsonObject calculatedProps;
        for (auto iter = document.calculatedProperties.begin(); iter != document.calculatedProperties.end(); iter++)
            calculatedProps[iter.key()] = iter.value();
        root["calculated_properties"] = calculatedProps;
    }

    if (!document.orbitals.isEmpty())
    {
        QJsonArray orbitals;
        for (auto const &o: document.orbitals)
        {
            QJsonObject orbital;
            orbital["id"] = o.id;
            orbital["occupancy"] = o.occupancy;
            orbital["energy"] = o.energy;
            orbital["symmetry"] = o.symmetry;
            orbitals.push_back(orbital);
        }
        root["orbitals"] = orbitals;
    }

    if (!document.frequencies.isEmpty())
    {
        QJsonArray frequencies;
        for (auto const &f: document.frequencies)
        {
            QJsonObject freq;
            freq["wavenumber"] = f.wavenum;
            freq["intensity"] = f.intensity;
            QJsonArray eigenvector;
            for (auto const &e: f.eigenvector)
            {
                eigenvector.push_back(e.x());
                eigenvector.push_back(e.y());
                eigenvector.push_back(e.z());
            }
            freq["eigenvector"] = eigenvector;
            frequencies.push_back(freq);
        }
        root["frequencies"] = frequencies;
    }

    QJsonDocument doc;
    doc.setObject(root);
    return doc.toJson();
}

MolDocument CVProjFile::fromPath(const QString &filename)
{
    QZipReader projZipReader(filename);

    QByteArray moleculeData = projZipReader.fileData("molecule.json");
    if (projZipReader.status() != QZipReader::Status::NoError)
        throw QString("Error reading file: ") + projZipReader.device()->errorString();
    if (moleculeData.isEmpty())
        throw QString("Missing molecule.json");
    return CVJSONFile::fromData(moleculeData);
}

bool CVProjFile::write(QIODevice *file, MolDocument const &document)
{
    QZipWriter projZipWriter(file);
    QByteArray moleculeData = CVJSONFile::write(document);
    projZipWriter.addFile("molecule.json", moleculeData);
    projZipWriter.close();

    return true;
}

bool CVProjFile::write(QIODevice *file, MolDocument const &document, const OptimizerNWChem &nwchem)
{
    QZipWriter projZipWriter(file);
    QByteArray moleculeData = CVJSONFile::write(document);
    projZipWriter.addFile("molecule.json", moleculeData);
    nwchem.saveToProjFile(projZipWriter, "molecule_nwchem");
    projZipWriter.close();

    return true;
}
