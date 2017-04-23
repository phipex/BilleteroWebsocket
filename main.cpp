#include <QtCore/QCoreApplication>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCommandLineOption>
#include "echoserver.h"
#include "ID003.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription("QtWebSockets example: echoserver");
    parser.addHelpOption();

    QCommandLineOption dbgOption(QStringList() << "d" << "debug",
            QCoreApplication::translate("main", "Debug output [default: off]."));
    parser.addOption(dbgOption);
    QCommandLineOption portOption(QStringList() << "p" << "port",
            QCoreApplication::translate("main", "Port for echoserver [default: 1234]."),
            QCoreApplication::translate("main", "port"), QLatin1Literal("1234"));
    parser.addOption(portOption);
    parser.process(a);
    bool debug = parser.isSet(dbgOption);
    int port = parser.value(portOption).toInt();

    EchoServer *server = new EchoServer(port, true);
    
    
    QString sPortPath = ID003::billfoldPortPath();
    ID003 *billetero = new ID003(sPortPath);


    QObject::connect(server, &EchoServer::closed, &a, &QCoreApplication::quit);
    QObject::connect(billetero, &ID003::billDetected, server, &EchoServer::sendIntMessage);

    return a.exec();
}