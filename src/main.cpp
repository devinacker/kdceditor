#include <QtWidgets/QApplication>
#include "mainwindow.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    a.setApplicationName(INFO_NAME);
    a.setApplicationVersion(INFO_VERS);
    
    MainWindow w;
    w.show();
    
    return a.exec();
}
