#include "WifiCtrl.h"

WifiCtrl::WifiCtrl(QObject *parent) : QObject(parent)
{
    rpcServer = new MaiaXmlRpcServer(8081,this);
    rpcServer->addMethod("update", this, "update");
    rpcServer->addMethod("scanWifi", this, "scanWifi");
    rpcServer->addMethod("addWifi", this, "addWifi");
    rpcServer->addMethod("wifiList", this, "wifiList");
    rpcServer->addMethod("setWifi", this, "setWifi");
    dev="wlan0";
    connect(&dhcpProcess,SIGNAL(finished(int)),this,SLOT(dhcpDone()));
    connect(&timer,SIGNAL(timeout()),this,SLOT(onTimer()));
    timer.start(3000);
    dhcp=true;
}

void WifiCtrl::Enable()
{
    if(wpaProcess.isOpen())
        return;
    QStringList args;
    args << "-D" << "nl80211" << "-i" << dev << "-c" << "/link/config/wpa.conf";
    wpaProcess.start("wpa_supplicant",args);
    qDebug("Enable wifi");
}

void WifiCtrl::Disable()
{
    QProcess process;
    QStringList args;
    args << "-i" << dev << "terminate";
    process.start("wpa_cli",args);
    process.waitForFinished(5000);


    wpaProcess.terminate();
    wpaProcess.close();
    system("ifconfig wlan0 down");
    system("/link/shell/setNetwork.sh");
    qDebug("Disable wifi");
}

void WifiCtrl::setIP()
{
    QProcess process;
    QStringList args;
    args << dev << ip << "netmask" << mask;
    process.start("ifconfig",args);
    process.waitForFinished(5000);
    QString str="echo nameserver "+dns+" > /etc/resolv.conf";
    system(str.toLatin1().data());

    system("echo 1 >/proc/sys/net/ipv4/conf/all/arp_filter");
    system("ifconfig eth0 down");
    system("/link/shell/setNetwork.sh");
}

void WifiCtrl::startDhcp()
{
    if(dhcpProcess.state()==QProcess::Running)
        return;
    QStringList args;
    args << "-i" << dev << "-q" << "-s" << "/link/shell/dhcp.sh";
    dhcpProcess.start("udhcpc",args);
    qDebug("start dhcp");
}


bool WifiCtrl::addWifi(QString ssid, QString passwd)
{


    QStringList args;
    args << "-i" << dev << "add_network";

    QProcess process;
    process.start("wpa_cli",args);
    process.waitForFinished();

    QString id=process.readAll();

    qDebug()<<"res"<<id;

    args.clear();
    args << "-i" << dev << "set_network" << id << "ssid" << "\""+ssid+"\"";
    process.start("wpa_cli",args);
    process.waitForFinished();

    args.clear();
    args << "-i" << dev << "set_network" << id << "proto" << "RSN";
    process.start("wpa_cli",args);
    process.waitForFinished();


    if(passwd=="")
    {
        args.clear();
        args << "-i" << dev << "set_network" << id << "key_mgmt" << "NONE";
        process.start("wpa_cli",args);
        process.waitForFinished();
    }
    else
    {
        args.clear();
        args << "-i" << dev << "set_network" << id << "psk" << "\""+passwd+"\"";
        process.start("wpa_cli",args);
        process.waitForFinished();


        args.clear();
        args << "-i" << dev << "set_network" << id << "key_mgmt" << "WPA-PSK";
        process.start("wpa_cli",args);
        process.waitForFinished();
    }

    args.clear();
    args << "-i" << dev << "enable_network" << id;
    process.start("wpa_cli",args);
    process.waitForFinished();

    args.clear();
    args << "-i" << dev << "save_config";
    process.start("wpa_cli",args);
    process.waitForFinished();

    if(dhcp)
        startDhcp();
    else
        setIP();

    return true;
}

QVariantList WifiCtrl::scanWifi()
{
    QStringList args;
    args << "-i" << dev << "scan";
    QProcess process;
    process.start("wpa_cli",args);
    process.waitForFinished();

    args.clear();
    args << "-i" << dev << "scan_result";
    process.start("wpa_cli",args);
    process.waitForFinished();
    QString res=process.readAllStandardOutput();
    qDebug()<<"res"<<res;

    QStringList lines=res.split("\n",QString::SkipEmptyParts);
    QStringList ssids;
    QVariantList ret;
    QVariantList sort;
    foreach(QString line ,lines)
    {
        QStringList sp=line.split("\t",QString::SkipEmptyParts);
        if(sp.count()<5)
            continue;

        QString ssid=sp[4];
        QString flags=(sp[3]=="[ESS]"?"open":"wpa");
        int level=sp[2].toInt();
        QString frequency=(sp[1].toInt()>5000?"5G":"2.4G");

        QVariantMap data;
        data["ssid"]=ssid;
        data["flags"]=flags;
        data["level"]=level;
        data["frequency"]=frequency;

        sort.append(data);
    }
    for(int i=0;i<sort.count();i++)
    {
        int j;
        for(j=0;j<ret.count();j++)
            if(sort[i].toMap()["level"].toInt()>ret[j].toMap()["level"].toInt())
            {
                ret.insert(j,sort[i]);
                break;
            }

        if(j==ret.count())
            ret.append(sort[i]);
    }

    for(int i=0;i<ret.count();i++)
    {
        if(ssids.contains(ret[i].toMap()["ssid"].toString()))
        {
            ret.removeAt(i);
            i--;
        }
        ssids.append(ret[i].toMap()["ssid"].toString());
    }

    for(int i=0;i<ret.count();i++)
    {
        qDebug()<<ret[i].toMap()["ssid"].toString()<<ret[i].toMap()["level"].toInt();
    }

    return ret;
}

QVariantMap WifiCtrl::wifiStatu()
{
    QVariantMap ret;
    QProcess process;
    QStringList args;
    args<<dev;
    process.start("iwconfig",args);
    process.waitForFinished();

    QString res=process.readAllStandardOutput();
    int a=res.indexOf("ESSID:\"");
    if(a>0)
    {
        a+=7;
        int b=res.indexOf("\"",a);
        QString ssid=res.mid(a,b-a);
        ret["ssid"]=ssid;
    }
    else
    {
        ret["ssid"]="null";
    }

    a=res.indexOf("Quality=");
    a+=8;
    int b=res.indexOf("/",a);
    QString quality=res.mid(a,b-a);
    ret["quality"]=quality.toInt();

    return ret;
}

QVariantList WifiCtrl::wifiList()
{
    QVariantList ret;
    QStringList args;
    args << "-i" << dev << "list_network";
    QProcess process;
    process.start("wpa_cli",args);
    process.waitForFinished();

    QString res=process.readAllStandardOutput();
    QStringList lines=res.split("\n",QString::SkipEmptyParts);
    foreach(QString line,lines)
    {
        if(line.startsWith("network"))
            continue;
        QStringList sp=line.split("\t");
        QVariantMap map;
        map["id"]=sp[0].toInt();
        map["ssid"]=sp[1];
        map["flags"]=sp[3];
        ret.append(map);

    }


    return ret;
}

bool WifiCtrl::setWifi(QString func, QString id)
{
    qDebug()<<func<<id;
    QStringList args;
    args << "-i" << dev << func << id;
    QProcess process;
    process.start("wpa_cli",args);
    process.waitForFinished();

    args.clear();
    args << "-i" << dev << "save_config";
    process.start("wpa_cli",args);
    process.waitForFinished();

    if(dhcp)
        startDhcp();
    else
        setIP();

    return true;
}

void WifiCtrl::dhcpDone()
{
    QString res=dhcpProcess.readAllStandardOutput();
    int a,b;
    a=res.indexOf("dhcp_result");
    if(a<0)
    {
        qDebug("dhcp error");
        startDhcp();
        return;
    }
    res=res.mid(a);
    a=res.indexOf("ip:")+3;
    b=res.indexOf("\n",a);
    ip=res.mid(a,b-a);

    a=res.indexOf("mask:")+5;
    b=res.indexOf("\n",a);
    mask=res.mid(a,b-a);

    a=res.indexOf("gw:")+3;
    b=res.indexOf("\n",a);
    gw=res.mid(a,b-a);

    a=res.indexOf("dns:")+4;
    b=res.indexOf(" ",a);
    if(b<0)
        b=res.indexOf("\n",a);
    dns=res.mid(a,b-a);
    qDebug("dhcp done %s",ip.toLatin1().data());

    QVariantMap data = Json::loadFile("/link/config/wifi.json").toMap();

    data["ip"]=ip;
    data["mask"]=mask;
    data["gw"]=gw;
    data["dns"]=dns;
    Json::saveFile(data,"/link/config/wifi.json");
    setIP();
}

bool WifiCtrl::update(QVariantMap cfg)
{
    qDebug()<<cfg;
    if(!QFile::exists("/sys/class/net/wlan0/operstate"))
        return false;

    Json::saveFile(cfg,"/link/config/wifi.json");
    ip=cfg["ip"].toString();
    mask=cfg["mask"].toString();
    gw=cfg["gw"].toString();
    dns=cfg["dns"].toString();

    bool enable=cfg["enable"].toBool();
    dhcp=cfg["dhcp"].toBool();
    if(enable)
    {

        Enable();
        if(dhcp)
            startDhcp();
        else
            setIP();
    }
    else
    {
        Disable();
        system("/link/shell/setNetwork.sh");
    }
    return true;
}

void WifiCtrl::onTimer()
{
    QVariantMap ret;
    QProcess process;
    QStringList args;
    args<<dev;
    process.start("iwconfig",args);
    process.waitForFinished();

    QString res=process.readAllStandardOutput();
    int a=res.indexOf("ESSID:\"");
    if(a>0)
    {
        a+=7;
        int b=res.indexOf("\"",a);
        QString ssid=res.mid(a,b-a);
        ret["ssid"]=ssid;

        a=res.indexOf("Quality=");
        a+=8;
        b=res.indexOf("/",a);
        QString quality=res.mid(a,b-a);
        ret["quality"]=quality.toInt();

    }
    else
    {
        ret["ssid"]="null";
        ret["quality"]=0;
    }



    Json::saveFile(ret,"/link/config/ssid.json");

}
