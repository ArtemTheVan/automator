#include "automatorwidget.h"
#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDir>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QAction>
#include <QProcessEnvironment>
#include <QCoreApplication>
#include <QScrollBar>   // Добавьте эту строку
#include <QTimer>       // Добавьте эту строку
#include <QFont>        // Добавьте эту строку
#include <QFontMetrics> // Добавьте эту строку
#include <QDateTime>    // Добавьте эту строку
#include <windows.h>
#include <vector>

// Для Qt 6
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringDecoder>
#include <QStringConverter>
#endif

// Вспомогательная структура для хранения действий мыши
struct MouseActionStruct
{
    int x;
    int y;
    DWORD flags;
};

// =========== AutomatorWidget ===========
AutomatorWidget::AutomatorWidget(QWidget *parent)
    : QWidget(parent),
      m_tabWidget(nullptr),
      m_editor(nullptr),
      m_lineNumbers(nullptr),
      m_outputBrowser(nullptr),
      m_recordButton(nullptr),
      m_loadButton(nullptr),
      m_saveButton(nullptr),
      m_startButton(nullptr),
      m_stopButton(nullptr),
      m_statusLabel(nullptr),
      m_lineColLabel(nullptr),
      m_fontSizeCombo(nullptr),
      m_templateCombo(nullptr),
      m_menuBar(nullptr),
      m_fileMenu(nullptr), // Перенесено выше
      m_editMenu(nullptr),
      m_runMenu(nullptr),
      m_settingsMenu(nullptr),
      m_recentFilesMenu(nullptr),
      m_scriptProcess(nullptr),
      m_recordingTimer(nullptr),
      m_settings(nullptr),
      m_currentFile(""),
      m_pythonPath("python"),
      m_tempPythonFile(""),
      m_mingwPath(""),
      m_opencvPath(""),
      m_automatorLibPath(""),
      m_scriptsPath(""),
      m_isRecording(false),
      m_isModified(false)
{
    // Инициализация настроек
    m_settings = new QSettings("Automator", "ScriptEditor", this);

    // Загружаем список последних файлов
    m_recentFiles = m_settings->value("recentFiles").toStringList();

    setupUI();
    setupMenu();
    setupEditor();
    // Загружаем настройки
    loadSettings();

    // Ищем Python если путь не указан
    if (m_pythonPath == "python" || m_pythonPath.isEmpty())
    {
        findPython();
    }
    else
    {
        m_statusLabel->setText(QString("Python: %1").arg(m_pythonPath));
    }

    // Пытаемся загрузить последний открытый файл
    QString lastFile = m_settings->value("lastOpenedFile").toString();
    if (!lastFile.isEmpty() && QFile::exists(lastFile))
    {
        loadScriptFile(lastFile);
    }
}

AutomatorWidget::~AutomatorWidget()
{
    if (m_scriptProcess && m_scriptProcess->state() == QProcess::Running)
    {
        m_scriptProcess->terminate();
        m_scriptProcess->waitForFinished(1000);
        delete m_scriptProcess;
    }

    cleanupTempFile(); // Удалить временный файл
}

void AutomatorWidget::loadSettings()
{
    m_mingwPath = m_settings->value("mingwPath", "C:/msys64/mingw64/bin").toString();
    m_opencvPath = m_settings->value("opencvPath", "C:/msys64/mingw64/bin").toString();
    m_automatorLibPath = m_settings->value("automatorLibPath",
                                           "D:/Projects/automator/lib/libautomator.dll")
                             .toString();
    m_scriptsPath = m_settings->value("scriptsPath",
                                      "D:/Projects/automator/scripts")
                        .toString();
    m_pythonPath = m_settings->value("pythonPath", "python").toString();
}

void AutomatorWidget::saveSettings()
{
    m_settings->setValue("mingwPath", m_mingwPath);
    m_settings->setValue("opencvPath", m_opencvPath);
    m_settings->setValue("automatorLibPath", m_automatorLibPath);
    m_settings->setValue("scriptsPath", m_scriptsPath);
    m_settings->setValue("pythonPath", m_pythonPath);
    m_settings->sync();
}

void AutomatorWidget::openSettings()
{
    SettingsDialog dlg(m_settings, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        // Обновляем пути из настроек
        m_mingwPath = dlg.mingwPath();
        m_opencvPath = dlg.opencvPath();
        m_pythonPath = dlg.pythonPath();
        m_automatorLibPath = dlg.automatorLibPath();
        m_scriptsPath = dlg.scriptsPath();

        // Сохраняем настройки
        saveSettings();

        // Обновляем статус
        m_statusLabel->setText("Настройки сохранены");
        QTimer::singleShot(2000, [this]()
                           { m_statusLabel->setText("Готов"); });
    }
}

QString AutomatorWidget::buildEnvironmentPath() const
{
    QStringList pathList;

    // 1. MinGW ПЕРВЫЙ - КРИТИЧЕСКИ ВАЖНО!
    if (!m_mingwPath.isEmpty())
    {
        QDir mingwDir(m_mingwPath);
        if (mingwDir.exists())
        {
            pathList << QDir::toNativeSeparators(m_mingwPath);
        }
    }

    // 2. Путь к библиотеке automator
    QFileInfo libInfo(m_automatorLibPath);
    if (libInfo.exists())
    {
        pathList << QDir::toNativeSeparators(libInfo.absolutePath());
    }

    // 3. Временная директория
    pathList << QDir::toNativeSeparators(QDir::tempPath());

    // 4. НЕ добавляем путь к release, чтобы избежать конфликта

    // 5. Системный PATH
    QString systemPath = QProcessEnvironment::systemEnvironment().value("PATH");
    QStringList systemPaths = systemPath.split(";");

    // Фильтруем пути - убираем дубликаты и ненужные пути
    for (const QString &sysPath : systemPaths)
    {
        if (sysPath.isEmpty())
            continue;

        // Исключаем пути, которые могут содержать дублирующие DLL
        QString lowerPath = sysPath.toLower();
        if (lowerPath.contains("gui-qt\\release"))
            continue;

        if (!pathList.contains(sysPath, Qt::CaseInsensitive))
        {
            pathList << sysPath;
        }
    }

    return pathList.join(";");
}

void AutomatorWidget::findPython()
{
    // Пытаемся найти Python
    QStringList pythonPaths = {
        "python",
        "python3",
        "py",
        "C:/Python39/python.exe",
        "C:/Python310/python.exe",
        "C:/Python311/python.exe",
        "C:/Python312/python.exe",
        "C:/Program Files/Python39/python.exe",
        "C:/Program Files/Python310/python.exe",
        "C:/Program Files/Python311/python.exe",
        "C:/Program Files/Python312/python.exe"};

    for (const QString &path : pythonPaths)
    {
        QProcess testProcess;
        testProcess.start(path, {"--version"});
        if (testProcess.waitForFinished(1000) && testProcess.exitCode() == 0)
        {
            m_pythonPath = path;
            m_statusLabel->setText(QString("Python найден: %1").arg(path));
            return;
        }
    }

    m_statusLabel->setText("Python не найден. Установите Python и добавьте в PATH");
}

bool AutomatorWidget::copyWrapperToTempDir()
{
    QString wrapperPath = QDir::tempPath() + "/wrapper_automator.py";

    // Удаляем старый файл
    if (QFile::exists(wrapperPath))
    {
        QFile::remove(wrapperPath);
    }

    // Пробуем скопировать из пути из настроек
    QStringList sourcePaths;

    if (!m_scriptsPath.isEmpty())
    {
        sourcePaths << m_scriptsPath + "/wrapper_automator.py";
    }

    // Стандартные пути
    sourcePaths << "D:/Projects/automator/scripts/wrapper_automator.py";
    sourcePaths << QCoreApplication::applicationDirPath() + "/scripts/wrapper_automator.py";
    sourcePaths << QCoreApplication::applicationDirPath() + "/../scripts/wrapper_automator.py";

    for (const QString &sourcePath : sourcePaths)
    {
        if (QFile::exists(sourcePath))
        {
            if (QFile::copy(sourcePath, wrapperPath))
            {
                qDebug() << "wrapper_automator.py скопирован из:" << sourcePath;
                return true;
            }
        }
    }

    qDebug() << "Не удалось найти wrapper_automator.py";
    return false;
}

QString AutomatorWidget::createTempPythonFile(const QString &script)
{
    QString tempDir = QDir::tempPath();
    QString fileName = QString("%1/automator_python_%2.py")
                           .arg(tempDir)
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return "";
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "# -*- coding: utf-8 -*-\n";
    out << "import sys\n";
    out << "import os\n";
    out << "\n";
    out << "# Выводим информацию о среде\n";
    out << "print(f'Текущая директория: {os.getcwd()}')\n";
    out << "print(f'Путь к скрипту: {__file__}')\n";
    out << "print(f'PATH: {os.environ.get(\"PATH\", \"\")}')\n";
    out << "\n";
    out << "# Простой импорт wrapper\n";
    out << "sys.path.insert(0, r'" << tempDir.replace("\\", "\\\\") << "')\n";
    out << "try:\n";
    out << "    from wrapper_automator import automator\n";
    out << "    print('Модуль wrapper_automator успешно импортирован')\n";
    out << "except ImportError as e:\n";
    out << "    print(f'Ошибка импорта wrapper_automator: {e}')\n";
    out << "    import traceback\n";
    out << "    traceback.print_exc()\n";
    out << "    sys.exit(1)\n";
    out << "\n";
    out << "# =========== Начало пользовательского скрипта ===========\n";
    out << "\n";
    out << script;

    file.close();

    qDebug() << "Создан временный файл:" << fileName;
    return fileName;
}

void AutomatorWidget::cleanupTempFile()
{
    if (!m_tempPythonFile.isEmpty() && QFile::exists(m_tempPythonFile))
    {
        QFile::remove(m_tempPythonFile);
        m_tempPythonFile.clear();
    }
}

void AutomatorWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(2);
    mainLayout->setContentsMargins(2, 2, 2, 2);

    // Меню бар
    m_menuBar = new QMenuBar(this);
    mainLayout->setMenuBar(m_menuBar);

    // Верхняя панель инструментов
    QHBoxLayout *topLayout = new QHBoxLayout();

    m_fontSizeCombo = new QComboBox();
    for (int i = 8; i <= 24; i += 2)
    {
        m_fontSizeCombo->addItem(QString::number(i), i);
    }
    m_fontSizeCombo->setCurrentText("12");
    connect(m_fontSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AutomatorWidget::changeFontSize);

    m_templateCombo = new QComboBox();
    m_templateCombo->addItem("Выберите шаблон...", "");
    m_templateCombo->addItem("Текст и клик", "text_click");
    m_templateCombo->addItem("Скриншот", "screenshot");
    m_templateCombo->addItem("Последовательность мыши", "mouse_sequence");
    m_templateCombo->addItem("Запуск приложения", "start_app");
    m_templateCombo->addItem("Заполнение формы", "fill_form");
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index)
            { if (index > 0) insertTemplate(); });

    topLayout->addWidget(new QLabel("Размер шрифта:"));
    topLayout->addWidget(m_fontSizeCombo);
    topLayout->addSpacing(20);
    topLayout->addWidget(new QLabel("Шаблоны:"));
    topLayout->addWidget(m_templateCombo);
    topLayout->addStretch();

    // Область редактора и вывода
    m_tabWidget = new QTabWidget();

    // Вкладка редактора
    QWidget *editorTab = new QWidget();
    QHBoxLayout *editorLayout = new QHBoxLayout(editorTab);
    editorLayout->setContentsMargins(0, 0, 0, 0);

    m_lineNumbers = new QPlainTextEdit();
    m_lineNumbers->setReadOnly(true);
    m_lineNumbers->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_lineNumbers->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_lineNumbers->setMaximumWidth(60);
    m_lineNumbers->setFont(QFont("Consolas", 10));
    m_lineNumbers->setStyleSheet("QPlainTextEdit { background-color: #f0f0f0; border: none; }");

    m_editor = new QTextEdit();
    m_editor->setFont(QFont("Consolas", 12));
    m_editor->setAcceptRichText(false);

    // Синхронизация скроллинга
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
            m_lineNumbers->verticalScrollBar(), &QScrollBar::setValue);
    connect(m_editor, &QTextEdit::textChanged, this, &AutomatorWidget::updateLineNumbers);
    connect(m_editor, &QTextEdit::cursorPositionChanged, [this]()
            {
        QTextCursor cursor = m_editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        m_lineColLabel->setText(QString("Строка: %1, Колонка: %2").arg(line).arg(col)); });

    editorLayout->addWidget(m_lineNumbers);
    editorLayout->addWidget(m_editor, 1);

    // Вкладка вывода
    m_outputBrowser = new QTextBrowser();
    m_outputBrowser->setFont(QFont("Consolas", 10));
    m_outputBrowser->setStyleSheet("QTextBrowser { background-color: black; color: white; }");

    m_tabWidget->addTab(editorTab, "Редактор");
    m_tabWidget->addTab(m_outputBrowser, "Вывод");

    // Нижняя панель кнопок
    QHBoxLayout *buttonLayout = new QHBoxLayout();

    m_recordButton = new QPushButton("⏺ Запись");
    m_recordButton->setCheckable(true);
    m_recordButton->setStyleSheet("QPushButton:checked { background-color: red; color: white; }");
    connect(m_recordButton, &QPushButton::toggled, this, &AutomatorWidget::toggleRecording);

    m_loadButton = new QPushButton("📂 Загрузить");
    connect(m_loadButton, &QPushButton::clicked, this, &AutomatorWidget::loadScript);

    m_saveButton = new QPushButton("💾 Сохранить");
    connect(m_saveButton, &QPushButton::clicked, this, &AutomatorWidget::saveScript);

    m_startButton = new QPushButton("▶ Выполнить");
    connect(m_startButton, &QPushButton::clicked, this, &AutomatorWidget::runScript);

    m_stopButton = new QPushButton("⏹ Стоп");
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, this, &AutomatorWidget::stopScript);

    buttonLayout->addWidget(m_recordButton);
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_stopButton);

    // Статус бар
    QHBoxLayout *statusLayout = new QHBoxLayout();

    m_lineColLabel = new QLabel("Строка: 1, Колонка: 1");
    m_statusLabel = new QLabel("Готов");
    m_statusLabel->setStyleSheet("QLabel { padding: 3px; background-color: #e0e0e0; border: 1px solid #a0a0a0; }");

    statusLayout->addWidget(m_lineColLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(m_statusLabel);

    // Сборка интерфейса
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_tabWidget, 1);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addLayout(statusLayout);

    // Таймер для записи
    m_recordingTimer = new QTimer(this);
    connect(m_recordingTimer, &QTimer::timeout, [this]()
            {
        if (m_isRecording) {
            m_statusLabel->setText("Запись...");
        } });

    // Загружаем пример скрипта только если нет последнего файла
    if (m_currentFile.isEmpty())
    {
        QString exampleScript =
            "# Пример Python скрипта для automator\n"
            "# Использует глобальный экземпляр automator\n\n"
            "import time\n\n"
            "automator.info('Начинаю выполнение скрипта...')\n\n"
            "# Пауза 2 секунды перед началом\n"
            "time.sleep(2)\n\n"
            "# Ввод текста\n"
            "automator.keystroke('Hello from Python!')\n"
            "automator.sleep(1)\n\n"
            "# Клик мыши\n"
            "automator.click(500, 300)\n"
            "automator.info('Клик выполнен')\n\n"
            "# Еще пауза\n"
            "time.sleep(1)\n\n"
            "# Ввод еще текста\n"
            "automator.keystroke('Это автоматизированное сообщение.')\n\n"
            "automator.info('Скрипт выполнен успешно!')\n";

        m_editor->setPlainText(exampleScript);
    }

    updateLineNumbers();
    updateTitle();
}

void AutomatorWidget::setupMenu()
{
    // Файл
    m_fileMenu = m_menuBar->addMenu("Файл");

    QAction *newAction = m_fileMenu->addAction("Новый");
    newAction->setShortcut(QKeySequence::New);
    connect(newAction, &QAction::triggered, this, &AutomatorWidget::newScript);

    QAction *loadAction = m_fileMenu->addAction("Загрузить...");
    loadAction->setShortcut(QKeySequence::Open);
    connect(loadAction, &QAction::triggered, this, &AutomatorWidget::loadScript);

    QAction *saveAction = m_fileMenu->addAction("Сохранить");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &AutomatorWidget::saveScript);

    QAction *saveAsAction = m_fileMenu->addAction("Сохранить как...");
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &AutomatorWidget::saveScriptAs);

    m_fileMenu->addSeparator();

    // Меню недавних файлов
    m_recentFilesMenu = m_fileMenu->addMenu("Недавние файлы");
    updateRecentFilesMenu();

    m_fileMenu->addSeparator();

    QAction *exitAction = m_fileMenu->addAction("Выход");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);

    // Правка
    m_editMenu = m_menuBar->addMenu("Правка");

    QAction *undoAction = m_editMenu->addAction("Отменить");
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, m_editor, &QTextEdit::undo);

    QAction *redoAction = m_editMenu->addAction("Повторить");
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, m_editor, &QTextEdit::redo);

    m_editMenu->addSeparator();

    QAction *cutAction = m_editMenu->addAction("Вырезать");
    cutAction->setShortcut(QKeySequence::Cut);
    connect(cutAction, &QAction::triggered, m_editor, &QTextEdit::cut);

    QAction *copyAction = m_editMenu->addAction("Копировать");
    copyAction->setShortcut(QKeySequence::Copy);
    connect(copyAction, &QAction::triggered, m_editor, &QTextEdit::copy);

    QAction *pasteAction = m_editMenu->addAction("Вставить");
    pasteAction->setShortcut(QKeySequence::Paste);
    connect(pasteAction, &QAction::triggered, m_editor, &QTextEdit::paste);

    // Выполнение
    m_runMenu = m_menuBar->addMenu("Выполнение");

    QAction *runAction = m_runMenu->addAction("Выполнить скрипт");
    runAction->setShortcut(QKeySequence("F5"));
    connect(runAction, &QAction::triggered, this, &AutomatorWidget::runScript);

    QAction *stopAction = m_runMenu->addAction("Остановить выполнение");
    stopAction->setShortcut(QKeySequence("Shift+F5"));
    connect(stopAction, &QAction::triggered, this, &AutomatorWidget::stopScript);

    m_runMenu->addSeparator();

    QAction *findPythonAction = m_runMenu->addAction("Найти Python");
    connect(findPythonAction, &QAction::triggered, this, &AutomatorWidget::findPython);

    // Настройки (ДОБАВЬТЕ ЭТОТ БЛОК)
    m_settingsMenu = m_menuBar->addMenu("Настройки");

    QAction *settingsAction = m_settingsMenu->addAction("Пути зависимостей...");
    settingsAction->setShortcut(QKeySequence("Ctrl+P"));
    connect(settingsAction, &QAction::triggered, this, &AutomatorWidget::openSettings);

    QAction *showPathsAction = m_settingsMenu->addAction("Показать текущие пути");
    connect(showPathsAction, &QAction::triggered, [this]()
            {
        QString message = QString(
            "Текущие пути:\n\n"
            "MinGW: %1\n"
            "OpenCV: %2\n"
            "Python: %3\n"
            "Библиотека: %4\n"
            "Скрипты: %5\n\n"
            "PATH: %6"
        ).arg(m_mingwPath, m_opencvPath, m_pythonPath, 
              m_automatorLibPath, m_scriptsPath, buildEnvironmentPath());
        
        QMessageBox::information(this, "Текущие пути", message); });

    m_settingsMenu->addSeparator();

    QAction *reloadSettingsAction = m_settingsMenu->addAction("Перезагрузить настройки");
    connect(reloadSettingsAction, &QAction::triggered, [this]()
            {
        loadSettings();
        m_statusLabel->setText("Настройки перезагружены"); });
}

void AutomatorWidget::setupEditor()
{
    QFont editorFont("Consolas", 12);
    m_editor->setFont(editorFont);
    m_editor->setTabStopDistance(40);

    connect(m_editor, &QTextEdit::textChanged, [this]()
            {
        m_isModified = true;
        updateTitle(); });
}

bool AutomatorWidget::copyDllToTempDir()
{
    QString sourceDllPath = m_automatorLibPath;

    if (sourceDllPath.isEmpty() || !QFile::exists(sourceDllPath))
    {
        qDebug() << "Исходный файл DLL не найден:" << sourceDllPath;
        return false;
    }

    QString tempDllPath = QDir::tempPath() + "/libautomator.dll";

    // Удаляем старый файл
    if (QFile::exists(tempDllPath))
    {
        QFile::remove(tempDllPath);
    }

    // Копируем файл
    if (QFile::copy(sourceDllPath, tempDllPath))
    {
        qDebug() << "DLL скопирована в:" << tempDllPath;
        return true;
    }
    else
    {
        qDebug() << "Не удалось скопировать DLL";
        return false;
    }
}

QString AutomatorWidget::createSimpleTempPythonFile(const QString &script)
{
    QString tempDir = QDir::tempPath();
    QString fileName = QString("%1/simple_python_%2.py")
                           .arg(tempDir)
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return "";
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    out << "# -*- coding: utf-8 -*-\n";
    out << "import sys\n";
    out << "import os\n";
    out << "import ctypes\n";
    out << "from ctypes import wintypes\n";
    out << "\n";
    out << "# Выводим информацию о среде\n";
    out << "print('=== Запуск Python скрипта ===')\n";
    out << "print(f'Используется Python: {sys.executable}')\n";
    out << "print(f'Текущая директория: {os.getcwd()}')\n";
    out << "\n";
    out << "# КРИТИЧЕСКИ ВАЖНО: Для Python 3.8+ добавляем пути поиска DLL\n";
    out << "if hasattr(os, 'add_dll_directory'):\n";
    out << "    # Путь к MinGW\n";
    out << "    mingw_path = r'" << m_mingwPath.replace("\\", "\\\\") << "'\n";
    out << "    # Путь к библиотеке automator\n";
    QString libDirPath = QFileInfo(m_automatorLibPath).absolutePath();
    out << "    lib_path = r'" << libDirPath.replace("\\", "\\\\") << "'\n";
    out << "    # Добавляем оба пути\n";
    out << "    for path in [mingw_path, lib_path]:\n";
    out << "        if os.path.exists(path):\n";
    out << "            os.add_dll_directory(path)\n";
    out << "            print(f'Добавлен путь поиска DLL: {path}')\n";
    out << "else:\n";
    out << "    print('Внимание: Python < 3.8, os.add_dll_directory недоступен')\n";
    out << "\n";
    out << "# Проверяем PATH\n";
    out << "paths = os.environ.get('PATH', '').split(';')\n";
    out << "print('Первый путь в PATH:', paths[0] if paths else 'пусто')\n";
    out << "\n";
    out << "# Пробуем загрузить DLL\n";
    out << "dll_path = r'" << m_automatorLibPath.replace("\\", "\\\\") << "'\n";
    out << "print(f'Пробую загрузить DLL: {dll_path}')\n";
    out << "print(f'Файл существует: {os.path.exists(dll_path)}')\n";
    out << "\n";
    out << "if os.path.exists(dll_path):\n";
    out << "    try:\n";
    out << "        # Используем CDLL для MinGW\n";
    out << "        lib = ctypes.CDLL(dll_path)\n";
    out << "        print('УСПЕХ: DLL загружена через CDLL')\n";
    out << "        \n";
    out << "        # Проверяем функции\n";
    out << "        if hasattr(lib, 'simulate_keystroke'):\n";
    out << "            print('Функция simulate_keystroke найдена')\n";
    out << "            lib.simulate_keystroke.argtypes = [ctypes.c_char_p]\n";
    out << "            lib.simulate_keystroke.restype = None\n";
    out << "            lib.simulate_keystroke(b'Test from Python!')\n";
    out << "            print('Функция simulate_keystroke вызвана успешно')\n";
    out << "        else:\n";
    out << "            print('Функция simulate_keystroke НЕ найдена')\n";
    out << "            \n";
    out << "    except Exception as e:\n";
    out << "        print(f'Ошибка загрузки DLL: {e}')\n";
    out << "        import traceback\n";
    out << "        traceback.print_exc()\n";
    out << "        # Пробуем альтернативные пути\n";
    out << "        temp_dll = os.path.join(os.environ.get('TEMP', ''), 'libautomator.dll')\n";
    out << "        print(f'Пробую загрузить: {temp_dll}')\n";
    out << "        if os.path.exists(temp_dll):\n";
    out << "            try:\n";
    out << "                lib = ctypes.CDLL(temp_dll)\n";
    out << "                print('УСПЕХ: DLL загружена из временной директории')\n";
    out << "            except Exception as e2:\n";
    out << "                print(f'Ошибка загрузки из временной директории: {e2}')\n";
    out << "else:\n";
    out << "    print('ОШИБКА: Файл DLL не найден')\n";
    out << "\n";
    out << "# Пользовательский скрипт\n";
    out << script;

    file.close();

    qDebug() << "Создан простой временный файл:" << fileName;
    return fileName;
}

// =========== Слоты ===========
void AutomatorWidget::runScript()
{
    QString script = m_editor->toPlainText();
    if (script.isEmpty())
    {
        QMessageBox::warning(this, "Ошибка", "Скрипт пуст!");
        return;
    }

    // Очищаем предыдущий временный файл
    cleanupTempFile();

    // Создаем временный файл
    m_tempPythonFile = createSimpleTempPythonFile(script);
    if (m_tempPythonFile.isEmpty())
    {
        m_outputBrowser->append("Ошибка: Не удалось создать временный файл скрипта");
        return;
    }

    // Настраиваем UI
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_recordButton->setEnabled(false);
    m_loadButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_statusLabel->setText("Выполнение скрипта...");
    m_statusLabel->setStyleSheet("QLabel { background-color: #ccffcc; color: black; }");

    // Очищаем вывод
    m_outputBrowser->clear();
    m_outputBrowser->append("=== Запуск Python скрипта ===");

    // Создаем процесс Python
    m_scriptProcess = new QProcess(this);
    m_scriptProcess->setProcessChannelMode(QProcess::MergedChannels);

    // Настраиваем окружение
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("PYTHONIOENCODING", "utf-8");
    env.insert("PYTHONUTF8", "1");

    // КРИТИЧЕСКИ ВАЖНО: Устанавливаем правильный PATH перед запуском
    QString newPath = buildEnvironmentPath();
    env.insert("PATH", newPath);

    m_scriptProcess->setProcessEnvironment(env);

    // КРИТИЧЕСКИ ВАЖНО: Устанавливаем рабочую директорию в путь MinGW
    // Это нужно потому что Windows ищет DLL сначала в рабочей директории
    QDir mingwDir(m_mingwPath);
    if (mingwDir.exists())
    {
        m_scriptProcess->setWorkingDirectory(mingwDir.absolutePath());
    }
    else
    {
        // Если MinGW путь не существует, используем путь к библиотеке
        QFileInfo dllInfo(m_automatorLibPath);
        if (dllInfo.exists())
        {
            m_scriptProcess->setWorkingDirectory(dllInfo.absolutePath());
        }
        else
        {
            m_scriptProcess->setWorkingDirectory(QDir::tempPath());
        }
    }

    // Выводим информацию для отладки
    m_outputBrowser->append(QString("Используется Python: %1").arg(m_pythonPath));
    m_outputBrowser->append(QString("Файл скрипта: %1").arg(m_tempPythonFile));
    m_outputBrowser->append(QString("Рабочая директория: %1").arg(m_scriptProcess->workingDirectory()));
    m_outputBrowser->append(QString("Установлен PATH:"));
    QStringList pathList = newPath.split(";");
    for (const QString &path : pathList.mid(0, 10))
    { // Показываем первые 10 путей
        m_outputBrowser->append("  " + path);
    }
    if (pathList.size() > 10)
    {
        m_outputBrowser->append(QString("  ... и еще %1 путей").arg(pathList.size() - 10));
    }

    // Подключаем сигналы
    connect(m_scriptProcess, &QProcess::readyReadStandardOutput,
            this, &AutomatorWidget::onScriptOutput);
    connect(m_scriptProcess, &QProcess::readyReadStandardError,
            this, &AutomatorWidget::onScriptError);
    connect(m_scriptProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AutomatorWidget::onScriptFinished);

    // Запускаем Python
    QStringList arguments;
    arguments << "-X" << "utf8" << m_tempPythonFile;

    m_scriptProcess->start(m_pythonPath, arguments);

    if (!m_scriptProcess->waitForStarted(3000))
    {
        m_outputBrowser->append("Ошибка: Не удалось запустить Python.");
        m_outputBrowser->append("Проверьте путь к Python в настройках.");
        cleanupTempFile();
        onScriptFinished(-1, QProcess::CrashExit);
        return;
    }
}

void AutomatorWidget::stopScript()
{
    if (m_scriptProcess && m_scriptProcess->state() == QProcess::Running)
    {
        m_scriptProcess->terminate();
        if (!m_scriptProcess->waitForFinished(1000))
        {
            m_scriptProcess->kill();
        }
    }
}

void AutomatorWidget::onScriptFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit)
    {
        m_outputBrowser->append("=== Скрипт завершился аварийно ===");
    }
    else
    {
        m_outputBrowser->append(QString("=== Скрипт завершен с кодом %1 ===").arg(exitCode));
    }

    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_recordButton->setEnabled(true);
    m_loadButton->setEnabled(true);
    m_saveButton->setEnabled(true);

    if (exitCode == 0 && exitStatus == QProcess::NormalExit)
    {
        m_statusLabel->setText("Скрипт выполнен успешно");
        m_statusLabel->setStyleSheet("QLabel { background-color: #ccffcc; color: black; }");
    }
    else
    {
        m_statusLabel->setText("Скрипт завершился с ошибкой");
        m_statusLabel->setStyleSheet("QLabel { background-color: #ffe0e0; color: black; }");
    }

    // Удаляем временный файл
    cleanupTempFile();

    if (m_scriptProcess)
    {
        m_scriptProcess->deleteLater();
        m_scriptProcess = nullptr;
    }
}

void AutomatorWidget::onScriptOutput()
{
    if (m_scriptProcess)
    {
        QByteArray output = m_scriptProcess->readAllStandardOutput();
        QString outputStr = QString::fromUtf8(output).trimmed();
        if (!outputStr.isEmpty())
        {
            m_outputBrowser->append(outputStr);
        }
    }
}

void AutomatorWidget::onScriptError()
{
    if (m_scriptProcess)
    {
        QByteArray error = m_scriptProcess->readAllStandardError();
        QString errorStr = QString::fromUtf8(error).trimmed();
        if (!errorStr.isEmpty())
        {
            m_outputBrowser->append("Ошибка: " + errorStr);
        }
    }
}

void AutomatorWidget::newScript()
{
    if (m_isModified)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Новый скрипт",
                                                                  "Сохранить текущий скрипт?",
                                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
        {
            saveScript();
        }
        else if (reply == QMessageBox::Cancel)
        {
            return;
        }
    }

    m_editor->clear();
    m_currentFile = "";
    m_isModified = false;
    updateTitle();
    updateLineNumbers();

    // Обновляем статус
    m_statusLabel->setText("Создан новый скрипт");
}

void AutomatorWidget::loadScript()
{
    if (m_isModified)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Загрузить скрипт",
                                                                  "Сохранить текущий скрипт?",
                                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
        {
            saveScript();
        }
        else if (reply == QMessageBox::Cancel)
        {
            return;
        }
    }

    // Используем последнюю директорию вместо QDir::homePath()
    QString lastDir = getLastDirectory();

    QString fileName = QFileDialog::getOpenFileName(this, "Загрузить скрипт",
                                                    lastDir, // Используем последнюю директорию
                                                    "Python скрипты (*.py);;Текстовые файлы (*.txt);;Все файлы (*)");
    if (!fileName.isEmpty())
    {
        loadScriptFile(fileName);
    }
}

void AutomatorWidget::saveScript()
{
    if (m_currentFile.isEmpty())
    {
        saveScriptAs();
    }
    else
    {
        saveScriptFile(m_currentFile);
    }
}

void AutomatorWidget::saveScriptAs()
{
    // Используем последнюю директорию вместо QDir::homePath()
    QString lastDir = getLastDirectory();
    QString defaultFile = lastDir + "/untitled.py";

    // Если есть текущий файл, предлагаем сохранить в той же директории с его именем
    if (!m_currentFile.isEmpty())
    {
        QFileInfo currentFileInfo(m_currentFile);
        defaultFile = lastDir + "/" + currentFileInfo.fileName();
    }

    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить скрипт",
                                                    defaultFile,
                                                    "Python скрипты (*.py);;Текстовые файлы (*.txt);;Все файлы (*)");
    if (!fileName.isEmpty())
    {
        saveScriptFile(fileName);
    }
}

void AutomatorWidget::updateLineNumbers()
{
    int blockCount = m_editor->document()->blockCount();
    QString numbers;

    for (int i = 1; i <= blockCount; ++i)
    {
        numbers.append(QString::number(i) + "\n");
    }

    m_lineNumbers->setPlainText(numbers);

    QFontMetrics fm(m_editor->font());
    int docHeight = fm.lineSpacing() * blockCount + 10;
    m_lineNumbers->setMinimumHeight(docHeight);
}

void AutomatorWidget::changeFontSize(int index)
{
    int size = m_fontSizeCombo->itemData(index).toInt();
    QFont font = m_editor->font();
    font.setPointSize(size);
    m_editor->setFont(font);
    m_lineNumbers->setFont(font);
    updateLineNumbers();
}

void AutomatorWidget::insertTemplate()
{
    QString templateName = m_templateCombo->currentData().toString();
    QString templateText = getScriptTemplate(templateName);

    if (!templateText.isEmpty())
    {
        m_editor->textCursor().insertText(templateText + "\n\n");
        m_isModified = true;
        updateTitle();
    }

    m_templateCombo->setCurrentIndex(0);
}

void AutomatorWidget::toggleRecording(bool checked)
{
    m_isRecording = checked;

    if (checked)
    {
        m_recordButton->setText("⏹ Стоп запись");
        m_statusLabel->setText("Запись начата...");
        m_statusLabel->setStyleSheet("QLabel { background-color: #ffcccc; color: black; }");
        m_recordingTimer->start(1000);

        QDateTime now = QDateTime::currentDateTime();
        m_editor->append(QString("\n# Запись начата: %1\n").arg(now.toString("dd.MM.yyyy HH:mm:ss")));
    }
    else
    {
        m_recordButton->setText("⏺ Запись");
        m_statusLabel->setText("Запись остановлена");
        m_statusLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: black; }");
        m_recordingTimer->stop();

        QDateTime now = QDateTime::currentDateTime();
        m_editor->append(QString("# Запись остановлена: %1\n").arg(now.toString("dd.MM.yyyy HH:mm:ss")));
    }
}

// =========== Вспомогательные функции ===========

void AutomatorWidget::addToRecentFiles(const QString &fileName)
{
    // Удаляем файл из списка, если он уже есть
    m_recentFiles.removeAll(fileName);

    // Добавляем файл в начало списка
    m_recentFiles.prepend(fileName);

    // Ограничиваем список 10 файлами
    while (m_recentFiles.size() > 10)
    {
        m_recentFiles.removeLast();
    }

    // Сохраняем список в настройках
    m_settings->setValue("recentFiles", m_recentFiles);
    m_settings->setValue("lastOpenedFile", fileName);

    // Обновляем меню
    updateRecentFilesMenu();
}

void AutomatorWidget::updateRecentFilesMenu()
{
    m_recentFilesMenu->clear();

    if (m_recentFiles.isEmpty())
    {
        QAction *noFilesAction = m_recentFilesMenu->addAction("Нет недавних файлов");
        noFilesAction->setEnabled(false);
        return;
    }

    for (int i = 0; i < m_recentFiles.size(); ++i)
    {
        QString displayName = QString("%1. %2").arg(i + 1).arg(QFileInfo(m_recentFiles[i]).fileName());
        QAction *fileAction = m_recentFilesMenu->addAction(displayName);

        // Сохраняем полный путь в данных действия
        fileAction->setData(m_recentFiles[i]);

        connect(fileAction, &QAction::triggered, this, &AutomatorWidget::loadRecentFile);
    }

    m_recentFilesMenu->addSeparator();

    QAction *clearAction = m_recentFilesMenu->addAction("Очистить список");
    connect(clearAction, &QAction::triggered, [this]()
            {
        m_recentFiles.clear();
        m_settings->setValue("recentFiles", m_recentFiles);
        updateRecentFilesMenu(); });
}

void AutomatorWidget::loadRecentFile()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;

    QString fileName = action->data().toString();
    if (fileName.isEmpty())
        return;

    if (m_isModified)
    {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Загрузить скрипт",
                                                                  "Сохранить текущий скрипт?",
                                                                  QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        if (reply == QMessageBox::Yes)
        {
            saveScript();
        }
        else if (reply == QMessageBox::Cancel)
        {
            return;
        }
    }

    loadScriptFile(fileName);
}

void AutomatorWidget::loadScriptFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл: " + file.errorString());
        return;
    }

    QTextStream in(&file);
    m_editor->setPlainText(in.readAll());
    file.close();

    m_currentFile = fileName;
    m_isModified = false;
    updateTitle();
    updateLineNumbers();

    // Добавляем файл в список недавних
    addToRecentFiles(fileName);

    // Обновляем последнюю директорию
    updateLastDirectory(fileName);

    m_statusLabel->setText(QString("Загружен: %1").arg(QFileInfo(fileName).fileName()));
}

void AutomatorWidget::saveScriptFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл: " + file.errorString());
        return;
    }

    QTextStream out(&file);
    out << m_editor->toPlainText();
    file.close();

    m_currentFile = fileName;
    m_isModified = false;
    updateTitle();

    // Добавляем файл в список недавних
    addToRecentFiles(fileName);

    // Обновляем последнюю директорию
    updateLastDirectory(fileName);

    m_statusLabel->setText(QString("Сохранен: %1").arg(QFileInfo(fileName).fileName()));
}

void AutomatorWidget::updateTitle()
{
    QString title = "Qt Automator - Редактор скриптов";
    if (!m_currentFile.isEmpty())
    {
        title += " - " + QFileInfo(m_currentFile).fileName();
    }
    if (m_isModified)
    {
        title += " *";
    }
    setWindowTitle(title);
}

QString AutomatorWidget::getScriptTemplate(const QString &name)
{
    if (name == "text_click")
    {
        return "# Шаблон: Ввод текста и клик мыши\n"
               "import time\n"
               "from wrapper_automator import automator\n\n"
               "def main():\n"
               "    automator.info('Начинаю выполнение скрипта...')\n"
               "    \n"
               "    # Пауза перед началом\n"
               "    time.sleep(2)\n"
               "    \n"
               "    # Ввод текста\n"
               "    automator.keystroke('Hello World!')\n"
               "    automator.sleep(0.5)\n"
               "    \n"
               "    # Клик мыши\n"
               "    automator.click(500, 300)\n"
               "    automator.info('Текст введен и клик выполнен')\n"
               "    \n"
               "if __name__ == '__main__':\n"
               "    main()";
    }
    else if (name == "screenshot")
    {
        return "# Шаблон: Захват скриншота\n"
               "import time\n"
               "import os\n"
               "from wrapper_automator import automator\n"
               "from datetime import datetime\n\n"
               "def main():\n"
               "    automator.info('Захват скриншота...')\n"
               "    \n"
               "    # Пауза перед захватом\n"
               "    time.sleep(3)\n"
               "    \n"
               "    # Генерируем имя файла с датой и временем\n"
               "    timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')\n"
               "    filename = f'screenshot_{timestamp}.png'\n"
               "    \n"
               "    # Захват всего экрана (подставьте свои координаты)\n"
               "    result = automator.capture_screen(0, 0, 1920, 1080, filename)\n"
               "    \n"
               "    if result == 0:\n"
               "        automator.info(f'Скриншот сохранен как: {filename}')\n"
               "    else:\n"
               "        automator.error('Ошибка при захвате скриншота')\n"
               "    \n"
               "if __name__ == '__main__':\n"
               "    main()";
    }
    else if (name == "mouse_sequence")
    {
        return "# Шаблон: Последовательность действий мыши\n"
               "import time\n"
               "from wrapper_automator import automator, MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_LEFTUP\n\n"
               "def main():\n"
               "    automator.info('Выполняю последовательность действий мыши...')\n"
               "    \n"
               "    # Пауза перед началом\n"
               "    time.sleep(2)\n"
               "    \n"
               "    # Определяем последовательность действий\n"
               "    actions = [\n"
               "        (100, 100, MOUSEEVENTF_LEFTDOWN),   # Нажатие левой кнопки\n"
               "        (200, 200, 0),                       # Перемещение с зажатой кнопкой\n"
               "        (200, 200, MOUSEEVENTF_LEFTUP)      # Отпускание кнопки\n"
               "    ]\n"
               "    \n"
               "    # Выполняем последовательность\n"
               "    automator.mouse_sequence(actions)\n"
               "    automator.info('Последовательность выполнена')\n"
               "    \n"
               "if __name__ == '__main__':\n"
               "    main()";
    }
    else if (name == "start_app")
    {
        return "# Шаблон: Запуск приложения\n"
               "import time\n"
               "import subprocess\n"
               "from wrapper_automator import automator\n\n"
               "def main():\n"
               "    automator.info('Запуск приложения...')\n"
               "    \n"
               "    # Запускаем приложение (например, Блокнот)\n"
               "    try:\n"
               "        subprocess.Popen(['notepad.exe'])\n"
               "        automator.info('Приложение запущено')\n"
               "    except Exception as e:\n"
               "        automator.error(f'Ошибка запуска: {e}')\n"
               "    \n"
               "    # Ждем немного\n"
               "    time.sleep(2)\n"
               "    \n"
               "    # Вводим текст в открывшееся окно\n"
               "    automator.keystroke('Привет от Python!')\n"
               "    \n"
               "if __name__ == '__main__':\n"
               "    main()";
    }
    else if (name == "fill_form")
    {
        return "# Шаблон: Заполнение формы\n"
               "import time\n"
               "from wrapper_automator import automator\n\n"
               "def main():\n"
               "    automator.info('Заполнение формы...')\n"
               "    \n"
               "    # Пауза перед началом\n"
               "    time.sleep(2)\n"
               "    \n"
               "    # Данные для заполнения\n"
               "    form_data = {\n"
               "        'Имя': 'Иван',\n"
               "        'Фамилия': 'Петров',\n"
               "        'Email': 'ivan@example.com',\n"
               "        'Телефон': '+7 999 123-45-67'\n"
               "    }\n"
               "    \n"
               "    # Заполняем поля формы\n"
               "    for field_name, value in form_data.items():\n"
               "        automator.keystroke(field_name)\n"
               "        automator.sleep(0.2)\n"
               "        automator.keystroke('\\t')  # Tab для перехода к полю ввода\n"
               "        automator.sleep(0.2)\n"
               "        automator.keystroke(value)\n"
               "        automator.sleep(0.3)\n"
               "        automator.keystroke('\\t')  # Tab для перехода к следующему полю\n"
               "        automator.sleep(0.2)\n"
               "    \n"
               "    automator.info('Форма заполнена')\n"
               "    \n"
               "if __name__ == '__main__':\n"
               "    main()";
    }

    return "";
}

QString AutomatorWidget::getFileExtension() const
{
    return "py";
}

QString AutomatorWidget::getLastDirectory() const
{
    // Получаем последнюю директорию из настроек
    QString lastDir = m_settings->value("lastDirectory", QDir::homePath()).toString();

    // Проверяем, что директория существует
    QDir dir(lastDir);
    if (!dir.exists())
    {
        return QDir::homePath();
    }

    return lastDir;
}

void AutomatorWidget::setLastDirectory(const QString &path)
{
    QFileInfo fileInfo(path);
    QString dirPath;

    if (fileInfo.isDir())
    {
        dirPath = path;
    }
    else
    {
        dirPath = fileInfo.absolutePath();
    }

    // Сохраняем только если директория существует
    QDir dir(dirPath);
    if (dir.exists())
    {
        m_settings->setValue("lastDirectory", dirPath);
    }
}

void AutomatorWidget::updateLastDirectory(const QString &filePath)
{
    if (!filePath.isEmpty())
    {
        QFileInfo fileInfo(filePath);
        setLastDirectory(fileInfo.absolutePath());
    }
}
