#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

class QSettings;
class QLineEdit;
class QLabel;

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QSettings *settings, QWidget *parent = nullptr);
    ~SettingsDialog();

    QString mingwPath() const;
    QString opencvPath() const;
    QString pythonPath() const;
    QString automatorLibPath() const;
    QString scriptsPath() const;

private slots:
    void browseMingw();
    void browseOpencv();
    void browsePython();
    void browseAutomatorLib();
    void browseScripts();
    void validatePaths();
    void saveSettings();
    void restoreDefaults();

private:
    void setupUI();
    void loadSettings();

    QSettings *m_settings;

    QLineEdit *m_mingwEdit;
    QLineEdit *m_opencvEdit;
    QLineEdit *m_pythonEdit;
    QLineEdit *m_automatorLibEdit;
    QLineEdit *m_scriptsEdit;

    QLabel *m_statusLabel;
};

#endif // SETTINGSDIALOG_H