#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QTimer>
#include "Link.h"
#include "maiaXmlRpcServer.h"
#include "OLED.h"


#define CFGPATH "/link/config/config.json"

class Worker : public QObject
{
    Q_OBJECT
public:
    explicit Worker(QObject *parent = 0);
    void init();
private:
    MaiaXmlRpcServer *rpcServer;
    QVariantMap config;

    LinkObject *inputA;
    LinkObject *encA;
    LinkObject *volume;

    LinkObject *inputV;
    LinkObject *overlay;
    LinkObject *adjust;
    LinkObject *snap;

    struct Profile{
        LinkObject *enc;
        QMap<QString,LinkObject*> mux;
        LinkObject *udp;
        LinkObject *push;
        bool pushEnable;
    };

    LinkObject *httpSvr;
    LinkObject *rtspSvr;

    QList<Profile> profileList;
    QList<LinkObject*> recordList;

    QString fileName;
    QDateTime startTime;
    QTimer timer;

    OLED oled;
signals:

public slots:
    bool update(QVariantMap data);
    QVariantMap pushSpeed();
    QVariantMap startPush();
    QVariantMap stopPush();

    QVariantMap getState();
    QVariantMap startRecord();
    QVariantMap stopRecord();

    QVariantMap sysState();
    QVariantMap netState();

    bool doSnap();

    void onTimer();
};

#endif // WORKER_H
