#include "automatorwidget.h"
#include <QDebug>
#include <QScrollBar>
#include <QTextBlock>
#include <QStringConverter>
#include <QRegularExpression>
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
    , m_editor(nullptr)
    , m_lineNumbers(nullptr)
    , m_startButton(nullptr)
    , m_stopButton(nullptr)
    , m_recordButton(nullptr)
    , m_loadButton(nullptr)
    , m_saveButton(nullptr)
    , m_statusLabel(nullptr)
    , m_lineColLabel(nullptr)
    , m_fontSizeCombo(nullptr)
    , m_templateCombo(nullptr)
    , m_menuBar(nullptr)
    , m_worker(nullptr)
    , m_recordingTimer(nullptr)
    , m_currentFile("")
    , m_isRecording(false)
    , m_isModified(false)
{
    setupUI();
    setupMenu();
    setupEditor();
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
    
    // Область редактора с номерами строк
    QHBoxLayout *editorLayout = new QHBoxLayout();
    
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
    
    m_stopButton = new QPushButton("⏹ Стоп");
    m_stopButton->setEnabled(false);
    connect(m_stopButton, &QPushButton::clicked, this, &AutomatorWidget::stopAutomation);
    
    m_startButton = new QPushButton("▶ Выполнить");
    connect(m_startButton, &QPushButton::clicked, this, &AutomatorWidget::startAutomation);
    
    buttonLayout->addWidget(m_recordButton);
    buttonLayout->addWidget(m_loadButton);
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addStretch();
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
    mainLayout->addLayout(editorLayout, 1);
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
        "# Пример скрипта автоматизации\n"
        "# ============================\n\n"
        "SLEEP 2000\n"
        "KEYSTROKE \"Hello World!\"\n"
        "CLICK 960, 540\n"
        "SLEEP 1000\n"
        "CAPTURE 0, 0, 1920, 1080, screenshot.png\n";
    
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
    connect(m_worker, &AutomationWorker::finished,
            m_worker, &QObject::deleteLater);
    
    m_worker->start();
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
    m_recordButton->setEnabled(true);
    m_loadButton->setEnabled(true);
    m_saveButton->setEnabled(true);
    m_statusLabel->setText("Скрипт выполнен!");
    m_statusLabel->setStyleSheet("QLabel { background-color: #ccffcc; color: black; }");
    
    if (m_worker) {
        m_worker->deleteLater();
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
                                                   "Скрипты (*.txt *.auto);;Все файлы (*)");
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
                                                   QDir::homePath() + "/untitled.auto",
                                                   "Скрипты (*.auto);;Текстовые файлы (*.txt);;Все файлы (*)");
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

void AutomatorWidget::showLineNumbers(bool show)
{
    m_lineNumbers->setVisible(show);
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
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
    #else
    in.setCodec("UTF-8");
    #endif
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
    #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
    #else
    out.setCodec("UTF-8");
    #endif
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
        return "SLEEP 2000\nKEYSTROKE \"Hello World!\"\nCLICK 500, 300\nSLEEP 1000";
    }
    else if (name == "screenshot") {
        return "SLEEP 3000\nCAPTURE 0, 0, 1920, 1080, screenshot.png";
    }
    else if (name == "mouse_sequence") {
        return "SLEEP 2000\nMOUSE_SEQUENCE 100,100,LEFTDOWN;200,200,MOVE;200,200,LEFTUP";
    }
    
    return "";
}
