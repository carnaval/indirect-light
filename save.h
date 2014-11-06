#ifndef SAVE_H
#define SAVE_H

#include <QTextStream>
#include "geom.h"

QTextStream& operator<<(QTextStream& t, const QVector2D& v);
QTextStream& operator<<(QTextStream& t, const Geom& p);
QTextStream& operator>>(QTextStream& t, QVector2D& v);
QTextStream& operator>>(QTextStream& t, Geom& p);

#endif // SAVE_H
