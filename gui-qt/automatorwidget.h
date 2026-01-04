#ifndef AUTOMATORWIDGET_H
#define AUTOMATORWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QFileDialog>
#include <QMenuBar>
#include <QMenu>
#include <QComboBox>
#include <QTimer>
#include <QTabWidget>
#include <QTextBrowser>
#include <QDateTime>
#include <QTemporaryFile>
#include <QProcess>
#include <QDir>
#include <QUuid>
#include <QSettings>

// Основное окно приложения
class AutomatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AutomatorWidget(QWidget *parent = nullptr);
    ~AutomatorWidget();

private slots:
    // Файловые операции
    void newScript();
    void loadScript();
    void saveScript();
    void saveScriptAs();

    // Редактор
    void updateLineNumbers();
    void changeFontSize(int index);
    void insertTemplate();

    // Запись действий
    void toggleRecording(bool checked);

    // Выполнение скрипта
    void runScript();
    void stopScript();
    void onScriptFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onScriptOutput();
    void onScriptError();
    void findPython();
    void updateRecentFilesMenu();
    void loadRecentFile();

private:
    // Виджеты
    QTabWidget *m_tabWidget;
    QTextEdit *m_editor;
    QPlainTextEdit *m_lineNumbers;
    QTextBrowser *m_outputBrowser;
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
    QMenu *m_runMenu;
    QMenu *m_recentFilesMenu;

    // Дополнительные
    QProcess *m_scriptProcess;
    QTimer *m_recordingTimer;
    QSettings *m_settings;
    QString m_currentFile;
    QString m_pythonPath;
    QString m_tempPythonFile;
    QStringList m_recentFiles;
    bool m_isRecording;
    bool m_isModified;

    void setupUI();
    void setupMenu();
    void setupEditor();

    void loadScriptFile(const QString &fileName);
    void saveScriptFile(const QString &fileName);
    void updateTitle();
    void addToRecentFiles(const QString &fileName);

    // Вспомогательные функции
    QString getScriptTemplate(const QString &name);
    QString getFileExtension() const;
    void cleanupTempFile();
    bool copyWrapperToTempDir();
    QString createTempPythonFile(const QString &script);

    // Методы для работы с последней директорией
    QString getLastDirectory() const;
    void setLastDirectory(const QString &path);
    void updateLastDirectory(const QString &filePath);
};

#endif // AUTOMATORWIDGET_H