#ifndef GPIO_H
#define GPIO_H

#include <QObject>
#include "Link.h"
#include <QTimer>

class Gpio : public QObject
{
    Q_OBJECT
public:
    explicit Gpio(QObject *parent = 0);
    void init();
private:
    LinkObject *gpio;
    QTimer timer;
    int count;
signals:

public slots:
    void onGPIO(QString type, QVariant info);
    void onTimer();
};

#endif // GPIO_H
