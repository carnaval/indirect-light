#ifndef SCREEN_H
#define SCREEN_H

#include <QGLFunctions>
#include <QGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QVector2D>
#include <QGLFramebufferObject>

class Screen : public QGLWidget, private QGLFunctions
{
    Q_OBJECT
public:
    explicit Screen(QWidget *parent = 0);

    virtual void keyPressEvent(QKeyEvent*);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mousePressEvent(QMouseEvent*);

    void initializeGL();
    void paintGL();
    void resizeGL(int,int);
public slots:
    void quit();

private:
    QVector2D mousePos();

    QGLFramebufferObject* shadowFb;
};

#endif // SCREEN_H
