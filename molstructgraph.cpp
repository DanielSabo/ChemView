#include "molstructgraph.h"

MolStructGraph::MolStructGraph(const MolStruct &mol)
{
    atoms.resize(mol.atoms.size());
    for (int i = 0; i < mol.bonds.size(); ++i)
    {
        auto const &b = mol.bonds[i];
        atoms[b.from].bonds.push_back({i, b.to});
        atoms[b.to].bonds.push_back({i, b.from});
    }
}

QVector<int> MolStructGraph::findPath(int from, int to, int limit)
{
    QList<int> currentQueue;
    QList<int> nextQueue;
    QVector<int> visited; // Record the parent from which this node was visited
    visited.fill(-1, atoms.size());
    visited[from] = from;
    currentQueue.push_back(from);

    if (limit < 0)
        limit = atoms.size() + 1;

    int depth = 0;
    while ((depth < limit) && !currentQueue.isEmpty())
    {
        while (!currentQueue.isEmpty())
        {
            int a = currentQueue.takeFirst();
            for (auto const &b: atoms[a].bonds)
            {
                if (b.partner == to)
                {
                    // We found our target, now backtrack to get the path
                    QVector<int> result;
                    result.push_back(to);
                    for (int last = a; last != from; last = visited[last])
                    {
                        result.push_back(last);
                    }
                    result.push_back(from);
                    std::reverse(result.begin(), result.end());

                    return result;
                }
                else if (-1 == visited[b.partner])
                {
                    nextQueue.push_back(b.partner);
                    visited[b.partner] = a;
                }
            }
        }
        std::swap(currentQueue, nextQueue);
    }

    return {};
}

QVector<int> MolStructGraph::selectBranch(int from, int to)
{
    QVector<int> result;
    QVector<bool> visited;
    visited.resize(atoms.size());

    int bond = -1;
    for (auto const &b: atoms[from].bonds)
        if (b.partner == to)
            bond = b.id;

    if (bond < 0) // "from" is not connected to "to"
        return {};
    visited[from] = true;

    std::function<void(int)> visitAtom = [&](int id) {
        result.push_back(id);
        visited[id] = true;
        for (auto const &b: atoms[id].bonds)
            if (!visited[b.partner])
                visitAtom(b.partner);
    };

    visitAtom(to);

    return result;
}
