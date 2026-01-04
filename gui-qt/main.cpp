#include "automatorwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Qt Automator");
    app.setOrganizationName("Automator");
    
    AutomatorWidget widget;
    widget.resize(1024, 768);
    widget.show();
    
    return app.exec();
}