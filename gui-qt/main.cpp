#include <QApplication>
#include "automatorwidget.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Устанавливаем название организации и приложения
    QApplication::setOrganizationName("Automator");
    QApplication::setApplicationName("Qt Automator Script Editor");
    
    // Устанавливаем стиль Fusion для современного вида
    app.setStyle("Fusion");
    
    // Темная тема (опционально)
    /*
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
    */
    
    AutomatorWidget window;
    window.setWindowTitle("Qt Automator - Редактор скриптов");
    window.resize(1024, 768);
    
    // Центрируем окно на экране
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - window.width()) / 2;
    int y = (screenGeometry.height() - window.height()) / 2;
    window.move(x, y);
    
    window.show();
    
    return app.exec();
}