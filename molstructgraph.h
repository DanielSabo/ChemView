#ifndef MOLSTRUCTGRAPH_H
#define MOLSTRUCTGRAPH_H

#include "molstruct.h"

class MolStructGraph
{
public:
    MolStructGraph() = default;
    MolStructGraph(MolStructGraph const &mol) = default;

    explicit MolStructGraph(MolStruct const &mol);

    struct Bond {
        int id;
        int partner;
    };

    struct Atom {
        QVector<Bond> bonds;
    };

    // Find the shortest path from "from" to "to" in terms of bonds traversed, with a maximum of "limit" intermediate nodes in the path
    QVector<int> findPath(int from, int to, int limit);
    // Return the ids of all atoms connected to "to" without passing through "from"
    QVector<int> selectBranch(int from, int to);

    QVector<Atom> atoms;
};

#endif // MOLSTRUCTGRAPH_H
