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
    void setStage(const QString& name);

private:
    void bounceOnce(QVector2D,QVector2D,bool,float);
    void glVertex(QVector3D);

    QVector2D mousePos();
    void resizeFb(int,int);

    QGLFramebufferObject* shadowFb;
    QGLFramebufferObject* bounceFb;
    QGLFramebufferObject* synthFb;
    QFont debugFont;
};

#endif // SCREEN_H
