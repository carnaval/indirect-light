#include "screen.h"

#include <QGLShaderProgram>
#include <QTimer>

Screen::Screen(QWidget *parent) :
    QGLWidget(parent)
{
}

QVector<QVector2D> starts;
QVector<QVector2D> ends;

QVector2D Screen::mousePos() {
    QPoint mouse(mapFromGlobal(QCursor::pos()));
    float x = ((float)mouse.x())/width(),
          y = 1. - ((float)mouse.y())/height();
    x = 2*x-1; y = 2*y-1;
    return QVector2D(x,y);
}

int polyBegin = -1;
bool lit = false;
QVector2D lightStart;
bool pressed = false;
void Screen::mouseReleaseEvent(QMouseEvent*)
{
    pressed = false;
    if (polyBegin != -1) {
        QVector2D m = mousePos();
        ends.push_back(m); starts.push_back(m);
    } else {

    }
}
void Screen::mousePressEvent(QMouseEvent*)
{
    pressed = true;
    if (polyBegin == -1) {
        lightStart = mousePos();
    }
}

void Screen::keyPressEvent(QKeyEvent* e)
{
    switch(e->key()) {
    case Qt::Key_Escape: exit(0);
    case Qt::Key_Space:
        if (polyBegin == -1) {
            polyBegin = starts.length();
            starts.push_back(mousePos());
        } else {
            ends.push_back(starts[polyBegin]);
            polyBegin = -1;
        }
        break;
    case Qt::Key_L:
        lit = !lit;
        break;
    }
}

void Screen::quit() { exit(0); }

const char* GLSL_PREFIX =
        "#version 440\n"
        "#extension GL_EXT_gpu_shader4 : enable\n";
QGLShaderProgram* compileShader(const QString& vpath, const QString& fpath)
{
    QGLShaderProgram* s = new QGLShaderProgram;
    QString v(GLSL_PREFIX);
    QFile vf(vpath); vf.open(QFile::ReadOnly);
    v += QString(vf.readAll());
    QString f(GLSL_PREFIX);
    QFile ff(fpath); ff.open(QFile::ReadOnly);
    f += QString(ff.readAll());
    s->addShaderFromSourceCode(QGLShader::Vertex, v);
    s->addShaderFromSourceCode(QGLShader::Fragment, f);
    s->link();
    if (!s->isLinked()) {
        qDebug() << "Shader failure " << s->log();
        exit(1);
    }
    return s;
}

QGLShaderProgram* lit_shader = 0;
QGLShaderProgram* std_shader = 0;
void Screen::initializeGL()
{
    initializeGLFunctions();
    glEnable(GL_TEXTURE_2D);
    std_shader = compileShader("../spom/std.vs", "../spom/std.fs");
    lit_shader = compileShader("../spom/std.vs", "../spom/lit.fs");
    //blur_shader = compileShader("../giess/a.vs", "../giess/prop2.fs");
    //ping = bindTexture(QImage("../giess/a.png"));
    //pong = bindTexture(QImage("../giess/a.png"));

    QTimer* frameTimer = new QTimer(this);
    connect(frameTimer, SIGNAL(timeout()), this, SLOT(update()));
    frameTimer->setInterval(0);
    frameTimer->start();
    glClearColor(0.95, 0.4, 0, 1);
    glEnable(GL_STENCIL_TEST);
    glStencilMask(0xFF);

    shadowFb = NULL;
}

void Screen::paintGL()
{
    if (!shadowFb) shadowFb = new QGLFramebufferObject(width(), height(), QGLFramebufferObject::CombinedDepthStencil, GL_TEXTURE_2D, GL_RGBA);
    glClear(GL_COLOR_BUFFER_BIT);
    std_shader->bind();

    shadowFb->bind();
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0.95, 0.4, 0, 1);
    glViewport(0, 0, width(), height());
    std_shader->setUniformValue("col", QVector3D(0, 0, 1));
    glLineWidth(2);
    glBegin(GL_LINES);
    for(int i=0; i < ends.length(); i++) {
        glVertex3f(starts[i].x(), starts[i].y(), 1);
        glVertex3f(ends[i].x(), ends[i].y(), 1);
    }
    glEnd();
    glLineWidth(1);
    std_shader->setUniformValue("col", QVector3D(0.1, 0.1, 0.1));
    glClear(GL_STENCIL_BUFFER_BIT);
    QVector2D m = mousePos();
    const int N = 8;
    QVector2D dir = lightStart - m;
    dir /= (N+1);
    for(int j=0; j <= N+1; j++) {
    //if(pressed) {
/*        int cm = j == 0 ? GL_FALSE : GL_TRUE;
        glColorMask(cm,cm,cm,cm);
        if (j == 0) { glStencilFunc(GL_ALWAYS, 0, 0xFF); glStencilOp(GL_INCR, GL_INCR, GL_INCR); }
        else { glStencilFunc(GL_LEQUAL, 1, 0xFF); glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); }*/
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilFunc(GL_EQUAL, j, 0xFF); glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
    //}
    glBegin(GL_TRIANGLES);
    for(int i=0; i < ends.length(); i++) {
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        glVertex3f(sx, sy, 1);
        glVertex3f(sx-m.x(), sy-m.y(), 0);
        glVertex3f(ex, ey, 1);
        glVertex3f(ex, ey, 1);
        glVertex3f(sx - m.x(), sy - m.y(), 0);
        glVertex3f(ex - m.x(), ey - m.y(), 0);
    }
    glEnd();
    glStencilFunc(GL_EQUAL, j, 0xFF); glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
    glLineWidth(2);
    glBegin(GL_LINES);
    for(int i=0; i < ends.length(); i++) {
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        glVertex3f(sx, sy, 1);
        glVertex3f(ex, ey, 1);
    }
    glEnd();
    glLineWidth(1);
    if (!pressed) break;
    m += dir;
    }
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glStencilFunc(GL_LESS, pressed ? (N+1) : 0, 0xFF); glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glBegin(GL_QUADS);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);
    glEnd();

    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    QGLFramebufferObject::bindDefault();
    lit_shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowFb->texture());
    if (lit) {
        glBegin(GL_QUADS);
        glVertex3f(-1, -1, 1);
        glVertex3f(1, -1, 1);
        glVertex3f(1, 1, 1);
        glVertex3f(-1, 1, 1);
        glEnd();
    } else {

    //std_shader->setUniformValue("col", QVector3D(0, 0, 1));
        glLineWidth(2);
    glBegin(GL_LINES);
    for(int i=0; i < ends.length(); i++) {
        glVertex3f(starts[i].x(), starts[i].y(), 1);
        glVertex3f(ends[i].x(), ends[i].y(), 1);
    }
    glEnd();
    glLineWidth(1 );

    std_shader->bind();
    std_shader->setUniformValue("col", QVector3D(1, 1, 0));
    if (pressed) {
        m = mousePos();
        glBegin(GL_LINES);
        glVertex3f(m.x(), m.y(), 1);
        glVertex3f(lightStart.x(), lightStart.y(), 1);
        glEnd();
    }
    }
}

void
Screen::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}
