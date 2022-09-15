#include "filehandlers.h"
#include "moldocument.h"

#include <QString>
#include <QList>
#include <QFileInfo>
//#include <functional>
//#include <optional>

namespace FileHandlers {
struct FileTypeInfo
{
    QString name;
    QStringList extensions;
    bool canOpen = false;
    bool canSave = false;
//    std::function<MolDocument(QString)> openFunction;
//    std::function<bool(QString, MolDocument)> saveFunction;
};

const QList<FileTypeInfo> &getFileTypes()
{
    static QList<FileTypeInfo> fileTypes;
    if (!fileTypes.isEmpty())
        return fileTypes;

    FileTypeInfo cvProj;
    cvProj.name = "ChemView Project";
    cvProj.extensions = QStringList{"cvproj"};
    cvProj.canOpen = true;
    cvProj.canSave = true;
    fileTypes.push_back(cvProj);

    FileTypeInfo gaussianCube;
    gaussianCube.name = "Gaussian Cube";
    gaussianCube.extensions = QStringList{"cube"};
    gaussianCube.canOpen = true;
    fileTypes.push_back(gaussianCube);

    FileTypeInfo molFile;
    molFile.name = "MOL/SDF";
    molFile.extensions = QStringList{"mol","sdf"};
    molFile.canOpen = true;
    molFile.canSave = true;
    fileTypes.push_back(molFile);

    FileTypeInfo nwchemOutput;
    nwchemOutput.name = "NWChem Output";
    nwchemOutput.extensions = QStringList{"out", "nwout", "log"};
    nwchemOutput.canOpen = true;
    fileTypes.push_back(nwchemOutput);

    FileTypeInfo nwchemInput;
    nwchemInput.name = "NWChem Input";
    nwchemInput.extensions = QStringList{"nw"};
    nwchemInput.canSave = true;
    fileTypes.push_back(nwchemInput);

    FileTypeInfo xyzFile;
    xyzFile.name = "XYZ (Cartesian)";
    xyzFile.extensions = QStringList{"xyz"};
    xyzFile.canOpen = true;
    xyzFile.canSave = true;
    fileTypes.push_back(xyzFile);

    return fileTypes;
}

bool canOpenPath(QString path)
{
    QString suffix = QFileInfo(path).suffix();

    for (const auto &filetype: getFileTypes())
    {
        if (filetype.canOpen && filetype.extensions.contains(suffix))
            return true;
    }

    return false;
}

bool canSavePath(QString path)
{
    QString suffix = QFileInfo(path).suffix();

    for (const auto &filetype: getFileTypes())
    {
        if (filetype.canSave && filetype.extensions.contains(suffix))
            return true;
    }

    return false;
}

const QString &getOpenFilters()
{
    static QString openFilters;
    if (openFilters.isEmpty())
    {
        QStringList filterList({"All files (*.*)"});

        for (const auto &filetype: getFileTypes())
        {
            if (filetype.canOpen)
            {
                QStringList extensionList;
                for (const auto &extension: filetype.extensions)
                    extensionList.append(QStringLiteral("*.") + extension);
                filterList.append(filetype.name + " (" + extensionList.join(" ") + ")");
            }
        }

        openFilters = filterList.join(";;");
    }
    return openFilters;
}

const QString &getSaveFilters()
{
    static QString saveFilters;
    if (saveFilters.isEmpty())
    {
        QStringList filterList;

        for (const auto &filetype: getFileTypes())
        {
            if (filetype.canSave)
            {
                QStringList extensionList;
                for (const auto &extension: filetype.extensions)
                    extensionList.append(QStringLiteral("*.") + extension);
                filterList.append(filetype.name + " (" + extensionList.join(" ") + ")");
            }
        }

        saveFilters = filterList.join(";;");
    }
    return saveFilters;
}

}

