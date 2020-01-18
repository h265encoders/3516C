#include "Worker.h"
#include "Json.h"
#include <unistd.h>
#include <QProcess>
#include <QStringList>

Worker::Worker(QObject *parent) : QObject(parent)
{
}

void Worker::init()
{
    inputA=Link::create("InputAi");
    encA=Link::create("EncodeA");
    volume=Link::create("Volume");
    inputV=Link::create("InputVi");
    overlay=Link::create("Overlay");
    adjust=Link::create("AdjustV");
    snap=Link::create("EncodeV");


    httpSvr=Link::create("TSHttp");
    httpSvr->start();
    rtspSvr=Link::create("Rtsp");
    rtspSvr->start();

    update(Json::loadFile(CFGPATH).toMap());
    if(config["stream"].toMap()["push"].toMap()["autorun"].toBool())
        startPush();

    rpcServer = new MaiaXmlRpcServer(8080,this);
    rpcServer->addMethod("update", this, "update");
    rpcServer->addMethod("pushSpeed", this, "pushSpeed");
    rpcServer->addMethod("startPush", this, "startPush");
    rpcServer->addMethod("stopPush", this, "stopPush");
    rpcServer->addMethod("getState", this, "getState");
    rpcServer->addMethod("startRecord", this, "startRecord");
    rpcServer->addMethod("stopRecord", this, "stopRecord");
    rpcServer->addMethod("snap", this, "doSnap");
    rpcServer->addMethod("sysState", this, "sysState");
    rpcServer->addMethod("netState", this, "netState");

    connect(&timer,SIGNAL(timeout()),this,SLOT(onTimer()));
    timer.start(3000);
}

bool Worker::update(QVariantMap data)
{
    if(data.isEmpty())
        return false;
    config=data;
    Json::saveFile(config,CFGPATH);

    QVariantMap audio=config["audio"].toMap();
    {
        QVariantMap input=audio["input"].toMap();
        QVariantMap enc=audio["enc"].toMap();
        if(enc["codec"]=="close")
        {
            inputA->stop();
            encA->stop();
        }
        else
        {
            input["resamplerate"]=enc["samplerate"];
            inputA->start(input);
            encA->start(enc);
            inputA->linkA(encA);
            inputA->linkA(volume);
        }
    }

    QVariantMap video=config["video"].toMap();
    {
        inputV->start(video["input"].toMap());
        adjust->start(video["adjust"].toMap());
        QVariantMap overData;
        overData["lays"]=video["overlay"].toList();
        overlay->start(overData);
        snap->start(video["snap"].toMap());

        inputV->linkV(adjust)->linkV(overlay)->linkV(snap);

        QVariantList profile=video["profile"].toList();
        for(int i=0;i<profile.count();i++)
        {
            if(i>profileList.count()-1)
            {
                Profile pro;
                pro.enc=Link::create("EncodeV");

                QVariantMap path;
                if(encA->getState()!="started")
                    path["mute"]=true;
                else
                    path["mute"]=false;

                QMap<QString,LinkObject*> muxMap;
                muxMap["rtmp"]=Link::create("Mux");
                path["path"]="rtmp://127.0.0.1/live/stream"+QString::number(i);
                muxMap["rtmp"]->setData(path);
                muxMap["hls"]=Link::create("Mux");
                path["path"]="/tmp/hls/stream"+QString::number(i)+".m3u8";
                muxMap["hls"]->setData(path);

                muxMap["ts"]=Link::create("Mux");
                path["format"]="mpegts";
                path["path"]="mem://stream"+QString::number(i);
                muxMap["ts"]->setData(path);

                muxMap["rtsp"]=Link::create("Mux");
                path["format"]="rtsp";
                muxMap["rtsp"]->setData(path);
                muxMap["rtsp"]->linkV(rtspSvr);
                muxMap["rtsp"]->linkA(rtspSvr);
                pro.mux=muxMap;

                pro.udp=Link::create("TSUdp");
                muxMap["ts"]->linkV(pro.udp);
                overlay->linkV(pro.enc);


                pro.push=Link::create("Mux");
                muxMap["push"]=pro.push;

                foreach(QString key,muxMap.keys())
                {
                    if(encA->getState()=="started")
                        encA->linkA(muxMap[key]);
                    pro.enc->linkV(muxMap[key]);
                }

                profileList.append(pro);
            }
            if(profile[i].toMap()["enc"].toMap()["codec"]!="close")
                profileList[i].enc->start(profile[i].toMap()["enc"].toMap());
            else
                profileList[i].enc->stop();
        }

    }

    QVariantMap stream = config["stream"].toMap();
    {
        for(int i=0;i<profileList.count();i++)
        {
            Profile pro=profileList[i];
            QMap<QString,LinkObject*> muxMap=pro.mux;


            if(stream["rtmp"].toBool())
                muxMap["rtmp"]->start();
            else
                muxMap["rtmp"]->stop();

            if(stream["hls"].toBool())
                muxMap["hls"]->start();
            else
                muxMap["hls"]->stop();


            if(stream["http"].toBool()  || stream["udp"].toMap()["enable"].toBool())
                muxMap["ts"]->start();
            else
                muxMap["ts"]->stop();

            if(stream["rtsp"].toBool() )
                muxMap["rtsp"]->start();
            else
                muxMap["rtsp"]->stop();


            if(stream["http"].toBool())
                muxMap["ts"]->linkV(httpSvr);
            else
                muxMap["ts"]->unLinkV(httpSvr);

            if(stream["udp"].toMap()["enable"].toBool())
            {
                QVariantMap addr=stream["udp"].toMap();
                addr["port"]=addr["port"].toInt()+i;
                pro.udp->start(addr);
            }
            else
                pro.udp->stop();

            QVariantMap pushData = stream["push"].toMap()["profile"].toList()[i].toMap();
            profileList[i].pushEnable=pushData["enable"].toBool();
            pro.push->setData(pushData);
        }
    }

    return true;
}

QVariantMap Worker::pushSpeed()
{
    for(int i=0;i<profileList.count();i++)
    {
        if(profileList[i].pushEnable)
        {
            return profileList[i].push->invoke("getSpeed").toMap();
        }
    }
    return QVariantMap();
}

QVariantMap Worker::startPush()
{
    for(int i=0;i<profileList.count();i++)
    {
        if(profileList[i].pushEnable)
        {
            profileList[i].push->start();
        }
    }

    return getState();
}

QVariantMap Worker::stopPush()
{
    for(int i=0;i<profileList.count();i++)
    {
        profileList[i].push->stop();
    }
    return getState();
}

QVariantMap Worker::getState()
{
    QVariantMap ret;
    ret["push"]=(!pushSpeed().isEmpty());
    ret["record"]=(!fileName.isEmpty());
    ret["fileName"]=fileName;
    ret["startTime"]=startTime.toMSecsSinceEpoch();
    ret["duration"]=(QDateTime::currentDateTime().toMSecsSinceEpoch()-startTime.toMSecsSinceEpoch())/1000;
    return ret;
}

QVariantMap Worker::startRecord()
{
    QProcess pro;
    pro.start("df /files/");
    pro.waitForFinished(3000);
    QString str=pro.readAll();
//    qDebug()<<str;
    if(!str.contains("mmcblk") || !fileName.isEmpty())
        return getState();

    if(str.split("\n")[1].split(" ",QString::SkipEmptyParts)[1].toInt()<100000)
        return getState();

    startTime=QDateTime::currentDateTime();
    QVariantMap record=config["record"].toMap();
    QString fileDir=record["dir"].toString();
    QDir dir(fileDir);
    fileName="VIDEO-"+QString::number(dir.entryList(QStringList(), QDir::Dirs, QDir::Name).count()-2);
    QString cmd="mkdir "+fileDir+fileName;
    system(cmd.toLatin1().data());
    QString curPath=fileDir+fileName+"/"+fileName;
    QVariantList format=record["format"].toList();
    for(int i=0;i<format.count();i++)
    {
        QString extension=format[i].toString();
        QVariantMap dataMux;
        dataMux["path"]=curPath+"."+extension;

        if(recordList.count()<=i)
            recordList.append(Link::create("Mux"));

        if(encA->getState()!="started")
        {
            dataMux["mute"]=true;
            encA->unLinkA(recordList[i]);
        }
        else
        {
            dataMux["mute"]=false;
            encA->linkA(recordList[i]);
        }

        recordList[i]->start(dataMux);
        profileList[0].enc->linkV(recordList[i]);


    }

    snap->invoke("snapSync",curPath+".jpg");
    return getState();
}

QVariantMap Worker::stopRecord()
{
    for(int i=0;i<recordList.count();i++)
    {
        recordList[i]->stop();
    }
    fileName.clear();
    return getState();
}

QVariantMap Worker::sysState()
{
    QVariantMap ret;
    ret["hdmi"]=inputV->invoke("getReport");

    QProcess pro;
    pro.start("df /files/");
    pro.waitForFinished(5000);
    QString str=pro.readAll();
    if(!str.contains("mmc"))
        ret["disk"]=-1;
    else
    {
        int a=str.lastIndexOf("%");
        int b=str.lastIndexOf(" ",a-1)+1;

        ret["disk"]=str.mid(b,a-b);
    }

    return ret;
}

QVariantMap Worker::netState()
{
    static qlonglong lastRx=0;
    static qlonglong lastTx=0;
    static qint64 lastPTS=0;

    qint64 now=QDateTime::currentDateTime().toMSecsSinceEpoch();
    qint64 span=now-lastPTS;
    lastPTS=now;
    QFile file("/proc/net/dev");
    file.open(QFile::ReadOnly);
    QString line;
    for(int i=0;i<3;i++)
        line=file.readLine();

    line=file.readLine();
    qlonglong rx=line.split(' ',QString::SkipEmptyParts).at(1).toLongLong()*8;
    qlonglong tx=line.split(' ',QString::SkipEmptyParts).at(9).toLongLong()*8;
    file.close();

    int speedrx=(rx-lastRx)*1000/span/1024;
    int speedtx=(tx-lastTx)*1000/span/1024;
    lastRx=rx;
    lastTx=tx;

    if(span>1000)
    {
        speedrx=0;
        speedtx=0;
    }

    QVariantMap rt;
    rt["rx"]=speedrx;
    rt["tx"]=speedtx;
    return rt;
}

bool Worker::doSnap()
{
    snap->invoke("snap","/tmp/snap/snap.jpg");
    return true;
}

void Worker::onTimer()
{
    QString ip=Json::loadFile("/link/config/net.json").toMap()["ip"].toString();
    QFile file("/sys/class/net/eth0/operstate");
    file.open(QFile::ReadOnly);
    QString lanState=file.readLine();
    file.close();
    if(!lanState.startsWith("up"))
    {
        QString ssid=Json::loadFile("/link/config/ssid.json").toMap()["ssid"].toString();
        if(ssid!="null")
            ip=Json::loadFile("/link/config/wifi.json").toMap()["ip"].toString();
    }

    QVariantMap report=inputV->invoke("getReport").toMap();
    QString input="";
    if(report["avalible"].toBool())
    {
        input=QString::number(report["height"].toInt());
        input+=report["interlace"].toBool()?"I":"P";
        input+=QString::number(report["framerate"].toInt());
    }
    int speed=pushSpeed()["speed"].toInt()*8/1024;
    if(speed>9999)
        speed=9999;
    QString bitrate="";
    if(speed>0)
        bitrate=QString::number(speed)+"kb";

    oled.setText(ip,input,bitrate);
}


