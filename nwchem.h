#ifndef NWCHEM_H
#define NWCHEM_H

#include "molstruct.h"
#include "moldocument.h"
#include "linebuffer.h"
#include "nwchemconfiguration.h"

#include <QByteArray>
#include <regex>

namespace NWChem {
    QByteArray molToOptimize(MolStruct mol, QString name, NWChemConfiguration configuration);

    MolDocument molFromOutputPath(QString path);
    MolDocument molFromOutput(QByteArray data);

    class Parser {
    public:
        Parser(LineBuffer &b) : buffer(b) {
        }

        void parse();

        // Incrementally parse new data after appending it to the linebuffer,
        // returns true if a new section was parsed.
        bool parseIncremental(QByteArray const &data);

        bool findSections(int startLine = 0, int endLine = -1);
        void findTasks();
        int taskForSection(int sectionIdx);

        // Parse an geometry optimization or energy task
        void parseGeomEnergyTask(int start, int end);
        void parseFrequencyTask(int start, int end);

        void parseInputSection(int sectionIdx);
        void parseEnergySection(int sectionIdx);
        void parseGradientSection(int sectionIdx);
        void parseFrequencySection(int sectionIdx);

        bool parseGeometry(int start, int end);
        bool parseConnectivity(int start, int end);
        bool parseOrbitals(int start, int end);

        enum class SectionType {
            Frequency,
            GeometryOpt,
            ModuleDFT,
            ModuleSCF,
            ModuleInput,
            GradientDFT,
            GradientSCF,
            ModuleCPHF,
            Terminate,
            Invalid
        };

        struct SectionStartQuery {
            SectionStartQuery(const char *reStr, SectionType type) : re(reStr), type(type) {}
            std::regex re;
            SectionType type;
        };

        struct SectionInfo {
            SectionType type = SectionType::Invalid;
            int start = -1;
        };

        struct TaskInfo {
            SectionType type = SectionType::Invalid;
            int section = -1;
            int end = -1;
        };

        LineBuffer &buffer;

        std::vector<SectionInfo> sections;
        std::vector<TaskInfo> tasks;
        QList<QString> optimizationSteps;

        MolDocument document;
    };
}

#endif // NWCHEM_H
