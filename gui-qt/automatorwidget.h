#ifndef AUTOMATORWIDGET_H
#define AUTOMATORWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QProcess>
#include <QTimer>
#include <QSettings>
#include <QUuid>
#include <QTextBrowser>

class AutomatorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AutomatorWidget(QWidget *parent = nullptr);
    ~AutomatorWidget();

public slots:
    void openSettings();

private:
    // UI элементы (в том же порядке, что и в конструкторе)
    QTabWidget *m_tabWidget;
    QTextEdit *m_editor;
    QPlainTextEdit *m_lineNumbers;
    QTextBrowser *m_outputBrowser;

    QPushButton *m_recordButton;
    QPushButton *m_loadButton;
    QPushButton *m_saveButton;
    QPushButton *m_startButton;
    QPushButton *m_stopButton;

    QLabel *m_statusLabel;
    QLabel *m_lineColLabel;

    QComboBox *m_fontSizeCombo;
    QComboBox *m_templateCombo;

    QMenuBar *m_menuBar;
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_runMenu;
    QMenu *m_settingsMenu;
    QMenu *m_recentFilesMenu;

    QProcess *m_scriptProcess;
    QTimer *m_recordingTimer;

    QSettings *m_settings;

    QString m_currentFile;
    QString m_pythonPath;
    QString m_tempPythonFile;
    QString m_mingwPath;
    QString m_opencvPath;
    QString m_automatorLibPath;
    QString m_scriptsPath;

    bool m_isRecording;
    bool m_isModified;

    QStringList m_recentFiles;

private:
    void setupUI();
    void setupMenu();
    void setupEditor();

    bool copyWrapperToTempDir();
    bool copyDllToTempDir();
    bool copyMingwDependenciesToTempDir();

    QString createTempPythonFile(const QString &script);
    void cleanupTempFile();

    void findPython();
    void loadSettings();
    void saveSettings();
    QString buildEnvironmentPath() const;

    void updateLineNumbers();
    void updateTitle();
    void updateRecentFilesMenu();

    void addToRecentFiles(const QString &fileName);
    void loadRecentFile();

    QString getLastDirectory() const;
    void setLastDirectory(const QString &path);
    void updateLastDirectory(const QString &filePath);

    QString getScriptTemplate(const QString &name);
    QString getFileExtension() const;

private slots:
    void runScript();
    void stopScript();
    void onScriptFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onScriptOutput();
    void onScriptError();

    void newScript();
    void loadScript();
    void loadScriptFile(const QString &fileName);
    void saveScript();
    void saveScriptAs();
    void saveScriptFile(const QString &fileName);

    void changeFontSize(int index);
    void insertTemplate();
    void toggleRecording(bool checked);
};

#endif // AUTOMATORWIDGET_H