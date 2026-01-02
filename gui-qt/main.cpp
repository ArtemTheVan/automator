#include <QApplication>
#include "automatorwidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Проверяем, доступны ли функции библиотеки
    // В реальном приложении можно добавить проверку загрузки DLL
    // или вызова простой тестовой функции
    
    AutomatorWidget window;
    window.setWindowTitle("Qt Automator v1.0 - Windows 10");
    window.resize(400, 300);
    window.show();
    
    return app.exec();
}