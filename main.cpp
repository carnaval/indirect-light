#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QVBoxLayout>
#include <QWidget>

#include "screen.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Screen s;
    QQuickView qmlView;
    qmlView.setSource(QUrl("qrc:/main.qml"));
    QWidget* container = QWidget::createWindowContainer(&qmlView);
    container->setMaximumHeight(200);
    QObject::connect(qmlView.engine(), SIGNAL(quit()), &s, SLOT(quit()));
    s.show();
    //container->show();
    return app.exec();
}
