#include "settingsdialog.h"
#include <QSettings>
#include <QFormLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QProcess>

SettingsDialog::SettingsDialog(QSettings *settings, QWidget *parent)
    : QDialog(parent), m_settings(settings),
      m_mingwEdit(nullptr), m_opencvEdit(nullptr), m_pythonEdit(nullptr),
      m_automatorLibEdit(nullptr), m_scriptsEdit(nullptr), m_statusLabel(nullptr)
{
    setWindowTitle("Settings");
    setMinimumWidth(600);

    setupUI();
    loadSettings();
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Paths group
    QGroupBox *pathsGroup = new QGroupBox("Dependency Paths");
    QFormLayout *pathsLayout = new QFormLayout;

    // MinGW path
    m_mingwEdit = new QLineEdit;
    QPushButton *mingwBrowseBtn = new QPushButton("Browse...");
    connect(mingwBrowseBtn, &QPushButton::clicked, this, &SettingsDialog::browseMingw);
    QHBoxLayout *mingwLayout = new QHBoxLayout;
    mingwLayout->addWidget(m_mingwEdit);
    mingwLayout->addWidget(mingwBrowseBtn);
    pathsLayout->addRow("MinGW Path (bin):", mingwLayout);

    // OpenCV path
    m_opencvEdit = new QLineEdit;
    QPushButton *opencvBrowseBtn = new QPushButton("Browse...");
    connect(opencvBrowseBtn, &QPushButton::clicked, this, &SettingsDialog::browseOpencv);
    QHBoxLayout *opencvLayout = new QHBoxLayout;
    opencvLayout->addWidget(m_opencvEdit);
    opencvLayout->addWidget(opencvBrowseBtn);
    pathsLayout->addRow("OpenCV Path (bin):", opencvLayout);

    // Python path
    m_pythonEdit = new QLineEdit;
    QPushButton *pythonBrowseBtn = new QPushButton("Browse...");
    connect(pythonBrowseBtn, &QPushButton::clicked, this, &SettingsDialog::browsePython);
    QHBoxLayout *pythonLayout = new QHBoxLayout;
    pythonLayout->addWidget(m_pythonEdit);
    pythonLayout->addWidget(pythonBrowseBtn);
    pathsLayout->addRow("Python Path:", pythonLayout);

    // Automator library
    m_automatorLibEdit = new QLineEdit;
    QPushButton *libBrowseBtn = new QPushButton("Browse...");
    connect(libBrowseBtn, &QPushButton::clicked, this, &SettingsDialog::browseAutomatorLib);
    QHBoxLayout *libLayout = new QHBoxLayout;
    libLayout->addWidget(m_automatorLibEdit);
    libLayout->addWidget(libBrowseBtn);
    pathsLayout->addRow("libautomator.dll Path:", libLayout);

    // Scripts
    m_scriptsEdit = new QLineEdit;
    QPushButton *scriptsBrowseBtn = new QPushButton("Browse...");
    connect(scriptsBrowseBtn, &QPushButton::clicked, this, &SettingsDialog::browseScripts);
    QHBoxLayout *scriptsLayout = new QHBoxLayout;
    scriptsLayout->addWidget(m_scriptsEdit);
    scriptsLayout->addWidget(scriptsBrowseBtn);
    pathsLayout->addRow("Scripts Path:", scriptsLayout);

    pathsGroup->setLayout(pathsLayout);
    mainLayout->addWidget(pathsGroup);

    // Status
    m_statusLabel = new QLabel("Specify paths to required dependencies");
    m_statusLabel->setStyleSheet("QLabel { padding: 5px; background-color: #f0f0f0; }");
    mainLayout->addWidget(m_statusLabel);

    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                       QDialogButtonBox::Cancel |
                                                       QDialogButtonBox::RestoreDefaults);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(buttonBox->button(QDialogButtonBox::RestoreDefaults), &QPushButton::clicked,
            this, &SettingsDialog::restoreDefaults);

    mainLayout->addWidget(buttonBox);

    // Connect validation signals
    connect(m_mingwEdit, &QLineEdit::textChanged, this, &SettingsDialog::validatePaths);
    connect(m_pythonEdit, &QLineEdit::textChanged, this, &SettingsDialog::validatePaths);
    connect(m_automatorLibEdit, &QLineEdit::textChanged, this, &SettingsDialog::validatePaths);
}

void SettingsDialog::loadSettings()
{
    m_mingwEdit->setText(m_settings->value("mingwPath", "C:/msys64/mingw64/bin").toString());
    m_opencvEdit->setText(m_settings->value("opencvPath", "C:/msys64/mingw64/bin").toString());
    m_pythonEdit->setText(m_settings->value("pythonPath", "python").toString());
    m_automatorLibEdit->setText(m_settings->value("automatorLibPath",
                                                  "D:/Projects/automator/lib/libautomator.dll")
                                    .toString());
    m_scriptsEdit->setText(m_settings->value("scriptsPath",
                                             "D:/Projects/automator/scripts")
                               .toString());

    validatePaths();
}

QString SettingsDialog::mingwPath() const { return m_mingwEdit->text(); }
QString SettingsDialog::opencvPath() const { return m_opencvEdit->text(); }
QString SettingsDialog::pythonPath() const { return m_pythonEdit->text(); }
QString SettingsDialog::automatorLibPath() const { return m_automatorLibEdit->text(); }
QString SettingsDialog::scriptsPath() const { return m_scriptsEdit->text(); }

void SettingsDialog::browseMingw()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select MinGW Directory (bin)",
                                                    m_mingwEdit->text());
    if (!dir.isEmpty())
    {
        m_mingwEdit->setText(QDir::toNativeSeparators(dir));
        validatePaths();
    }
}

void SettingsDialog::browseOpencv()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select OpenCV Directory (bin)",
                                                    m_opencvEdit->text());
    if (!dir.isEmpty())
    {
        m_opencvEdit->setText(QDir::toNativeSeparators(dir));
    }
}

void SettingsDialog::browsePython()
{
    QString file = QFileDialog::getOpenFileName(this, "Select Python Executable",
                                                m_pythonEdit->text(),
                                                "Executables (*.exe);;All files (*.*)");
    if (!file.isEmpty())
    {
        m_pythonEdit->setText(QDir::toNativeSeparators(file));
        validatePaths();
    }
}

void SettingsDialog::browseAutomatorLib()
{
    QString file = QFileDialog::getOpenFileName(this, "Select libautomator.dll",
                                                m_automatorLibEdit->text(),
                                                "Dynamic Libraries (*.dll);;All files (*.*)");
    if (!file.isEmpty())
    {
        m_automatorLibEdit->setText(QDir::toNativeSeparators(file));
        validatePaths();
    }
}

void SettingsDialog::browseScripts()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Scripts Directory",
                                                    m_scriptsEdit->text());
    if (!dir.isEmpty())
    {
        m_scriptsEdit->setText(QDir::toNativeSeparators(dir));
    }
}

void SettingsDialog::validatePaths()
{
    QString status;
    QStringList issues;

    // Check MinGW
    if (!m_mingwEdit->text().isEmpty())
    {
        QDir mingwDir(m_mingwEdit->text());
        if (!mingwDir.exists())
        {
            issues << "MinGW path does not exist";
        }
        else
        {
            // Check for required DLLs
            QStringList requiredDlls = {"libgcc_s_seh-1.dll", "libstdc++-6.dll", "libwinpthread-1.dll"};
            QStringList missingDlls;
            for (const QString &dll : requiredDlls)
            {
                if (!QFile::exists(mingwDir.filePath(dll)))
                {
                    missingDlls << dll;
                }
            }
            if (!missingDlls.isEmpty())
            {
                issues << QString("Missing DLLs in MinGW: %1").arg(missingDlls.join(", "));
            }
        }
    }

    // Check Python
    if (!m_pythonEdit->text().isEmpty())
    {
        QFileInfo pythonFile(m_pythonEdit->text());
        if (pythonFile.fileName().contains("python", Qt::CaseInsensitive))
        {
            if (!pythonFile.exists())
            {
                issues << "Python file not found";
            }
        }
        else
        {
            // Try to find python in PATH
            QProcess process;
            process.start("python", {"--version"});
            if (!process.waitForFinished(1000) || process.exitCode() != 0)
            {
                issues << "Python not found in PATH and specified file doesn't look like python";
            }
        }
    }

    // Check library
    if (!m_automatorLibEdit->text().isEmpty())
    {
        QFileInfo libFile(m_automatorLibEdit->text());
        if (!libFile.exists())
        {
            issues << "libautomator.dll file not found";
        }
        else if (libFile.suffix().toLower() != "dll")
        {
            issues << "Selected file is not a DLL";
        }
    }

    if (issues.isEmpty())
    {
        status = "✓ All paths are valid";
        m_statusLabel->setStyleSheet("QLabel { padding: 5px; background-color: #d4edda; color: #155724; }");
    }
    else
    {
        status = "⚠ Issues:\n" + issues.join("\n");
        m_statusLabel->setStyleSheet("QLabel { padding: 5px; background-color: #f8d7da; color: #721c24; }");
    }

    m_statusLabel->setText(status);
}

void SettingsDialog::saveSettings()
{
    m_settings->setValue("mingwPath", m_mingwEdit->text());
    m_settings->setValue("opencvPath", m_opencvEdit->text());
    m_settings->setValue("pythonPath", m_pythonEdit->text());
    m_settings->setValue("automatorLibPath", m_automatorLibEdit->text());
    m_settings->setValue("scriptsPath", m_scriptsEdit->text());
    m_settings->sync();
}

void SettingsDialog::restoreDefaults()
{
    m_mingwEdit->setText("C:/msys64/mingw64/bin");
    m_opencvEdit->setText("C:/msys64/mingw64/bin");
    m_pythonEdit->setText("python");
    m_automatorLibEdit->setText("D:/Projects/automator/lib/libautomator.dll");
    m_scriptsEdit->setText("D:/Projects/automator/scripts");
    validatePaths();
}