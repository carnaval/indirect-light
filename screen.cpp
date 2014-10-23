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
int lit = 1;
int multi = 1;
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
        lit = (lit + 1) % 2;
        break;
    case Qt::Key_M:
        multi = (multi + 1) % 2;
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
QGLShaderProgram* bounce_shader = 0;
QGLShaderProgram* light_shader = 0;
QGLShaderProgram* lightseg_shader = 0;
void Screen::initializeGL()
{
    initializeGLFunctions();
    glEnable(GL_TEXTURE_2D);
    std_shader = compileShader("../spom/std.vs", "../spom/std.fs");
    lit_shader = compileShader("../spom/std.vs", "../spom/lit.fs");
    bounce_shader = compileShader("../spom/std.vs", "../spom/bounce.fs");
    light_shader = compileShader("../spom/std.vs", "../spom/light.fs");
    lightseg_shader = compileShader("../spom/std.vs", "../spom/lightseg.fs");
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
    bounceFb = NULL;
    bounceFb = new QGLFramebufferObject(2048, 2048, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D,
                                        GL_RGBA32F);
                                        //GL_LUMINANCE_ALPHA32F_ARB);
    resizeFb(width(), height());
    if (!(bounceFb->isValid() && shadowFb->isValid())) {
        qDebug() << "Could not init fb";
        exit(1);
    }
    debugFont = QFont("Helvetica", 16, false, false);
    glEnable(GL_CULL_FACE);
    starts.append(QVector2D(-0.3, 0));
    ends.append(QVector2D(0.3, 0));
}
void
Screen::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    resizeFb(w,h);
}

void Screen::resizeFb(int w, int h)
{
    if (shadowFb != NULL) {
        shadowFb->release();
        delete shadowFb;
    }
    shadowFb = new QGLFramebufferObject(w, h, QGLFramebufferObject::CombinedDepthStencil, GL_TEXTURE_2D, GL_RGBA);
}

void Screen::glVertex(QVector3D v) { glVertex3f(v.x(),v.y(),v.z()); }

static float fsign(float f) { if(f >= 0) return 1.; return 0.; }
void Screen::bounceOnce(QVector2D a, QVector2D b, bool point)
{
    const int N = 64;
    QVector2D dir = b - a;
    light_shader->bind();

    light_shader->setUniformValue("origin", (a+b)/2);
    glClear(GL_STENCIL_BUFFER_BIT);
        //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilFunc(GL_ALWAYS, 0, 0xFF); glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
    //glCullFace(GL_FRONT);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

    for(int i=0; i < ends.length(); i++) {
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        QVector2D s = starts[i], e = ends[i];
        QVector3D projA, projB;
        if (point) {
            projA = QVector3D(s-a, 0);
            projB = QVector3D(e-a, 0);
        } else {
            QVector2D da = s-a, db = e-b, ba = a-b;
            QVector2D ab = (a-b);

            QVector2D nb(db.y(), -db.x()), na(-da.y(), da.x());
            float ga = QVector2D::dotProduct(nb, -ab),
                  gb = QVector2D::dotProduct(na, ab);
            nb *= fsign(ga);
            na *= fsign(gb);
            qDebug() << "G " << ga << gb;
            //nb.normalize(); na.normalize();
            ga = qAbs(ga); gb = qAbs(gb);
            float an = qMax(QVector2D::dotProduct(da, nb), 0.f),
                  bn = qMax(QVector2D::dotProduct(db, na), 0.f);
            projA = QVector3D(a*an + ga*da, an);
            projB = QVector3D(b*bn + gb*db, bn);
            /*projB = QVector3D(da, 0);
            projA = QVector3D(db, 0);*/
            qDebug() << "Proj " << projA << projB;
            lightseg_shader->bind();
            lightseg_shader->setUniformValue("screen", QVector2D(width(), height()));
            lightseg_shader->setUniformValue("col", QVector3D(1, 0, 0));
            lightseg_shader->setUniformValue("emitA", a);
            lightseg_shader->setUniformValue("emitB", b);
            lightseg_shader->setUniformValue("casterA", s);
            lightseg_shader->setUniformValue("casterB", e);
            if (multi) {
            glBegin(GL_QUADS);
            glVertex3f(-1, -1, 1);
            glVertex3f(1, -1, 1);
            glVertex3f(1, 1, 1);
            glVertex3f(-1, 1, 1);
            glEnd();
            continue;
            //break;
            }
            glBegin(GL_TRIANGLES);
            // partial1
            glVertex(QVector3D(s, 1));
            glVertex(QVector3D(s-a, 0));
            glVertex(QVector3D(s-b, 0));
            // partial2
            glVertex(QVector3D(e, 1));
            glVertex(QVector3D(e-a, 0));
            glVertex(QVector3D(e-b, 0));
            // full
            glVertex(QVector3D(s, 1));
            glVertex(QVector3D(e, 1));
            glVertex(projA);
            /*glVertex(QVector3D(s, 1));
            glVertex(projA);
            glVertex(QVector3D(e, 1));*/
            glVertex(QVector3D(e, 1));
                        glVertex(projB);
                        glVertex(projA);
            glEnd();
             /*lightseg_shader->setUniformValue("emitA", a);
            lightseg_shader->setUniformValue("emitB", b);
            lightseg_shader->setUniformValue("caster", e);
            glBegin(GL_TRIANGLES);
            glVertex(QVector3D(e, 1));
            glVertex(QVector3D(e-b, 0));
            glVertex(QVector3D(e-a, 0));
            glEnd();*/
            /*light_shader->setUniformValue("col", QVector3D(0, 0, 1));
            glBegin(GL_TRIANGLES);

            glEnd();*/
        }
        //qDebug() << a << b;
        light_shader->bind();
        light_shader->setUniformValue("col", QVector3D(1, 1, 1));
        /*glBegin(GL_LINES);
        glVertex(QVector3D(a,1));glVertex(projA);
        glEnd(); continue;*/

        /*glBegin(GL_TRIANGLES);
        glVertex3f(s.x(), s.y(), 1);
        glVertex3f(e.x(), e.y(), 1);
        glVertex(projA);

        glVertex3f(e.x(), e.y(), 1);
        glVertex(projB);
        glVertex(projA);
        glEnd();*/
    }
    glDisable(GL_BLEND);
    //glCullFace(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    /*glStencilFunc(GL_GEQUAL, point ? 0 : 0, 0xFF); glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glBegin(GL_QUADS);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);
    glEnd();*/

    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}
#include <QTime>
QTime frameTime;

void Screen::paintGL()
{
    float frameDt = frameTime.elapsed();
    frameTime.restart();
    glClearColor(0.95, 0.4, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);


    shadowFb->bind();
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, width(), height());

    /*glLineWidth(2);
    glBegin(GL_LINES);
    for(int i=0; i < ends.length(); i++) {
        glVertex3f(starts[i].x(), starts[i].y(), 1);
        glVertex3f(ends[i].x(), ends[i].y(), 1);
    }
    glEnd();
    glLineWidth(1);*/


    QVector2D m = mousePos();
    bounceOnce(lightStart, m, !pressed);

    // 2. compute bounce
    bounceFb->bind();
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    bounce_shader->bind();
    //std_shader->setUniformValue("col", QVector3D(1, 0, 0));
    glViewport(0, 0, 2048, 2048);
    glLineWidth(2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowFb->texture());
    glBegin(GL_LINES);
    for(int i=0; i < ends.length(); i++) {
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        QVector2D no(-(ey-sy), ex-sx);
        no.normalize(); no *= 0.001;
        sx += no.x(); ex += no.x(); sy += no.y(); ey += no.y();
        //glVertex3f(sx, sy, 1);
        //glVertex3f(ex, ey, 1);
        float x = (4*(float)i)/2048 - 1;
        float len = QVector2D(ex-sx, ey-sy).length();
        glVertexAttrib2f(1, sx, sy);
        glVertex3f(x, -1, 1);
        glVertexAttrib2f(1, ex, ey);
        glVertex3f(x, 1, 1);
    }
    glEnd();
    glLineWidth(1 );
    float mx[ends.length()], mn[ends.length()];

    float buf[4*2048];
    for (int i=0; i < ends.length(); i++) {
    glReadPixels(2*i, 0, 1, 2048, GL_RGBA, GL_FLOAT, buf);
    mx[i] = 0.; mn[i] = 1.;
    for(int p = 0; p < 4*2048; p += 4) {
        mx[i] = qMax(buf[p], mx[i]);
        mn[i] = qMin(buf[p+1], mn[i]);
//        qDebug() << "C: " << buf[i] << buf[i+1] << buf[i+2] << buf[i+3];
    }
    //qDebug() << "mx " << i << mx[i] << mn[i];
    }
    if (0 && multi > 0) {
    std_shader->bind();
shadowFb->bind();
glViewport(0, 0, width(), height());
    // make it rain
    for (int i=0; i < ends.length(); i++) {
        float M = mx[i], m = mn[i];
        if (M-m <= 0) continue;
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        QVector2D d(ex-sx, ey-sy);
        sx += m*d.x(); sy += m*d.y();
        ex -= (1-M)*d.x(); ey -= (1-M)*d.y();
        QVector2D no(-(ey-sy), ex-sx);
        no.normalize(); no *= 0.001;
        sx += no.x(); ex += no.x(); sy += no.y(); ey += no.y();
        bounceOnce(QVector2D(sx,sy), QVector2D(ex,ey),false);
    }
    }


    // 3. compose

    QGLFramebufferObject::bindDefault();
    glViewport(0, 0, width(), height());
    lit_shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lit == 2 ? bounceFb->texture() : shadowFb->texture());
    if (lit != 0) {
        glBegin(GL_QUADS);
        glVertex3f(-1, -1, 1);
        glVertex3f(1, -1, 1);
        glVertex3f(1, 1, 1);
        glVertex3f(-1, 1, 1);
        glEnd();
    } else {

    std_shader->setUniformValue("col", QVector3D(0, 0, 1));

    glLineWidth(4);
    glBegin(GL_LINES);
    /*for(int i=0; i < ends.length(); i++) {std_shader->bind();
        float M = mx[i], m = mn[i];
        if (M-m <= 0) continue;
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        QVector2D d(ex-sx, ey-sy);
        sx += m*d.x(); sy += m*d.y();
        ex -= (1-M)*d.x(); ey -= (1-M)*d.y();
        glVertex3f(sx, sy, 1);
        glVertex3f(ex, ey, 1);
    }*/

    for(int i=0; i < ends.length(); i++) {
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        QVector2D no(-(ey-sy), ex-sx);
        no.normalize(); no *= 0.001;
        sx += no.x(); ex += no.x(); sy += no.y(); ey += no.y();
        glVertex3f(sx, sy, 1);
        glVertex3f(ex, ey, 1);
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

    renderText(-0.99, -0.99, 0, QString("Bounce : %1 .. %2 ms/f").arg(multi).arg(QString::number(frameDt)), debugFont);
}

