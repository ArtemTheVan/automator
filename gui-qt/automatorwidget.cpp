#include "automatorwidget.h"
#include <QScrollBar>
#include <QTextBlock>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QAction>
#include <windows.h>
#include <vector>

// Вспомогательная структура для хранения действий мыши
struct MouseActionStruct
{
    int x;
    int y;
    DWORD flags;
};

// =========== AutomatorWidget ===========

AutomatorWidget::AutomatorWidget(QWidget *parent)
    : QWidget(parent), m_tabWidget(nullptr), m_editor(nullptr), m_lineNumbers(nullptr), m_outputBrowser(nullptr),
      m_startButton(nullptr), m_stopButton(nullptr), m_recordButton(nullptr), m_loadButton(nullptr),
      m_saveButton(nullptr), m_statusLabel(nullptr), m_lineColLabel(nullptr), m_fontSizeCombo(nullptr),
      m_templateCombo(nullptr), m_menuBar(nullptr), m_scriptProcess(nullptr), m_recordingTimer(nullptr),
      m_settings(nullptr), m_currentFile(""), m_pythonPath("python"), m_tempPythonFile(""),
      m_isRecording(false), m_isModified(false)
{
    // Инициализация настроек
    m_settings = new QSettings("Automator", "ScriptEditor", this);

    // Загружаем список последних файлов
    m_recentFiles = m_settings->value("recentFiles").toStringList();

    setupUI();
    setupMenu();
    setupEditor();
    findPython(); // Найти Python при запуске

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
    QString sourcePath = "D:/Projects/automator/scripts/wrapper_automator.py"; // Или полный путь к файлу
    if (!QFile::exists(wrapperPath))
    {
        QFile sourceFile(sourcePath);
        if (sourceFile.exists())
        {
            if (sourceFile.copy(wrapperPath))
            {
                qDebug() << "wrapper_automator.py скопирован в:" << wrapperPath;
                m_outputBrowser->append(QString("wrapper_automator.py скопирован в: %1").arg(wrapperPath));
                return true;
            }
            else
            {
                qDebug() << "Ошибка копирования wrapper_automator.py";
                m_outputBrowser->append(QString("Ошибка копирования wrapper_automator.py: %1").arg(wrapperPath));
            }
        }
        else
        {
            qDebug() << "Исходный файл wrapper_automator.py не найден:" << sourcePath;
            m_outputBrowser->append(QString("Исходный файл wrapper_automator.py не найден: %1").arg(sourcePath));
        }
        return false;
    }
    return true;
}

QString AutomatorWidget::createTempPythonFile(const QString &script)
{
    // Создаем временный файл в системной временной директории
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

    // Добавляем импорт модуля wrapper_automator из временной директории
    out << "import sys\n";
    out << "import os\n";
    out << "sys.path.insert(0, r'" << tempDir << "')\n";
    out << "\n";
    out << "try:\n";
    out << "    from wrapper_automator import automator\n";
    out << "    print('Модуль wrapper_automator успешно загружен')\n";
    out << "except ImportError as e:\n";
    out << "    print(f'Ошибка импорта wrapper_automator: {e}')\n";
    out << "    print('Попытка загрузить из других путей...')\n";
    out << "    # Добавляем дополнительные пути\n";
    out << "    additional_paths = [\n";
    out << "        '.',\n";
    out << "        '..',\n";
    out << "        '../scripts',\n";
    out << "        'D:/Projects/automator/scripts',\n";
    out << "    ]\n";
    out << "    for path in additional_paths:\n";
    out << "        if os.path.exists(path):\n";
    out << "            abs_path = os.path.abspath(path)\n";
    out << "            if abs_path not in sys.path:\n";
    out << "                sys.path.append(abs_path)\n";
    out << "    try:\n";
    out << "        from wrapper_automator import automator\n";
    out << "        print('Модуль wrapper_automator загружен из дополнительных путей')\n";
    out << "    except ImportError:\n";
    out << "        print('Не удалось загрузить wrapper_automator')\n";
    out << "        sys.exit(1)\n";
    out << "\n";
    out << "# =========== Начало пользовательского скрипта ===========\n\n";
    out << script;

    file.close();

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

    m_runMenu = m_menuBar->addMenu("Выполнение");

    QAction *runAction = m_runMenu->addAction("Выполнить скрипт");
    runAction->setShortcut(QKeySequence("F5"));
    connect(runAction, &QAction::triggered, this, &AutomatorWidget::runScript);

    QAction *stopAction = m_runMenu->addAction("Остановить выполнение");
    stopAction->setShortcut(QKeySequence("Shift+F5"));
    connect(stopAction, &QAction::triggered, this, &AutomatorWidget::stopScript);

    QAction *findPythonAction = m_runMenu->addAction("Найти Python");
    connect(findPythonAction, &QAction::triggered, this, &AutomatorWidget::findPython);
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

    // Копируем wrapper_automator.py во временную директорию
    if (!copyWrapperToTempDir())
    {
        m_outputBrowser->append("Предупреждение: Не удалось скопировать wrapper_automator.py. Возможно, он уже существует.");
    }

    // Создаем новый временный файл
    m_tempPythonFile = createTempPythonFile(script);
    if (m_tempPythonFile.isEmpty())
    {
        m_outputBrowser->append("Ошибка: Не удалось создать временный файл");
        return;
    }

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
    m_outputBrowser->append(QString("Используется Python: %1").arg(m_pythonPath));
    m_outputBrowser->append(QString("Файл скрипта: %1").arg(m_tempPythonFile));

    // Создаем процесс Python
    m_scriptProcess = new QProcess(this);
    m_scriptProcess->setProcessChannelMode(QProcess::MergedChannels);

    // Подключаем сигналы
    connect(m_scriptProcess, &QProcess::readyReadStandardOutput,
            this, &AutomatorWidget::onScriptOutput);
    connect(m_scriptProcess, &QProcess::readyReadStandardError,
            this, &AutomatorWidget::onScriptError);
    connect(m_scriptProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AutomatorWidget::onScriptFinished);

    // Запускаем Python
    m_scriptProcess->start(m_pythonPath, {m_tempPythonFile});

    if (!m_scriptProcess->waitForStarted(3000))
    {
        m_outputBrowser->append("Ошибка: Не удалось запустить Python. Убедитесь, что Python установлен и добавлен в PATH");
        m_outputBrowser->append("Попробуйте использовать действие 'Найти Python' в меню 'Выполнение'");
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