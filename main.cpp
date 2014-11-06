#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QQmlContext>
#include <QVBoxLayout>
#include <QWidget>
#include "save.h"
#include "screen.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Screen s;
    QQuickView qmlView;
    qmlView.engine()->rootContext()->setContextProperty("screen", &s);
    qmlView.setSource(QUrl("qrc:/main.qml"));
    /*QWidget* container = QWidget::createWindowContainer(&qmlView);
    container->setMaximumHeight(200);*/
    QObject::connect(qmlView.engine(), SIGNAL(quit()), &s, SLOT(quit()));

    qmlView.setMaximumWidth(200);
    qmlView.setFlags(Qt::Tool);
    qmlView.show();
    s.show();

    return app.exec();
}
