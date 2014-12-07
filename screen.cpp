#include "screen.h"

#include "geom.h"
#include "save.h"
#include <QGLShaderProgram>
#include <QTimer>

Screen::Screen(QWidget *parent) :
    QGLWidget(parent)
{
}

Geom g;

/*QVector<QVector2D> starts;
QVector<QVector2D> ends;*/

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
int NICE = 0;
float I = 0.5f;
QVector2D lightStart;
QVector2D lightEnd;
bool pressed = false;
QVector<QVector2D> lbegin;
QVector<QVector2D> lend;
void Screen::mouseReleaseEvent(QMouseEvent*)
{
        pressed = false;
    if (polyBegin != -1) {
        QVector2D m = mousePos();
        g.ends.push_back(m); g.starts.push_back(m);
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
void Screen::wheelEvent(QWheelEvent* e)
{
    qDebug() << e->delta();
    I = qMax(qMin(I+e->delta()*(3*0.1f/120), 12.f), 0.f);
}

void Screen::keyPressEvent(QKeyEvent* e)
{
    QFile f("../spom/map.dmp");
    switch(e->key()) {
    case Qt::Key_Escape: exit(0);
    case Qt::Key_Space:
        if (polyBegin == -1) {
            polyBegin = g.starts.length();
            g.starts.push_back(mousePos());
        } else {
            g.ends.push_back(g.starts[polyBegin]);
            polyBegin = -1;
        }
        break;
    case Qt::Key_L:
        lit = (lit + 1) % 3;
        break;
    case Qt::Key_M:
        multi = (multi + 1) % 2;
        break;
    case Qt::Key_N:
        NICE = (NICE + 1) % 3;
        break;
    case Qt::Key_P: P = !P; break;
    case Qt::Key_S:
        lbegin.push_back(lightStart);
        lend.push_back(lightEnd);
        break;
    case Qt::Key_D:
        f.open(QIODevice::Text | QIODevice::ReadWrite);
    {
        QTextStream out(&f);
        out << g;
    }
        break;
    case Qt::Key_U:
        f.open(QIODevice::Text | QIODevice::ReadOnly);
    {
        QTextStream in(&f);
        in >> g;
    }
        break;
    }
}

void Screen::setStage(const QString &name)
{
    if (name == "raw") {
        lit = 0;
    } else if (name == "light") {
        lit = 1;
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
QGLShaderProgram* synthseg_shader = 0;
QGLShaderProgram* synthbal_shader = 0;
QGLShaderProgram* lightseg_shader = 0;
QGLShaderProgram* lightbal_shader = 0;
QGLShaderProgram* flat_shader = 0;
int lvl = 0;
float angle = 0;
#define BOUNCE_RES 128
#define MAX_WALL 256
void Screen::initializeGL()
{
    initializeGLFunctions();
    glEnable(GL_TEXTURE_2D);
    std_shader = compileShader("../spom/std.vs", "../spom/std.fs");
    lit_shader = compileShader("../spom/std.vs", "../spom/lit.fs");
    bounce_shader = compileShader("../spom/std.vs", "../spom/bounce.fs");
    light_shader = compileShader("../spom/std.vs", "../spom/light.fs");
    lightseg_shader = compileShader("../spom/std.vs", "../spom/lightseg.fs");
    lightbal_shader = compileShader("../spom/std.vs", "../spom/lightbal.fs");
    synthseg_shader = compileShader("../spom/std.vs", "../spom/synth_seg.fs");
    synthbal_shader = compileShader("../spom/std.vs", "../spom/synth_bal.fs");
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
    bounceFb = new QGLFramebufferObject(MAX_WALL, BOUNCE_RES, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D,
                                        GL_RGBA32F);
    bounceFbPong = new QGLFramebufferObject(MAX_WALL, BOUNCE_RES, QGLFramebufferObject::NoAttachment, GL_TEXTURE_2D,
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
        g.starts.append(QVector2D(-0.3, 0));
        g.ends.append(QVector2D(0.3, 0));
        /*starts.append(QVector2D(0.3, 0));
        ends.append(QVector2D(0.3, 0.5));*/
    } else if (lvl == 2) {
        const int N = 350;
        g.starts.fill(QVector2D(), N);
        g.ends.fill(QVector2D(), N);
        for (int i = 0; i < N; i++) {
            int prev = i==0?N-1:i-1;
            g.starts[i] = g.ends[prev] = cardioid(((float)i)/N-1);
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

static float fsign(float f) { if(f >= 0) return 1.; return -1.; }
static QVector3D mid(QVector3D a, QVector3D b) {
    return QVector3D(0.5f*(a.x() + b.x()), 0.5f*(a.y() + b.y()*a.z()), a.z()*b.z());
}
#define SEG 0
#define BAL 1
void Screen::bounceOnce(QVector2D a, QVector2D b, float I, int mode = SEG)
{
    glClear(GL_STENCIL_BUFFER_BIT);
    shadowFb->bind();
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);

    glViewport(0, 0, width(), height());    //glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glStencilFunc(GL_ALWAYS, 0, 0xFF); glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
//glCullFace(GL_FRONT);
    //glDisable(GL_CULL_FACE);g
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    QGLShaderProgram* lshader;
    if (mode == BAL)
        lshader = lightbal_shader;
    else if (mode == SEG)
        lshader = lightseg_shader;
    lshader->bind();
    lshader->setUniformValue("screen", QVector2D(width(), height()));
    lshader->setUniformValue("col", QVector3D(1, 0, 0));
    lshader->setUniformValue("il", I);

    QVector2D mid = .5f*(a+b);
    float len = (a-b).length();
    for(int i=0; i < g.ends.length(); i++) {
        float sx = g.starts[i].x(), sy = g.starts[i].y(),
              ex = g.ends[i].x(),   ey = g.ends[i].y();
        
        QVector2D s = g.starts[i], e = g.ends[i];
        QVector3D projA, projB;
        if (0) {
            projA = QVector3D(s-a, 0);
            projB = QVector3D(e-a, 0);
        } else {
            QVector2D se = e-s;
            /*if (mode == BAL) {
                QVector2D use = se.normalized();
                a = mid - .5f*len*use;
                b = mid + .5f*len*use;
            }*/
            lshader->setUniformValue("_emitA", a);
            lshader->setUniformValue("_emitB", b);
            QVector2D da = s-a, db = e-b, ba = a-b;
            QVector2D ab = (a-b);
            QVector2D n_ab = QVector2D(ab.y(), -ab.x());

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

            /*lshader->setUniformValue("casterA", s);
            lshader->setUniformValue("casterB", e);*/
            if (P || mode == BAL) {
                lshader->bind();
#define X glVertexAttrib4f(1, s.x(),s.y(),e.x(),e.y());
glBegin(GL_QUADS);
            X;glVertex3f(-1, -1, 1);
            X;glVertex3f(1, -1, 1);
            X;glVertex3f(1, 1, 1);
            X;glVertex3f(-1, 1, 1);
glEnd();

            continue;
            //break;
            }
            float sSide = QVector2D::dotProduct(s-a,n_ab),
                  eSide = QVector2D::dotProduct(e-a,n_ab);
            float aSide = QVector2D::dotProduct(a-s,n_se),
                  bSide = QVector2D::dotProduct(b-s,n_se);
            //qDebug() << aSide << bSide << sSide << eSide;
            if (sSide < 0 && eSide < 0){
                continue;
            }
            if (aSide < 0 && bSide < 0) {
                continue; }
            if (sSide < 0) {
                projA = QVector3D(s-a, 0);
                projB = QVector3D(a-b, 0);
            } else if (eSide < 0) {
                projA = QVector3D(b-a, 0);
                projB = QVector3D(e-b, 0);
            } else if (aSide < 0) {
                projA = QVector3D(e-s, 0);
                projB = QVector3D(e, 1);
               // projB = QVector3D(s-b, 0);
            }
            else if (bSide < 0) {
                projA = QVector3D(s, 1);
                projB = QVector3D(s-e, 0);
            }
            lshader->bind();
            glBegin(GL_TRIANGLES);
            // partial1
            /*X;glVertex(QVector3D(s, 1));
            X;glVertex(QVector3D(s-a, 0));
            X;glVertex(QVectorD(s-b, 0));*/

            X;glVertex(QVector3D(s, 1));
            X;glVertex(projA);
            X;glVertex(QVector3D(s-b,0));


            X;glVertex(QVector3D(e,1));
            X;glVertex(QVector3D(e-a,0));
            X;glVertex(projB);
            if (projA.distanceToPoint(projB) <= 0.001)
            { X;glVertex(projA); }
            else {
            X;glVertex(QVector3D(e-a,0.f)); }
            //qDebug() << "Pro " << projA << projB << mid(projA,projB);
            X;glVertex(QVector3D(e-a,0));
            X;glVertex(QVector3D(s-b,0));
                        /*X;glVertex(QVector3D(QVector2D(projA)-b, 0));
            X;glVertex(QVector3D(s-b, 0));
            X;glVertex(projA);*/

            // partial2
            /*X;glVertex(QVector3D(e, 1));
            X;glVertex(QVector3D(e-a, 0));
            X;glVertex(QVector3D(e-b, 0));*/
                /*X;glVertex(pa);
                X;glVertex(projB);
                X;glVertex(QVector3D(e, 1));*/
                
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
             /*lshader->setUniformValue("emitA", a);
            lshader->setUniformValue("emitB", b);
            lshader->setUniformValue("caster", e);
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
    QGLShaderProgram* s;
    if (mode == BAL) {
        s = synthbal_shader;
        s->bind();
        s->setUniformValue("a", mid);
        s->setUniformValue("r", len);
    } else if (mode == SEG) {
        s = synthseg_shader;
        s->bind();
        s->setUniformValue("a", a);
        s->setUniformValue("b", b);
    }
    s->setUniformValue("E", I);
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

QTime bounceTime;
void Screen::paintGL()
{
    float frameDt = frameTime.elapsed();
    float msPerBounce;
    angle = fmod(angle + frameDt * 0.002 , 2*M_PI);
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

    if (NICE == 2) {
        QVector2D mm = mousePos();
        lightStart = QVector2D(mm.x()-0.01,mm.y());
        lightEnd = QVector2D(mm.x()+0.01,mm.y());
    }


    if (NICE == 0 || NICE == 2)
        bounceOnce(lightStart, lightEnd, I, BAL);
    if (NICE >= 1) {
        for(int i = 0; i < lbegin.length(); i++) {
            QTransform t;
            QVector2D mid = 0.5*(lbegin[i] + lend[i]);
            t.translate(mid.x(),mid.y());

            t.rotateRadians(angle);
            t.translate(-mid.x(),-mid.y());
        QPointF a = t.map(lbegin[i].toPointF());
        QPointF b = t.map(lend[i].toPointF());
        bounceOnce(QVector2D(a.x(),a.y()),QVector2D(b.x(),b.y()), 1.5);
        }
    }
if(!multi)goto bounce_end;
    // 2. compute bounce
{
    bounceFb->bind();
    glViewport(0, 0, MAX_WALL, BOUNCE_RES);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    bounce_shader->bind();
    //std_shader->setUniformValue("col", QVector3D(1, 0, 0));

    glLineWidth(1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, synthFb->texture());
    glBegin(GL_LINES);
    for(int i=0; i < g.ends.length(); i++) {
        float sx = g.starts[i].x(), sy = g.starts[i].y(),
              ex = g.ends[i].x(),   ey = g.ends[i].y();
        QVector2D no(-(ey-sy), ex-sx);
        no.normalize(); no *= -0.001;
        sx += no.x(); ex += no.x(); sy += no.y(); ey += no.y();
        //glVertex3f(sx, sy, 1);
        //glVertex3f(ex, ey, 1);
        float x = (2*(float)i)/MAX_WALL - 1;
        float len = QVector2D(ex-sx, ey-sy).length();
        glVertexAttrib4f(1, sx, sy,0,0);
        glVertex3f(x, -1, 1);
        glVertexAttrib4f(1, ex, ey,0,0);
        glVertex3f(x, 1, 1);
    }
    glEnd();
    {
        QGLFramebufferObject* tmp = bounceFb;
        bounceFb = bounceFbPong;
        bounceFbPong = tmp;
    }
    glLineWidth(1 );
    float mx[g.ends.length()], mn[g.ends.length()], avg[g.ends.length()];

    __attribute__((aligned(64))) float buf[BOUNCE_RES][MAX_WALL][4];

    //g.ends.length();
bounceFb->bind();
bounceTime.restart();
    glReadPixels(0, 0, MAX_WALL, BOUNCE_RES, GL_RGBA, GL_FLOAT, buf);
    msPerBounce = (float)bounceTime.elapsed();
    for (int i=0; i < g.ends.length(); i++) {
        mx[i] = 0.; mn[i] = 1.; avg[i] = 0.;
    }
    for(int p = 0; p < BOUNCE_RES; p += 1) {
    for (int i=0; i < g.ends.length(); i++) {
        mx[i] = qMax(buf[p][i][0], mx[i]);
        mn[i] = qMin(buf[p][i][1], mn[i]);
        avg[i] += buf[p][i][2];
        //qDebug() << "C: " << buf[i] << buf[i+1] << buf[i+2] << buf[i+3];
    }
    //avg[i] = qMin(avg[i], 1.f);
    //qDebug() << "AVG" << avg[i];
    //avg[i] *= qAbs(QVector2D::dotProduct(starts[i]-ends[i], lightStart-lightEnd));
    //avg[i] *= ;
    //avg[i] /= 64*8;
    //avg[i] /= 16;
    }


    /*std_shader->bind();
synthFb->bind();*/
glViewport(0, 0, width(), height());
    // make it rain

    for (int i=0; i < g.ends.length(); i++) {
        avg[i] /= BOUNCE_RES;
        avg[i] *= g.starts[i].distanceToPoint(g.ends[i]);
        float M = mx[i], m = mn[i];
        if (M-m <= 0.1/* || avg[i] <= 0.05*/) continue;
        float sx = g.starts[i].x(), sy = g.starts[i].y(),
              ex = g.ends[i].x(),   ey = g.ends[i].y();
        QVector2D d(ex-sx, ey-sy);
        sx += m*d.x(); sy += m*d.y();
        ex -= (1-M)*d.x(); ey -= (1-M)*d.y();
        QVector2D no(-(ey-sy), ex-sx);
        no.normalize(); no *= -0.001;
        sx += no.x(); ex += no.x(); sy += no.y(); ey += no.y();
        bounceOnce(QVector2D(ex,ey), QVector2D(sx,sy),avg[i]);
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
    glBindTexture(GL_TEXTURE_2D, lit == 2 ? bounceFb->texture() : synthFb->texture());
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

    for(int i=0; i < g.ends.length(); i++) {
        float sx = g.starts[i].x(), sy = g.starts[i].y(),
              ex = g.ends[i].x(),   ey = g.ends[i].y();
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

    renderText(-0.99, -0.99, 0, QString("Bounce : %1 .. %2 ms/f .. %5 ms/b .. lit %3 .. P %4 .. W %6").arg(multi).arg(QString::number(frameDt)).arg(lit).arg(P).arg(QString::number(msPerBounce)).arg(g.ends.length()), debugFont);
}

