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
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QFileDialog>
#include <QTextStream>
#include <QMenuBar>
#include <QMenu>
#include <QComboBox>
#include <QTimer>

// Внешние C-функции из библиотеки automator
#ifdef __cplusplus
extern "C" {
#endif
    void simulate_keystroke(const char* text);
    void simulate_mouse_click_at(int x, int y);
    void simulate_mouse_sequence(void* actions, int count);
    int capture_screen_region(int x, int y, int w, int h, const char* filename);
#ifdef __cplusplus
}
#endif

// Класс для выполнения автоматизации в отдельном потоке
class AutomationWorker : public QThread
{
    Q_OBJECT
    
public:
    explicit AutomationWorker(const QString& script) : m_script(script) {}
    
protected:
    void run() override;
    
signals:
    void automationFinished();
    void statusChanged(const QString& message);
    
private:
    QString m_script;
    
    void executeCommand(const QString& command);
};

// Основное окно приложения
class AutomatorWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit AutomatorWidget(QWidget *parent = nullptr);
    
private slots:
    void startAutomation();
    void stopAutomation();
    void onAutomationFinished();
    void updateStatus(const QString& message);
    
    // Файловые операции
    void newScript();
    void loadScript();
    void saveScript();
    void saveScriptAs();
    
    // Редактор
    void updateLineNumbers();
    void showLineNumbers(bool show);
    void changeFontSize(int size);
    void insertTemplate();
    
    // Запись действий
    void toggleRecording(bool checked);
    
private:
    // Виджеты
    QTextEdit *m_editor;
    QPlainTextEdit *m_lineNumbers;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;
    QPushButton *m_recordButton;
    QPushButton *m_loadButton;
    QPushButton *m_saveButton;
    QLabel *m_statusLabel;
    QLabel *m_lineColLabel;
    QComboBox *m_fontSizeCombo;
    QComboBox *m_templateCombo;
    
    // Меню
    QMenuBar *m_menuBar;
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_toolsMenu;
    
    // Дополнительные
    AutomationWorker *m_worker;
    QTimer *m_recordingTimer;
    QString m_currentFile;
    bool m_isRecording;
    bool m_isModified;
    
    void setupUI();
    void setupMenu();
    void setupEditor();
    
    void loadScriptFile(const QString& fileName);
    void saveScriptFile(const QString& fileName);
    void updateTitle();
    
    // Вспомогательные функции
    QString getScriptTemplate(const QString& name);
    void highlightCurrentLine(int lineNumber);
};

#endif // AUTOMATORWIDGET_H