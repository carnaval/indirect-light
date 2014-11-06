#include "save.h"
QTextStream& operator<<(QTextStream& t, const Geom& p)
{
    t << "geom " << size(p) << " ";
    for(int i = 0; i < size(p); i++) {
        t << p.starts[i] << " " << p.ends[i] << " ";
    }
    return t;
}
QTextStream& operator<<(QTextStream& t, const QVector2D& v)
{
    t << v.x() << " " << v.y();
    return t;
}

QTextStream& operator>>(QTextStream& t, QVector2D& v)
{
    float x,y;
    t >> x >> y;
    v.setX(x); v.setY(y);
    return t;
}

QTextStream& operator>>(QTextStream& t, Geom& g)
{
    int size;
    QString hd;
    t >> hd >> size;
    assert(hd == "geom");
    QVector<QVector2D> st,ed;
    for(int i = 0; i < size; i++) {
        QVector2D s, e;
        t >> s >> e;
        st.push_back(s); ed.push_back(e);
    }
    g.starts = st;
    g.ends = ed;
    return t;
}
