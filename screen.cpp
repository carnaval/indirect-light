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
int lit = 0;
int multi = 0;
int P = 0;
QVector2D lightStart;
QVector2D lightEnd;
bool pressed = false;
void Screen::mouseReleaseEvent(QMouseEvent*)
{
        pressed = false;
    if (polyBegin != -1) {
        QVector2D m = mousePos();
        ends.push_back(m); starts.push_back(m);
    } else {
        lightEnd = mousePos();
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
        lit = (lit + 1) % 3;
        break;
    case Qt::Key_M:
        multi = (multi + 1) % 2;
        break;
    case Qt::Key_P: P = !P; break;
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
#include <math.h>
QVector2D cardioid(float t) {
    t = t*2*M_PI;
    float a = 0.1;
    return 2*a*(1+(float)cos(t))*QVector2D((float)cos(t), (float)sin(t));
}

QGLShaderProgram* lit_shader = 0;
QGLShaderProgram* std_shader = 0;
QGLShaderProgram* bounce_shader = 0;
QGLShaderProgram* light_shader = 0;
QGLShaderProgram* synth_shader = 0;
QGLShaderProgram* lightseg_shader = 0;
QGLShaderProgram* flat_shader = 0;
int lvl = 1;
void Screen::initializeGL()
{
    initializeGLFunctions();
    glEnable(GL_TEXTURE_2D);
    std_shader = compileShader("../spom/std.vs", "../spom/std.fs");
    lit_shader = compileShader("../spom/std.vs", "../spom/lit.fs");
    bounce_shader = compileShader("../spom/std.vs", "../spom/bounce.fs");
    light_shader = compileShader("../spom/std.vs", "../spom/light.fs");
    lightseg_shader = compileShader("../spom/std.vs", "../spom/lightseg.fs");
    synth_shader = compileShader("../spom/std.vs", "../spom/synth.fs");
    flat_shader = compileShader("../spom/std.vs", "../spom/flat.fs");
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
    synthFb = NULL;
    bounceFb = NULL;
    bounceFb = new QGLFramebufferObject(2048, 2048, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D,
                                        GL_RGBA32F);
                                        //GL_LUMINANCE_ALPHA32F_ARB);
    resizeFb(width(), height());
    if (!(bounceFb->isValid() && shadowFb->isValid() && synthFb->isValid())) {
        qDebug() << "Could not init fb";
        exit(1);
    }
    debugFont = QFont("Helvetica", 16, false, false);
    glEnable(GL_CULL_FACE);
    if (lvl == 1) {
        starts.append(QVector2D(-0.3, 0));
        ends.append(QVector2D(0.3, 0));
        /*starts.append(QVector2D(0.3, 0));
        ends.append(QVector2D(0.3, 0.5));*/
    } else if (lvl == 2) {
        const int N = 350;
        starts.fill(QVector2D(), N);
        ends.fill(QVector2D(), N);
        for (int i = 0; i < N; i++) {
            int prev = i==0?N-1:i-1;
            starts[i] = ends[prev] = cardioid(((float)i)/N-1);
        }
    }
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
    if (synthFb != NULL) {
        synthFb->release();
        delete synthFb;
    }
    shadowFb = new QGLFramebufferObject(w, h, QGLFramebufferObject::CombinedDepthStencil, GL_TEXTURE_2D, GL_RG16F);
    synthFb = new QGLFramebufferObject(w, h, QGLFramebufferObject::CombinedDepthStencil, GL_TEXTURE_2D, GL_RGBA32F);
}

void Screen::glVertex(QVector3D v) { glVertex3f(v.x(),v.y(),v.z()); }

static float fsign(float f) { if(f >= 0) return 1.; return 0.; }
void Screen::bounceOnce(QVector2D a, QVector2D b, bool point, float I)
{
    const int N = 64;
    QVector2D dir = b - a;
    light_shader->bind();

    //light_shader->setUniformValue("origin", (a+b)/2);
    glClear(GL_STENCIL_BUFFER_BIT);
    shadowFb->bind();
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, width(), height());    //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        glStencilFunc(GL_ALWAYS, 0, 0xFF); glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
    //glCullFace(GL_FRONT);
        //glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        lightseg_shader->bind();
        lightseg_shader->setUniformValue("screen", QVector2D(width(), height()));
        lightseg_shader->setUniformValue("col", QVector3D(1, 0, 0));
        lightseg_shader->setUniformValue("emitA", a);
        lightseg_shader->setUniformValue("emitB", b);

        lightseg_shader->setUniformValue("il", I);


    for(int i=0; i < ends.length(); i++) {
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        float depth = (float)i/(ends.length()-1);
        QVector2D s = starts[i], e = ends[i];
        QVector3D projA, projB;
        if (point) {
            projA = QVector3D(s-a, 0);
            projB = QVector3D(e-a, 0);
        } else {
            QVector2D da = s-a, db = e-b, ba = a-b;
            QVector2D ab = (a-b);
            QVector2D n_ab = QVector2D(ab.y(), -ab.x());
            QVector2D se = e-s;
            QVector2D n_se = QVector2D(se.y(), -se.x());
            QVector2D nb(db.y(), -db.x()), na(-da.y(), da.x());
            float ga = QVector2D::dotProduct(nb, -ab),
                  gb = QVector2D::dotProduct(na, ab);
            nb *= fsign(ga);
            na *= fsign(gb);
            //nb.normalize(); na.normalize();
            ga = qAbs(ga); gb = qAbs(gb);
            float an = qMax(QVector2D::dotProduct(da, nb), 0.f),
                  bn = qMax(QVector2D::dotProduct(db, na), 0.f);
            projA = QVector3D(a*an + ga*da, an);
            projB = QVector3D(b*bn + gb*db, bn);
            /*projB = QVector3D(da, 0);
            projA = QVector3D(db, 0);*/

            /*lightseg_shader->setUniformValue("casterA", s);
            lightseg_shader->setUniformValue("casterB", e);*/
            if (P) {
                lightseg_shader->bind();
#define X glVertexAttrib4f(1, s.x(),s.y(),e.x(),e.y());
glBegin(GL_QUADS);
            X;glVertex3f(-1, -1, 1);
            X;glVertex3f(1, -1, 1);
            X;glVertex3f(1, 1, 1);
            X;glVertex3f(-1, 1, 1);
glEnd();

            if (P) continue;
            //break;
            }
            float sSide = QVector2D::dotProduct(s-a,n_ab),
                  eSide = QVector2D::dotProduct(e-a,n_ab);
            if (sSide < 0 &&
                eSide < 0) continue;
            /*if (QVector2D::dotProduct(a-s,n_se) <= 0 ||
                QVector2D::dotProduct(b-s,n_se) <= 0) continue;*/
            if (sSide < 0) projA = QVector3D(e-b, 0);
            if (eSide < 0) projB = QVector3D(s-a, 0);
            lightseg_shader->bind();
            glBegin(GL_TRIANGLES);
            // partial1
            X;glVertex(QVector3D(s, 1));
            X;glVertex(QVector3D(s-a, 0));
            X;glVertex(QVector3D(s-b, 0));
            // partial2
            X;glVertex(QVector3D(e, 1));
            X;glVertex(QVector3D(e-a, 0));
            X;glVertex(QVector3D(e-b, 0));
            glEnd();
            flat_shader->bind();
            flat_shader->setUniformValue("col", QVector3D(1,0, 0));
            glBegin(GL_TRIANGLES);
            // full

            glVertex(QVector3D(s, 1));
            glVertex(QVector3D(e, 1));
            glVertex(projA);
            glVertex(QVector3D(e, 1));
                        glVertex(projB);
                        glVertex(projA);
            glEnd();
            #undef X
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
        /*light_shader->bind();
        light_shader->setUniformValue("col", QVector3D(1, 1, 1));*/
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
//glEnd();
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
    synthFb->bind();
    synth_shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowFb->texture());
    glBegin(GL_QUADS);
    glVertex3f(-1, -1, 1);
    glVertex3f(1, -1, 1);
    glVertex3f(1, 1, 1);
    glVertex3f(-1, 1, 1);
    glEnd();
    glDisable(GL_BLEND);
}
#include <QTime>
QTime frameTime;

void Screen::paintGL()
{
    float frameDt = frameTime.elapsed();
    frameTime.restart();
    glClearColor(0.95, 0.4, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    synthFb->bind();
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    /*glLineWidth(2);
    glBegin(GL_LINES);
    for(int i=0; i < ends.length(); i++) {
        glVertex3f(starts[i].x(), starts[i].y(), 1);
        glVertex3f(ends[i].x(), ends[i].y(), 1);
    }
    glEnd();
    glLineWidth(1);*/



    bounceOnce(lightStart, lightEnd, pressed, 0.8);
if(!multi)goto bounce_end;
    // 2. compute bounce
{
    bounceFb->bind();
    glViewport(0, 0, 2048, 2048);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    bounce_shader->bind();
    //std_shader->setUniformValue("col", QVector3D(1, 0, 0));

    glLineWidth(2);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, shadowFb->texture());
    glBegin(GL_LINES);
    for(int i=0; i < ends.length(); i++) {
        float sx = starts[i].x(), sy = starts[i].y(),
              ex = ends[i].x(),   ey = ends[i].y();
        QVector2D no(-(ey-sy), ex-sx);
        no.normalize(); no *= -0.001;
        sx += no.x(); ex += no.x(); sy += no.y(); ey += no.y();
        //glVertex3f(sx, sy, 1);
        //glVertex3f(ex, ey, 1);
        float x = (4*(float)i)/2048 - 1;
        float len = QVector2D(ex-sx, ey-sy).length();
        glVertexAttrib4f(1, sx, sy,0,0);
        glVertex3f(x, -1, 1);
        glVertexAttrib4f(1, ex, ey,0,0);
        glVertex3f(x, 1, 1);
    }
    glEnd();
    glLineWidth(1 );
    float mx[ends.length()], mn[ends.length()], avg[ends.length()];

    float buf[4*2048];
    for (int i=0; i < ends.length(); i++) {
    glReadPixels(2*i, 0, 1, 2048, GL_RGBA, GL_FLOAT, buf);
    mx[i] = 0.; mn[i] = 1.; avg[i] = 0.;
    for(int p = 0; p < 4*2048; p += 4) {
        mx[i] = qMax(buf[p], mx[i]);
        mn[i] = qMin(buf[p+1], mn[i]);
        avg[i] += buf[p+2];
//        qDebug() << "C: " << buf[i] << buf[i+1] << buf[i+2] << buf[i+3];
    }

    avg[i] /= 2048;
    avg[i] = qMin(avg[i], 1.f);
    avg[i] *= qAbs(QVector2D::dotProduct(starts[i]-ends[i], lightStart-lightEnd));
    //avg[i] *= ;
    //avg[i] /= 64*8;
    //avg[i] /= 16;
    }

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
        no.normalize(); no *= -0.001;
        sx += no.x(); ex += no.x(); sy += no.y(); ey += no.y();
        bounceOnce(QVector2D(ex,ey), QVector2D(sx,sy),false,0.1f*avg[i]);
    }
}
bounce_end:
    // 3. compose

    QGLFramebufferObject::bindDefault();
    glViewport(0, 0, width(), height());
    if (lit == 1) {
        lit_shader->bind();
    } else std_shader->bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, lit == 2 ? shadowFb->texture() : synthFb->texture());
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

        glBegin(GL_LINES);
        glVertex3f(lightEnd.x(), lightEnd.y(), 1);
        glVertex3f(lightStart.x(), lightStart.y(), 1);
        glEnd();
    }

    renderText(-0.99, -0.99, 0, QString("Bounce : %1 .. %2 ms/f .. lit %3 .. P %4").arg(multi).arg(QString::number(frameDt)).arg(lit).arg(P), debugFont);
}

