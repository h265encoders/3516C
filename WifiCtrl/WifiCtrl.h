#ifndef WIFICTRL_H
#define WIFICTRL_H

#include <QObject>
#include "maiaXmlRpcServer.h"
#include "Json.h"
#include <QTimer>

class WifiCtrl : public QObject
{
    Q_OBJECT
public:
    explicit WifiCtrl(QObject *parent = 0);
    MaiaXmlRpcServer *rpcServer;

    void Enable();
    void Disable();
    QProcess wpaProcess;
    QProcess dhcpProcess;
    void setIP();
    void startDhcp();
private:
    QString dev,ip,mask,gw,dns;
    QTimer timer;
    bool dhcp;
signals:

public slots:
    bool addWifi(QString ssid,QString passwd);
    QVariantList scanWifi();
    QVariantMap wifiStatu();
    QVariantList wifiList();
    bool setWifi(QString func, QString id);
    void dhcpDone();
    bool update(QVariantMap cfg);
    void onTimer();
};

#endif // WIFICTRL_H
