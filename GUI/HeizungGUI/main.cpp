#include "Heizung.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Heizung w;
    w.show();

    return a.exec();
}
