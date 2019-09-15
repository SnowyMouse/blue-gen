#include "bluegenstone.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    BlueGenstone w;
    w.show();

    return a.exec();
}
