#include "element.h"

#include <QMap>
#include <QVector>

namespace {
    struct ElementData {
        int number;
        QString abbr;
        QString name;
        double mass;  /* daltons */
        double empiricalRadius; /* angstroms */
        double covalentRadius; /* angstroms */
    };

    // Atomic masses from: https://www.qmul.ac.uk/sbcs/iupac/AtWt/
    // Covalent radii from https://en.wikipedia.org/wiki/Atomic_radii_of_the_elements_(data_page)
    static QVector<ElementData> elements =
    {
        {0, "", "Invalid", 0.0, 0.0, 0.0},
        {1, "H", "Hydrogen", 1.008, 0.25, 0.32},
        {2, "He", "Helium", 4.002, 1.20, 0.46},
        {3, "Li", "Lithium", 6.94, 1.45, 1.33},
        {4, "Be", "Beryllium", 9.012, 1.05, 1.02},
        {5, "B", "Boron", 10.81, 0.85, 0.85},
        {6, "C", "Carbon", 12.011, 0.70, 0.75},
        {7, "N", "Nitrogen", 14.007, 0.65, 0.71},
        {8, "O", "Oxygen", 15.999, 0.60, 0.63},
        {9, "F", "Fluorine", 18.998, 0.50, 0.64},
        {10, "Ne", "Neon", 20.1797, 1.60, 0.67},
        {11, "Na", "Sodium", 22.989, 1.80, 1.55},
        {12, "Mg", "Magnesium", 24.305, 1.50, 1.39},
        {13, "Al", "Aluminium", 26.981, 1.25, 1.26},
        {14, "Si", "Silicon", 28.085, 1.10, 1.16},
        {15, "P", "Phosphorus", 30.973, 1.00, 1.11},
        {16, "S", "Sulfur", 32.06, 1.00, 1.03},
        {17, "Cl", "Chlorine", 35.45, 1.00, 0.99},
        {18, "Ar", "Argon", 39.948, 0.71, 0.96},
        {19, "K", "Potassium", 39.0983, 2.20, 1.96},
        {20, "Ca", "Calcium", 40.078, 1.80, 1.71},
        {21, "Sc", "Scandium", 44.955, 1.60, 1.48},
        {22, "Ti", "Titanium", 47.867, 1.40, 1.36},
        {23, "V", "Vanadium", 50.9415, 1.35, 1.34},
        {24, "Cr", "Chromium", 51.9961, 1.40, 1.22},
        {25, "Mn", "Manganese", 54.938, 1.40, 1.19},
        {26, "Fe", "Iron", 55.845, 1.40, 1.16},
        {27, "Co", "Cobalt", 58.933, 1.35, 1.11},
        {28, "Ni", "Nickel", 58.6934, 1.35, 1.10},
        {29, "Cu", "Copper", 63.546, 1.35, 1.12},
        {30, "Zn", "Zinc", 65.38, 1.35, 1.18},
        {31, "Ga", "Gallium", 69.723, 1.30, 1.24},
        {32, "Ge", "Germanium", 72.630, 1.25, 1.21},
        {33, "As", "Arsenic", 74.921, 1.15, 1.21},
        {34, "Se", "Selenium", 78.971, 1.15, 1.16},
        {35, "Br", "Bromine", 79.904, 1.15, 1.14},
        {36, "Kr", "Krypton", 83.798, 0.00, 1.17},
        {37, "Rb", "Rubidium", 85.4678, 2.35, 2.10},
        {38, "Sr", "Strontium", 87.62, 2.00, 1.85},
        {39, "Y", "Yttrium", 88.905, 1.80, 1.63},
        {40, "Zr", "Zirconium", 91.224, 1.55, 1.54},
        {41, "Nb", "Niobium", 92.906, 1.45, 1.47},
        {42, "Mo", "Molybdenum", 95.95, 1.45, 1.38},
        {43, "Tc", "Technetium", 97, 1.35, 1.28},
        {44, "Ru", "Ruthenium", 101.07, 1.30, 1.25},
        {45, "Rh", "Rhodium", 102.905, 1.35, 1.25},
        {46, "Pd", "Palladium", 106.42, 1.40, 1.20},
        {47, "Ag", "Silver", 107.8682, 1.60, 1.28},
        {48, "Cd", "Cadmium", 112.414, 1.55, 1.36},
        {49, "In", "Indium", 114.818, 1.55, 1.42},
        {50, "Sn", "Tin", 118.710, 1.45, 1.40},
        {51, "Sb", "Antimony", 121.760, 1.45, 1.40},
        {52, "Te", "Tellurium", 127.60, 1.40, 1.36},
        {53, "I", "Iodine", 126.904, 1.40, 1.33},
        {54, "Xe", "Xenon", 131.293, 0.00, 1.31},
        {55, "Cs", "Caesium", 132.905, 2.65, 2.32},
        {56, "Ba", "Barium", 137.327, 2.15, 1.96},
        {57, "La", "Lanthanum", 138.905, 1.95, 1.80},
        {58, "Ce", "Cerium", 140.116, 1.85, 1.63},
        {59, "Pr", "Praseodymium", 140.907, 1.85, 1.76},
        {60, "Nd", "Neodymium", 144.242, 1.85, 1.74},
        {61, "Pm", "Promethium", 145, 1.85, 1.73},
        {62, "Sm", "Samarium", 150.36, 1.85, 1.72},
        {63, "Eu", "Europium", 151.964, 1.85, 1.68},
        {64, "Gd", "Gadolinium", 157.25, 1.80, 1.69},
        {65, "Tb", "Terbium", 158.925, 1.75, 1.68},
        {66, "Dy", "Dysprosium", 162.500, 1.75, 1.67},
        {67, "Ho", "Holmium", 164.930, 1.75, 1.66},
        {68, "Er", "Erbium", 167.259, 1.75, 1.65},
        {69, "Tm", "Thulium", 168.934, 1.75, 1.64},
        {70, "Yb", "Ytterbium", 173.045, 1.75, 1.70},
        {71, "Lu", "Lutetium", 174.9668, 1.75, 1.62},
        {72, "Hf", "Hafnium", 178.486, 1.55, 1.52},
        {73, "Ta", "Tantalum", 180.947, 1.45, 1.46},
        {74, "W", "Tungsten", 183.84, 1.35, 1.37},
        {75, "Re", "Rhenium", 186.207, 1.35, 1.31},
        {76, "Os", "Osmium", 190.23, 1.30, 1.29},
        {77, "Ir", "Iridium", 192.217, 1.35, 1.22},
        {78, "Pt", "Platinum", 195.084, 1.35, 1.23},
        {79, "Au", "Gold", 196.966, 1.35, 1.24},
        {80, "Hg", "Mercury", 200.592, 1.50, 1.33},
        {81, "Tl", "Thallium", 204.38, 1.90, 1.44},
        {82, "Pb", "Lead", 207.2, 1.80, 1.44},
        {83, "Bi", "Bismuth", 208.980, 1.60, 1.51},
        {84, "Po", "Polonium", 209, 1.90, 1.45},
        {85, "At", "Astatine", 210, 0.00, 1.47},
        {86, "Rn", "Radon", 222, 0.00, 1.42},
        {87, "Fr", "Francium", 223, 0.00, 0.0},
        {88, "Ra", "Radium", 226, 2.15, 2.01},
        {89, "Ac", "Actinium", 227, 1.95, 1.86},
        {90, "Th", "Thorium", 232.0377, 1.80, 1.75},
        {91, "Pa", "Protactinium", 231.035, 1.80, 1.69},
        {92, "U", "Uranium", 238.028, 1.75, 1.70},
        {93, "Np", "Neptunium", 237, 1.75, 1.71},
        {94, "Pu", "Plutonium", 244, 1.75, 1.72},
        {95, "Am", "Americium", 243, 1.75, 1.66},
        {96, "Cm", "Curium", 247, 1.76, 1.66},
        {97, "Bk", "Berkelium", 247, 0.00, 0.0},
        {98, "Cf", "Californium", 251, 0.00, 0.0},
        {99, "Es", "Einsteinium", 252, 0.00, 0.0},
        {100, "Fm", "Fermium", 257, 0.00, 0.0},
        {101, "Md", "Mendelevium", 258, 0.00, 0.0},
        {102, "No", "Nobelium", 259, 0.00, 0.0},
        {103, "Lr", "Lawrencium", 262, 0.00, 0.0},
        {104, "Rf", "Rutherfordium", 267, 0.00, 0.0},
        {105, "Db", "Dubnium", 270, 0.00, 0.0},
        {106, "Sg", "Seaborgium", 269, 0.00, 0.0},
        {107, "Bh", "Bohrium", 270, 0.00, 0.0},
        {108, "Hs", "Hassium", 270, 0.00, 0.0},
        {109, "Mt", "Meitnerium", 278, 0.00, 0.0},
        {110, "Ds", "Darmstadtium", 281, 0.00, 0.0},
        {111, "Rg", "Roentgenium", 281, 0.00, 0.0},
        {112, "Cn", "Copernicium", 285, 0.00, 0.0},
        {113, "Nh", "Nihonium", 286, 0.00, 0.0},
        {114, "Fl", "Flerovium", 289, 0.00, 0.0},
        {115, "Mc", "Moscovium", 289, 0.00, 0.0},
        {116, "Lv", "Livermorium", 293, 0.00, 0.0},
        {117, "Ts", "Tennessine", 293, 0.00, 0.0},
        {118, "Og", "Oganesson", 294, 0.00, 0.0},
    };

    static QVector<int> periodicGroups = {
        1, 18,
        1, 2, 13, 14, 15, 16, 17, 18,
        1, 2, 13, 14, 15, 16, 17, 18,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        1, 2, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        1, 2, -1, -2, -3, -4, -5, -6, -7, -8, -9, -10, -11, -12, -13, -14, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};

    static QMap<QString, int> elementAbbreviations =
    {
        {"h", 1},
        {"he", 2},
        {"li", 3},
        {"be", 4},
        {"b", 5},
        {"c", 6},
        {"n", 7},
        {"o", 8},
        {"f", 9},
        {"ne", 10},
        {"na", 11},
        {"mg", 12},
        {"al", 13},
        {"si", 14},
        {"p", 15},
        {"s", 16},
        {"cl", 17},
        {"ar", 18},
        {"k", 19},
        {"ca", 20},
        {"sc", 21},
        {"ti", 22},
        {"v", 23},
        {"cr", 24},
        {"mn", 25},
        {"fe", 26},
        {"co", 27},
        {"ni", 28},
        {"cu", 29},
        {"zn", 30},
        {"ga", 31},
        {"ge", 32},
        {"as", 33},
        {"se", 34},
        {"br", 35},
        {"kr", 36},
        {"rb", 37},
        {"sr", 38},
        {"y", 39},
        {"zr", 40},
        {"nb", 41},
        {"mo", 42},
        {"tc", 43},
        {"ru", 44},
        {"rh", 45},
        {"pd", 46},
        {"ag", 47},
        {"cd", 48},
        {"in", 49},
        {"sn", 50},
        {"sb", 51},
        {"te", 52},
        {"i", 53},
        {"xe", 54},
        {"cs", 55},
        {"ba", 56},
        {"la", 57},
        {"ce", 58},
        {"pr", 59},
        {"nd", 60},
        {"pm", 61},
        {"sm", 62},
        {"eu", 63},
        {"gd", 64},
        {"tb", 65},
        {"dy", 66},
        {"ho", 67},
        {"er", 68},
        {"tm", 69},
        {"yb", 70},
        {"lu", 71},
        {"hf", 72},
        {"ta", 73},
        {"w", 74},
        {"re", 75},
        {"os", 76},
        {"ir", 77},
        {"pt", 78},
        {"au", 79},
        {"hg", 80},
        {"tl", 81},
        {"pb", 82},
        {"bi", 83},
        {"po", 84},
        {"at", 85},
        {"rn", 86},
        {"fr", 87},
        {"ra", 88},
        {"ac", 89},
        {"th", 90},
        {"pa", 91},
        {"u", 92},
        {"np", 93},
        {"pu", 94},
        {"am", 95},
        {"cm", 96},
        {"bk", 97},
        {"cf", 98},
        {"es", 99},
        {"fm", 100},
        {"md", 101},
        {"no", 102},
        {"lr", 103},
        {"rf", 104},
        {"db", 105},
        {"sg", 106},
        {"bh", 107},
        {"hs", 108},
        {"mt", 109},
        {"ds", 110},
        {"rg", 111},
        {"cn", 112},
        {"nh", 113},
        {"fl", 114},
        {"mc", 115},
        {"lv", 116},
        {"ts", 117},
        {"og", 118}
    };

    /* Average bond lengths from: Tro, N. J. Chemistry: A Molecular Approach, Fourth edition.; Pearson: Boston, 2017. */
    QMap<QString, double> averageBondLengths = {
        {"h-h", 0.74},
        {"c-h", 1.1},
        {"h-n", 1.0},
        {"h-o", 0.97},
        {"h-s", 1.32},
        {"f-h", 0.92},
        {"cl-h", 1.27},
        {"br-h", 1.41},
        {"h-i", 1.61},
        {"c-c", 1.54},
        {"c-n", 1.47},
        {"c-o", 1.43},
        {"c-cl", 1.78},
        {"n-n", 1.45},
        {"n-o", 1.36},
        {"o-o", 1.45},
        {"f-f", 1.43},
        {"cl-cl", 1.99},
        {"br-br", 2.28},
        {"i-i", 2.66},
    };

    const ElementData &dataRefFromNumber(int num) {
        if ((num < 0) || (num > elements.size()))
            return elements.at(0);
        return elements.at(num);
    }
}

Element Element::fromAtomicNumber(int num)
{
    return Element(num);
}

Element Element::fromAbbr(QString abbreviation)
{
    return Element(elementAbbreviations.value(abbreviation.toLower(), 0));
}

double Element::estimateBondLength(Element b)
{
    QString aAbbr = abbr.toLower();
    QString bAbbr = b.abbr.toLower();

    QString query;
    if (aAbbr < bAbbr)
        query = aAbbr + "-" + bAbbr;
    else
        query = bAbbr + "-" + aAbbr;

    double avgLength = averageBondLengths.value(query, 0.0);
    if (avgLength != 0.0)
        return avgLength;

    return covalentRadius() + b.covalentRadius();
}

Element::Element() : number(0)
{
}

Element::Element(int num)
{
    auto data = dataRefFromNumber(num);
    number = data.number;
    abbr = data.abbr;
}

Element::Element(QString abbreviation)
{
    auto data = dataRefFromNumber(elementAbbreviations.value(abbreviation.toLower(), 0));
    number = data.number;
    abbr = data.abbr;
}

QString Element::name()
{
    return dataRefFromNumber(number).name;
}

int Element::group()
{
    return periodicGroups.value(number - 1, -1000);
}

double Element::averageMass()
{
    return dataRefFromNumber(number).mass;
}

double Element::empiricalRadius()
{
    return dataRefFromNumber(number).empiricalRadius;
}

double Element::covalentRadius()
{
    return dataRefFromNumber(number).covalentRadius;
}
