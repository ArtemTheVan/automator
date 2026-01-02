#ifndef AUTOMATORWIDGET_H
#define AUTOMATORWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThread>
#include <QMessageBox>
#include <QApplication>
#include <QScreen>

// Внешние C-функции из библиотеки automator
#ifdef __cplusplus
extern "C" {
#endif
    void simulate_keystroke(const char* text);
    void simulate_mouse_click_at(int x, int y);
    int capture_screen_region(int x, int y, int w, int h, const char* filename);
#ifdef __cplusplus
}
#endif

// Класс для выполнения автоматизации в отдельном потоке
class AutomationWorker : public QThread
{
    Q_OBJECT
    
protected:
    void run() override;
    
signals:
    void automationFinished();
    void statusChanged(const QString& message);
};

// Основное окно приложения
class AutomatorWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit AutomatorWidget(QWidget *parent = nullptr);
    
private slots:
    void startAutomation();
    void onAutomationFinished();
    void updateStatus(const QString& message);
    
private:
    QPushButton *m_startButton;
    QLabel *m_statusLabel;
    QLabel *m_infoLabel;
    AutomationWorker *m_worker;
    
    void setupUI();
};

#endif // AUTOMATORWIDGET_H