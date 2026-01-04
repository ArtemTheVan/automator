#include "automatorwidget.h"
#include <QScrollBar>
#include <QTextBlock>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <windows.h>
#include <vector>

// Вспомогательная структура для хранения действий мыши
struct MouseActionStruct {
    int x;
    int y;
    DWORD flags;
};

// =========== AutomationWorker ===========

void AutomationWorker::run()
{
    emit statusChanged("Начинаю выполнение скрипта...");
    
    QStringList lines = m_script.split('\n', Qt::SkipEmptyParts);
    
    for (const QString& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) {
            continue;
        }
        
        executeCommand(trimmed);
        
        if (QThread::currentThread()->isInterruptionRequested()) {
            emit statusChanged("Выполнение прервано пользователем");
            break;
        }
    }
    
    emit statusChanged("Скрипт выполнен!");
    emit automationFinished();
}

void AutomationWorker::executeCommand(const QString& command)
{
    if (command.startsWith("KEYSTROKE ")) {
        QString text = command.mid(10).trimmed();
        if (text.startsWith('"') && text.endsWith('"')) {
            text = text.mid(1, text.length() - 2);
        }
        emit statusChanged(QString("Ввод текста: %1").arg(text));
        simulate_keystroke(text.toUtf8().constData());
    }
    else if (command.startsWith("CLICK ")) {
        QStringList parts = command.mid(6).split(',');
        if (parts.size() >= 2) {
            int x = parts[0].trimmed().toInt();
            int y = parts[1].trimmed().toInt();
            emit statusChanged(QString("Клик в (%1, %2)").arg(x).arg(y));
            simulate_mouse_click_at(x, y);
        }
    }
    else if (command.startsWith("SLEEP ")) {
        int ms = command.mid(6).trimmed().toInt();
        emit statusChanged(QString("Пауза %1 мс").arg(ms));
        Sleep(ms);
    }
    else if (command.startsWith("CAPTURE ")) {
        QStringList parts = command.mid(8).split(',');
        if (parts.size() >= 5) {
            int x = parts[0].trimmed().toInt();
            int y = parts[1].trimmed().toInt();
            int w = parts[2].trimmed().toInt();
            int h = parts[3].trimmed().toInt();
            QString filename = parts[4].trimmed();
            emit statusChanged(QString("Захват экрана в %1").arg(filename));
            capture_screen_region(x, y, w, h, filename.toUtf8().constData());
        }
    }
    else if (command.startsWith("MOUSE_SEQUENCE ")) {
        QString params = command.mid(15).trimmed();
        QStringList actions = params.split(';', Qt::SkipEmptyParts);
        std::vector<MouseActionStruct> mouseActions;
        
        for (const QString& action : actions) {
            QStringList parts = action.split(',');
            if (parts.size() >= 3) {
                int x = parts[0].trimmed().toInt();
                int y = parts[1].trimmed().toInt();
                DWORD flags = 0;
                
                QString flagStr = parts[2].trimmed().toUpper();
                if (flagStr == "LEFTDOWN") flags = MOUSEEVENTF_LEFTDOWN;
                else if (flagStr == "LEFTUP") flags = MOUSEEVENTF_LEFTUP;
                else if (flagStr == "RIGHTDOWN") flags = MOUSEEVENTF_RIGHTDOWN;
                else if (flagStr == "RIGHTUP") flags = MOUSEEVENTF_RIGHTUP;
                else if (flagStr == "MOVE") flags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
                
                mouseActions.push_back({x, y, flags});
            }
        }
        
        if (!mouseActions.empty()) {
            simulate_mouse_sequence(mouseActions.data(), mouseActions.size());
            emit statusChanged(QString("Выполнено %1 действий мыши").arg(mouseActions.size()));
        }
    }
    else if (command.startsWith("WAIT_WINDOW ")) {
        QString windowTitle = command.mid(12).trimmed();
        emit statusChanged(QString("Ожидание окна '%1'").arg(windowTitle));
        Sleep(1000);
    }
    else {
        emit statusChanged(QString("Неизвестная команда: %1").arg(command));
    }
}

// =========== AutomatorWidget ===========

AutomatorWidget::AutomatorWidget(QWidget *parent) 
    : QWidget(parent)
    , m_tabWidget(nullptr)
    , m_editor(nullptr)
    , m_lineNumbers(nullptr)
    , m_outputBrowser(nullptr)
    , m_startButton(nullptr)
    , m_stopButton(nullptr)
    , m_recordButton(nullptr)
    , m_loadButton(nullptr)
    , m_saveButton(nullptr)
    , m_pythonButton(nullptr)
    , m_pythonStopButton(nullptr)
    , m_statusLabel(nullptr)
    , m_lineColLabel(nullptr)
    , m_fontSizeCombo(nullptr)
    , m_templateCombo(nullptr)
    , m_menuBar(nullptr)
    , m_worker(nullptr)
    , m_pythonProcess(nullptr)
    , m_recordingTimer(nullptr)
    , m_currentFile("")
    , m_pythonPath("python")
    , m_tempPythonFile("")
    , m_isRecording(false)
    , m_isModified(false)
{
    setupUI();
    setupMenu();
    setupEditor();
    findPython(); // Найти Python при запуске
}

AutomatorWidget::~AutomatorWidget()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestInterruption();
        m_worker->wait();
        delete m_worker;
    }
    
    if (m_pythonProcess && m_pythonProcess->state() == QProcess::Running) {
        m_pythonProcess->terminate();
        m_pythonProcess->waitForFinished(1000);
        delete m_pythonProcess;
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
        "C:/Program Files/Python312/python.exe"
    };
    
    for (const QString& path : pythonPaths) {
        QProcess testProcess;
        testProcess.start(path, {"--version"});
        if (testProcess.waitForFinished(1000) && testProcess.exitCode() == 0) {
            m_pythonPath = path;
            m_statusLabel->setText(QString("Python найден: %1").arg(path));
            return;
        }
    }
    
    m_statusLabel->setText("Python не найден. Установите Python и добавьте в PATH");
}

QString AutomatorWidget::createTempPythonFile(const QString& script)
{
    // Создаем временный файл в системной временной директории
    QString tempDir = QDir::tempPath();
    QString fileName = QString("%1/automator_python_%2.py")
                      .arg(tempDir)
                      .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return "";
    }
    
    QTextStream out(&file);
    out << script;
    file.close();
    
    return fileName;
}

void AutomatorWidget::cleanupTempFile()
{
    if (!m_tempPythonFile.isEmpty() && QFile::exists(m_tempPythonFile)) {
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
    for (int i = 8; i <= 24; i += 2) {
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
    connect(m_templateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) { if (index > 0) insertTemplate(); });
    
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
    connect(m_editor, &QTextEdit::cursorPositionChanged, [this]() {
        QTextCursor cursor = m_editor->textCursor();
        int line = cursor.blockNumber() + 1;
        int col = cursor.columnNumber() + 1;
        m_lineColLabel->setText(QString("Строка: %1, Колонка: %2").arg(line).arg(col));
    });
    
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
    
    m_pythonButton = new QPushButton("🐍 Выполнить Python");
    connect(m_pythonButton, &QPushButton::clicked, this, &AutomatorWidget::runPythonScript);
    
    m_pythonStopButton = new QPushButton("⏹ Стоп Python");
    m_pythonStopButton->setEnabled(false);
    connect(m_pythonStopButton, &QPushButton::clicked, this, &AutomatorWidget::stopPythonScript);
    
    m_stopButton = new QPushButton("⏹ Стоп");
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, this, &AutomatorWidget::stopAutomation);
    
    m_startButton = new QPushButton("▶ Выполнить");
    connect(m_startButton, &QPushButton::clicked, this, &AutomatorWidget::startAutomation);
    
    buttonLayout->addWidget(m_recordButton);
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_pythonButton);
    buttonLayout->addWidget(m_pythonStopButton);
    buttonLayout->addWidget(m_stopButton);
    buttonLayout->addWidget(m_startButton);
    
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
    connect(m_recordingTimer, &QTimer::timeout, [this]() {
        if (m_isRecording) {
            m_statusLabel->setText("Запись...");
        }
    });
    
    // Загружаем пример скрипта
    QString exampleScript = 
        "# Пример Python скрипта\n"
        "import time\n"
        "print('Hello from Python!')\n"
        "for i in range(5):\n"
        "    print(f'Counting: {i+1}')\n"
        "    time.sleep(1)\n"
        "print('Done!')\n";
    
    m_editor->setPlainText(exampleScript);
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
    connect(runAction, &QAction::triggered, this, &AutomatorWidget::startAutomation);
    
    QAction *runPythonAction = m_runMenu->addAction("Выполнить Python");
    runPythonAction->setShortcut(QKeySequence("Ctrl+F5"));
    connect(runPythonAction, &QAction::triggered, this, &AutomatorWidget::runPythonScript);
    
    QAction *stopAction = m_runMenu->addAction("Остановить выполнение");
    stopAction->setShortcut(QKeySequence("Shift+F5"));
    connect(stopAction, &QAction::triggered, this, &AutomatorWidget::stopAutomation);
    
    QAction *findPythonAction = m_runMenu->addAction("Найти Python");
    connect(findPythonAction, &QAction::triggered, this, &AutomatorWidget::findPython);
}

void AutomatorWidget::setupEditor()
{
    QFont editorFont("Consolas", 12);
    m_editor->setFont(editorFont);
    m_editor->setTabStopDistance(40);
    
    connect(m_editor, &QTextEdit::textChanged, [this]() {
        m_isModified = true;
        updateTitle();
    });
}

// =========== Слоты ===========

void AutomatorWidget::startAutomation()
{
    QString script = m_editor->toPlainText();
    if (script.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Скрипт пуст!");
        return;
    }
    
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(true);
    m_pythonButton->setEnabled(false);
    m_recordButton->setEnabled(false);
    m_loadButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_statusLabel->setText("Выполнение скрипта...");
    m_statusLabel->setStyleSheet("QLabel { background-color: #ffffcc; color: black; }");
    
    m_worker = new AutomationWorker(script);
    connect(m_worker, &AutomationWorker::automationFinished,
            this, &AutomatorWidget::onAutomationFinished);
    connect(m_worker, &AutomationWorker::statusChanged,
            this, &AutomatorWidget::updateStatus);
    connect(m_worker, &QThread::finished,
            m_worker, &QObject::deleteLater);
    connect(m_worker, &QThread::finished, [this]() {
        m_worker = nullptr;
    });
    
    m_worker->start();
}

void AutomatorWidget::runPythonScript()
{
    QString script = m_editor->toPlainText();
    if (script.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Скрипт пуст!");
        return;
    }
    
    // Очищаем предыдущий временный файл
    cleanupTempFile();
    
    // Создаем новый временный файл
    m_tempPythonFile = createTempPythonFile(script);
    if (m_tempPythonFile.isEmpty()) {
        m_outputBrowser->append("Ошибка: Не удалось создать временный файл");
        return;
    }
    
    m_pythonButton->setEnabled(false);
    m_pythonStopButton->setEnabled(true);
    m_startButton->setEnabled(false);
    m_stopButton->setEnabled(false);
    m_recordButton->setEnabled(false);
    m_loadButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_statusLabel->setText("Выполнение Python скрипта...");
    m_statusLabel->setStyleSheet("QLabel { background-color: #ccffcc; color: black; }");
    
    // Очищаем вывод
    m_outputBrowser->clear();
    m_outputBrowser->append("=== Запуск Python скрипта ===");
    m_outputBrowser->append(QString("Используется Python: %1").arg(m_pythonPath));
    m_outputBrowser->append(QString("Файл скрипта: %1").arg(m_tempPythonFile));
    
    // Создаем процесс Python
    m_pythonProcess = new QProcess(this);
    m_pythonProcess->setProcessChannelMode(QProcess::MergedChannels);
    
    // Подключаем сигналы
    connect(m_pythonProcess, &QProcess::readyReadStandardOutput,
            this, &AutomatorWidget::onPythonOutput);
    connect(m_pythonProcess, &QProcess::readyReadStandardError,
            this, &AutomatorWidget::onPythonError);
    connect(m_pythonProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &AutomatorWidget::onPythonFinished);
    
    // Запускаем Python
    m_pythonProcess->start(m_pythonPath, {m_tempPythonFile});
    
    if (!m_pythonProcess->waitForStarted(3000)) {
        m_outputBrowser->append("Ошибка: Не удалось запустить Python. Убедитесь, что Python установлен и добавлен в PATH");
        m_outputBrowser->append("Попробуйте использовать действие 'Найти Python' в меню 'Выполнение'");
        cleanupTempFile();
        onPythonFinished(-1, QProcess::CrashExit);
        return;
    }
}

void AutomatorWidget::stopPythonScript()
{
    if (m_pythonProcess && m_pythonProcess->state() == QProcess::Running) {
        m_pythonProcess->terminate();
        if (!m_pythonProcess->waitForFinished(1000)) {
            m_pythonProcess->kill();
        }
    }
}

void AutomatorWidget::onPythonFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        m_outputBrowser->append("=== Python скрипт завершился аварийно ===");
    } else {
        m_outputBrowser->append(QString("=== Python скрипт завершен с кодом %1 ===").arg(exitCode));
    }
    
    m_pythonButton->setEnabled(true);
    m_pythonStopButton->setEnabled(false);
    m_startButton->setEnabled(true);
    m_recordButton->setEnabled(true);
    m_loadButton->setEnabled(true);
    m_saveButton->setEnabled(true);
    
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        m_statusLabel->setText("Python скрипт выполнен успешно");
        m_statusLabel->setStyleSheet("QLabel { background-color: #ccffcc; color: black; }");
    } else {
        m_statusLabel->setText("Python скрипт завершился с ошибкой");
        m_statusLabel->setStyleSheet("QLabel { background-color: #ffe0e0; color: black; }");
    }
    
    // Удаляем временный файл
    cleanupTempFile();
    
    if (m_pythonProcess) {
        m_pythonProcess->deleteLater();
        m_pythonProcess = nullptr;
    }
}

void AutomatorWidget::onPythonOutput()
{
    if (m_pythonProcess) {
        QByteArray output = m_pythonProcess->readAllStandardOutput();
        QString outputStr = QString::fromUtf8(output).trimmed();
        if (!outputStr.isEmpty()) {
            m_outputBrowser->append(outputStr);
        }
    }
}

void AutomatorWidget::onPythonError()
{
    if (m_pythonProcess) {
        QByteArray error = m_pythonProcess->readAllStandardError();
        QString errorStr = QString::fromUtf8(error).trimmed();
        if (!errorStr.isEmpty()) {
            m_outputBrowser->append("Ошибка: " + errorStr);
        }
    }
}

void AutomatorWidget::stopAutomation()
{
    if (m_worker && m_worker->isRunning()) {
        m_worker->requestInterruption();
        m_statusLabel->setText("Остановка выполнения...");
    }
}

void AutomatorWidget::onAutomationFinished()
{
    m_startButton->setEnabled(true);
    m_stopButton->setEnabled(false);
    m_pythonButton->setEnabled(true);
    m_recordButton->setEnabled(true);
    m_loadButton->setEnabled(true);
    m_saveButton->setEnabled(true);
    m_statusLabel->setText("Скрипт выполнен!");
    m_statusLabel->setStyleSheet("QLabel { background-color: #ccffcc; color: black; }");
    
    if (m_worker) {
        m_worker = nullptr;
    }
}

void AutomatorWidget::updateStatus(const QString& message)
{
    m_statusLabel->setText(message);
}

void AutomatorWidget::newScript()
{
    if (m_isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Новый скрипт",
            "Сохранить текущий скрипт?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            
        if (reply == QMessageBox::Yes) {
            saveScript();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }
    
    m_editor->clear();
    m_currentFile = "";
    m_isModified = false;
    updateTitle();
    updateLineNumbers();
}

void AutomatorWidget::loadScript()
{
    if (m_isModified) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, "Загрузить скрипт",
            "Сохранить текущий скрипт?",
            QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
            
        if (reply == QMessageBox::Yes) {
            saveScript();
        } else if (reply == QMessageBox::Cancel) {
            return;
        }
    }
    
    QString fileName = QFileDialog::getOpenFileName(this, "Загрузить скрипт", 
                                                   QDir::homePath(), 
                                                   "Скрипты (*.py *.txt *.auto);;Все файлы (*)");
    if (!fileName.isEmpty()) {
        loadScriptFile(fileName);
    }
}

void AutomatorWidget::saveScript()
{
    if (m_currentFile.isEmpty()) {
        saveScriptAs();
    } else {
        saveScriptFile(m_currentFile);
    }
}

void AutomatorWidget::saveScriptAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить скрипт",
                                                   QDir::homePath() + "/untitled.py",
                                                   "Python скрипты (*.py);;Скрипты (*.txt *.auto);;Все файлы (*)");
    if (!fileName.isEmpty()) {
        saveScriptFile(fileName);
    }
}

void AutomatorWidget::updateLineNumbers()
{
    int blockCount = m_editor->document()->blockCount();
    QString numbers;
    
    for (int i = 1; i <= blockCount; ++i) {
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
    
    if (!templateText.isEmpty()) {
        m_editor->textCursor().insertText(templateText + "\n\n");
        m_isModified = true;
        updateTitle();
    }
    
    m_templateCombo->setCurrentIndex(0);
}

void AutomatorWidget::toggleRecording(bool checked)
{
    m_isRecording = checked;
    
    if (checked) {
        m_recordButton->setText("⏹ Стоп запись");
        m_statusLabel->setText("Запись начата...");
        m_statusLabel->setStyleSheet("QLabel { background-color: #ffcccc; color: black; }");
        m_recordingTimer->start(1000);
        
        QDateTime now = QDateTime::currentDateTime();
        m_editor->append(QString("\n# Запись начата: %1\n").arg(now.toString("dd.MM.yyyy HH:mm:ss")));
    } else {
        m_recordButton->setText("⏺ Запись");
        m_statusLabel->setText("Запись остановлена");
        m_statusLabel->setStyleSheet("QLabel { background-color: #e0e0e0; color: black; }");
        m_recordingTimer->stop();
        
        QDateTime now = QDateTime::currentDateTime();
        m_editor->append(QString("# Запись остановлена: %1\n").arg(now.toString("dd.MM.yyyy HH:mm:ss")));
    }
}

// =========== Вспомогательные функции ===========

void AutomatorWidget::loadScriptFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
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
    
    m_statusLabel->setText(QString("Загружен: %1").arg(QFileInfo(fileName).fileName()));
}

void AutomatorWidget::saveScriptFile(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл: " + file.errorString());
        return;
    }
    
    QTextStream out(&file);
    out << m_editor->toPlainText();
    file.close();
    
    m_currentFile = fileName;
    m_isModified = false;
    updateTitle();
    
    m_statusLabel->setText(QString("Сохранен: %1").arg(QFileInfo(fileName).fileName()));
}

void AutomatorWidget::updateTitle()
{
    QString title = "Qt Automator - Редактор скриптов";
    if (!m_currentFile.isEmpty()) {
        title += " - " + QFileInfo(m_currentFile).fileName();
    }
    if (m_isModified) {
        title += " *";
    }
    setWindowTitle(title);
}

QString AutomatorWidget::getScriptTemplate(const QString& name)
{
    if (name == "text_click") {
        return "# Automator Script\nSLEEP 2000\nKEYSTROKE \"Hello World!\"\nCLICK 500, 300\nSLEEP 1000";
    }
    else if (name == "screenshot") {
        return "# Automator Script\nSLEEP 3000\nCAPTURE 0, 0, 1920, 1080, screenshot.png";
    }
    else if (name == "mouse_sequence") {
        return "# Automator Script\nSLEEP 2000\nMOUSE_SEQUENCE 100,100,LEFTDOWN;200,200,MOVE;200,200,LEFTUP";
    }
    
    return "";
}

QString AutomatorWidget::getFileExtension() const
{
    return "auto";
}