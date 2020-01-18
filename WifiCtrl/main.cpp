#include <QCoreApplication>
#include "WifiCtrl.h"
#include "Json.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    WifiCtrl wifi;
    wifi.update(Json::loadFile("/link/config/wifi.json").toMap());
    return a.exec();
}

