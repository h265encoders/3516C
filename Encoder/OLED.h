#ifndef OLED_H
#define OLED_H

#include <QWidget>
#include "LinkUI.h"
namespace Ui {
class OLED;
}

class OLED : public QWidget
{
    Q_OBJECT

public:
    explicit OLED(QWidget *parent = 0);
    ~OLED();
    void setText(QString ip,QString input,QString upload);

private:
    Ui::OLED *ui;
};

#endif // OLED_H
