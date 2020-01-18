#include "Gpio.h"
#include <QDebug>

Gpio::Gpio(QObject *parent) : QObject(parent)
{

}

void Gpio::init()
{
    count=0;
    gpio=Link::create("GPIO");
    gpio->start();
    if(gpio->invoke("registEvent","GPIO0_3").toBool())
    {
        connect(gpio,SIGNAL(newEvent(QString,QVariant)),this,SLOT(onGPIO(QString,QVariant)));
        connect(&timer,SIGNAL(timeout()),this,SLOT(onTimer()));
        timer.start(1000);
    }
}

void Gpio::onGPIO(QString type, QVariant info)
{
    QString name=info.toString();
    qDebug()<<type<<name;
}

void Gpio::onTimer()
{
    if(!gpio->invoke("getGPIO","GPIO0_3").toBool())
    {
//        qDebug()<<"Down";
        count++;
    }
    else
    {
//        qDebug()<<"Up";
        count=0;
    }

    if(count>=5)
    {
        qDebug("reset default config");
        system("/link/shell/reset.sh");
    }
}

