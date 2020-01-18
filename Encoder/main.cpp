#include <QApplication>
#include <QTextCodec>
#include "Link.h"
#include "LinkUI.h"
#include "Worker.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    if(!Link::init())
        return 0;

    Worker w;
    w.init();

    return a.exec();
}

