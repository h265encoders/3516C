#include "OLED.h"
#include "ui_OLED.h"

OLED::OLED(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OLED)
{
    ui->setupUi(this);
    LinkUI::initOLED(this,"ZJY12832");
    ui->IP->setText("");
    ui->INPUT->setText("");
    ui->UPLOAD->setText("");
}

OLED::~OLED()
{
    delete ui;
}

void OLED::setText(QString ip, QString input, QString upload)
{
    ui->IP->setText(ip);
    ui->INPUT->setText(input);
    ui->UPLOAD->setText(upload);
    LinkUI::refreshOLED();
}

