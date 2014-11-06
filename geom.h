#ifndef GEOM_H
#define GEOM_H

#include <QVector2D>
#include <QVector>

struct Geom {
    QVector<QVector2D> starts;
    QVector<QVector2D> ends;
};

#include <cassert>

inline int size(const Geom& g)
{
    assert(g.starts.length() == g.ends.length());
    return g.starts.length();
}

#endif // GEOM_H
