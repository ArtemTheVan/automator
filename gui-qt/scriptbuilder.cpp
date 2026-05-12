#include "scriptbuilder.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QShortcut>
#include <QSpinBox>
#include <QStringList>
#include <QVBoxLayout>

// =========== Маркеры генератора ===========

static const char *kBeginMarker = "# === BEGIN_CONSTRUCTOR ===";
static const char *kEndMarker = "# === END_CONSTRUCTOR ===";

// =========== Утилиты для Python-строк ===========

// Возвращает корректное Python-представление строки в одинарных кавычках.
static QString pyStr(const QString &s)
{
    QString out;
    out.reserve(s.size() + 2);
    out.append('\'');
    for (QChar c : s) {
        switch (c.unicode()) {
        case '\\': out.append("\\\\"); break;
        case '\'': out.append("\\'");  break;
        case '\n': out.append("\\n");  break;
        case '\r': out.append("\\r");  break;
        case '\t': out.append("\\t");  break;
        default:
            out.append(c);
        }
    }
    out.append('\'');
    return out;
}

// Декодирует тело Python-строки (без кавычек) с escape-последовательностями.
static QString pyDecodeStringBody(const QString &body)
{
    QString out;
    out.reserve(body.size());
    for (int i = 0; i < body.size(); ++i) {
        QChar c = body[i];
        if (c == '\\' && i + 1 < body.size()) {
            QChar n = body[i + 1];
            switch (n.unicode()) {
            case 'n':  out.append('\n'); break;
            case 'r':  out.append('\r'); break;
            case 't':  out.append('\t'); break;
            case '\\': out.append('\\'); break;
            case '\'': out.append('\''); break;
            case '"':  out.append('"');  break;
            default:   out.append(n);    break;
            }
            ++i;
        } else {
            out.append(c);
        }
    }
    return out;
}

// =========== ScriptStep ===========

QString ScriptStep::kindName(StepKind k)
{
    switch (k) {
    case StepKind::Comment:    return "Комментарий";
    case StepKind::Sleep:      return "Пауза";
    case StepKind::Log:        return "Сообщение в лог";
    case StepKind::Keystroke:  return "Ввод текста";
    case StepKind::Click:      return "Клик мышью";
    case StepKind::Move:       return "Переместить мышь";
    case StepKind::Drag:       return "Перетаскивание";
    case StepKind::Rectangle:  return "Прямоугольник мышью";
    case StepKind::Screenshot: return "Скриншот";
    case StepKind::OpenApp:    return "Запустить программу";
    case StepKind::RawCode:    return "Произвольный код";
    }
    return "?";
}

ScriptStep ScriptStep::makeDefault(StepKind k)
{
    ScriptStep s;
    s.kind = k;
    switch (k) {
    case StepKind::Comment:
        s.params["text"] = QString("Описание шага");
        break;
    case StepKind::Sleep:
        s.params["seconds"] = 1.0;
        break;
    case StepKind::Log:
        s.params["level"] = QString("info");
        s.params["message"] = QString("Сообщение");
        break;
    case StepKind::Keystroke:
        s.params["text"] = QString("Hello");
        break;
    case StepKind::Click:
        s.params["x"] = 500;
        s.params["y"] = 300;
        s.params["button"] = QString("left");
        break;
    case StepKind::Move:
        s.params["x"] = 500;
        s.params["y"] = 300;
        break;
    case StepKind::Drag:
        s.params["x1"] = 100;
        s.params["y1"] = 100;
        s.params["x2"] = 300;
        s.params["y2"] = 300;
        s.params["button"] = QString("left");
        break;
    case StepKind::Rectangle:
        s.params["x"] = 200;
        s.params["y"] = 200;
        s.params["w"] = 150;
        s.params["h"] = 150;
        break;
    case StepKind::Screenshot:
        s.params["x"] = 0;
        s.params["y"] = 0;
        s.params["w"] = 1920;
        s.params["h"] = 1080;
        s.params["filename"] = QString("screenshot.bmp");
        break;
    case StepKind::OpenApp:
        s.params["command"] = QString("notepad.exe");
        s.params["args"] = QString("");
        break;
    case StepKind::RawCode:
        s.params["code"] = QString("# python code");
        break;
    }
    return s;
}

QString ScriptStep::toPython() const
{
    switch (kind) {
    case StepKind::Comment: {
        const QString text = params.value("text").toString();
        QStringList lines = text.split('\n');
        for (QString &l : lines) l = "# " + l;
        return lines.join('\n');
    }
    case StepKind::Sleep: {
        const double s = params.value("seconds").toDouble();
        return QString("automator.sleep(%1)").arg(s, 0, 'g', 6);
    }
    case StepKind::Log: {
        const QString level = params.value("level").toString();
        const QString msg   = params.value("message").toString();
        const QString lvl   = (level == "warning" || level == "error") ? level : "info";
        return QString("automator.%1(%2)").arg(lvl, pyStr(msg));
    }
    case StepKind::Keystroke: {
        const QString text = params.value("text").toString();
        return QString("automator.keystroke(%1)").arg(pyStr(text));
    }
    case StepKind::Click: {
        const int x = params.value("x").toInt();
        const int y = params.value("y").toInt();
        const QString button = params.value("button").toString();
        if (button.isEmpty() || button == "left")
            return QString("automator.click(%1, %2)").arg(x).arg(y);
        return QString("automator.click(%1, %2, button=%3)").arg(x).arg(y).arg(pyStr(button));
    }
    case StepKind::Move: {
        const int x = params.value("x").toInt();
        const int y = params.value("y").toInt();
        return QString("automator.move(%1, %2)").arg(x).arg(y);
    }
    case StepKind::Drag: {
        const int x1 = params.value("x1").toInt();
        const int y1 = params.value("y1").toInt();
        const int x2 = params.value("x2").toInt();
        const int y2 = params.value("y2").toInt();
        const QString button = params.value("button").toString();
        if (button.isEmpty() || button == "left")
            return QString("automator.drag(%1, %2, %3, %4)").arg(x1).arg(y1).arg(x2).arg(y2);
        return QString("automator.drag(%1, %2, %3, %4, button=%5)")
            .arg(x1).arg(y1).arg(x2).arg(y2).arg(pyStr(button));
    }
    case StepKind::Rectangle: {
        const int x = params.value("x").toInt();
        const int y = params.value("y").toInt();
        const int w = params.value("w").toInt();
        const int h = params.value("h").toInt();
        return QString("automator.rectangle(%1, %2, %3, %4)").arg(x).arg(y).arg(w).arg(h);
    }
    case StepKind::Screenshot: {
        const int x = params.value("x").toInt();
        const int y = params.value("y").toInt();
        const int w = params.value("w").toInt();
        const int h = params.value("h").toInt();
        const QString file = params.value("filename").toString();
        return QString("automator.capture_screen(%1, %2, %3, %4, %5)")
            .arg(x).arg(y).arg(w).arg(h).arg(pyStr(file));
    }
    case StepKind::OpenApp: {
        const QString cmd = params.value("command").toString();
        const QString argsRaw = params.value("args").toString();
        QStringList parts;
        parts << pyStr(cmd);
        const QStringList rawArgs = argsRaw.split('\n', Qt::SkipEmptyParts);
        for (const QString &a : rawArgs) parts << pyStr(a.trimmed());
        return QString("subprocess.Popen([%1])").arg(parts.join(", "));
    }
    case StepKind::RawCode:
        return params.value("code").toString();
    }
    return "";
}

QString ScriptStep::summary() const
{
    switch (kind) {
    case StepKind::Comment: {
        QString t = params.value("text").toString();
        t.replace('\n', ' ');
        return QString("💬  %1").arg(t.left(60));
    }
    case StepKind::Sleep:
        return QString("⏱  Пауза %1 с").arg(params.value("seconds").toDouble(), 0, 'g', 4);
    case StepKind::Log: {
        const QString level = params.value("level").toString();
        QString msg = params.value("message").toString();
        msg.replace('\n', ' ');
        return QString("📝  Лог (%1): %2").arg(level, msg.left(50));
    }
    case StepKind::Keystroke: {
        QString t = params.value("text").toString();
        t.replace('\n', "\\n");
        return QString("⌨  Ввод: \"%1\"").arg(t.left(50));
    }
    case StepKind::Click: {
        const QString btn = params.value("button").toString();
        QString btnRu = "ЛКМ";
        if (btn == "right") btnRu = "ПКМ";
        else if (btn == "middle") btnRu = "СКМ";
        return QString("🖱  Клик %1 (%2, %3)").arg(btnRu)
            .arg(params.value("x").toInt()).arg(params.value("y").toInt());
    }
    case StepKind::Move:
        return QString("➡  Переместить мышь в (%1, %2)")
            .arg(params.value("x").toInt()).arg(params.value("y").toInt());
    case StepKind::Drag:
        return QString("↔  Перетаскивание (%1,%2)→(%3,%4)")
            .arg(params.value("x1").toInt()).arg(params.value("y1").toInt())
            .arg(params.value("x2").toInt()).arg(params.value("y2").toInt());
    case StepKind::Rectangle:
        return QString("□  Прямоугольник (%1,%2) %3×%4")
            .arg(params.value("x").toInt()).arg(params.value("y").toInt())
            .arg(params.value("w").toInt()).arg(params.value("h").toInt());
    case StepKind::Screenshot:
        return QString("📷  Скриншот %1×%2 → %3")
            .arg(params.value("w").toInt()).arg(params.value("h").toInt())
            .arg(params.value("filename").toString());
    case StepKind::OpenApp: {
        const QString cmd = params.value("command").toString();
        const QString args = params.value("args").toString().replace('\n', ' ');
        if (args.isEmpty()) return QString("▶  Запустить: %1").arg(cmd);
        return QString("▶  Запустить: %1 %2").arg(cmd, args);
    }
    case StepKind::RawCode: {
        QString c = params.value("code").toString();
        c.replace(QChar('\n'), QStringLiteral(" ⏎ "));
        return QString("{ }  %1").arg(c.left(70));
    }
    }
    return "?";
}

// =========== StepEditDialog ===========

StepEditDialog::StepEditDialog(const ScriptStep &step, QWidget *parent)
    : QDialog(parent), m_step(step)
{
    setWindowTitle(QString("Редактирование: %1").arg(ScriptStep::kindName(step.kind)));
    setMinimumWidth(450);

    QVBoxLayout *main = new QVBoxLayout(this);

    m_form = new QFormLayout;
    buildForm();
    main->addLayout(m_form);

    QDialogButtonBox *box = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(box, &QDialogButtonBox::accepted, this, &StepEditDialog::onAccept);
    connect(box, &QDialogButtonBox::rejected, this, &QDialog::reject);
    main->addWidget(box);
}

static QSpinBox *makeIntField(int value, int min = -100000, int max = 100000)
{
    QSpinBox *sb = new QSpinBox;
    sb->setRange(min, max);
    sb->setValue(value);
    return sb;
}

static QDoubleSpinBox *makeDoubleField(double value, double min = 0.0, double max = 3600.0)
{
    QDoubleSpinBox *sb = new QDoubleSpinBox;
    sb->setRange(min, max);
    sb->setDecimals(3);
    sb->setSingleStep(0.1);
    sb->setValue(value);
    return sb;
}

void StepEditDialog::buildForm()
{
    const QVariantMap &p = m_step.params;
    auto addInt    = [&](const QString &key, const QString &label, int def = 0) {
        QSpinBox *w = makeIntField(p.value(key, def).toInt());
        m_form->addRow(label, w);
        m_fields.insert(key, w);
    };
    auto addDouble = [&](const QString &key, const QString &label, double def = 0.0,
                         double min = 0.0, double max = 3600.0) {
        QDoubleSpinBox *w = makeDoubleField(p.value(key, def).toDouble(), min, max);
        m_form->addRow(label, w);
        m_fields.insert(key, w);
    };
    auto addLine   = [&](const QString &key, const QString &label, const QString &def = "") {
        QLineEdit *w = new QLineEdit(p.value(key, def).toString());
        m_form->addRow(label, w);
        m_fields.insert(key, w);
    };
    auto addMultiline = [&](const QString &key, const QString &label, const QString &def = "") {
        QPlainTextEdit *w = new QPlainTextEdit(p.value(key, def).toString());
        w->setMinimumHeight(80);
        m_form->addRow(label, w);
        m_fields.insert(key, w);
    };
    auto addCombo = [&](const QString &key, const QString &label,
                        const QStringList &items, const QString &def) {
        QComboBox *w = new QComboBox;
        w->addItems(items);
        int idx = items.indexOf(p.value(key, def).toString());
        if (idx >= 0) w->setCurrentIndex(idx);
        m_form->addRow(label, w);
        m_fields.insert(key, w);
    };

    switch (m_step.kind) {
    case StepKind::Comment:
        addMultiline("text", "Текст комментария:", "Описание");
        break;
    case StepKind::Sleep:
        addDouble("seconds", "Длительность (с):", 1.0);
        break;
    case StepKind::Log:
        addCombo("level", "Уровень:", {"info", "warning", "error"}, "info");
        addLine("message", "Сообщение:", "Сообщение");
        break;
    case StepKind::Keystroke:
        addMultiline("text", "Текст для ввода:", "Hello");
        break;
    case StepKind::Click:
        addInt("x", "X (пиксель):");
        addInt("y", "Y (пиксель):");
        addCombo("button", "Кнопка:", {"left", "right", "middle"}, "left");
        break;
    case StepKind::Move:
        addInt("x", "X (пиксель):");
        addInt("y", "Y (пиксель):");
        break;
    case StepKind::Drag:
        addInt("x1", "Начало X:");
        addInt("y1", "Начало Y:");
        addInt("x2", "Конец X:");
        addInt("y2", "Конец Y:");
        addCombo("button", "Кнопка:", {"left", "right", "middle"}, "left");
        break;
    case StepKind::Rectangle:
        addInt("x", "X:");
        addInt("y", "Y:");
        addInt("w", "Ширина:", 100);
        addInt("h", "Высота:", 100);
        break;
    case StepKind::Screenshot:
        addInt("x", "X:");
        addInt("y", "Y:");
        addInt("w", "Ширина:", 1920);
        addInt("h", "Высота:", 1080);
        addLine("filename", "Имя файла:", "screenshot.bmp");
        break;
    case StepKind::OpenApp:
        addLine("command", "Команда (.exe):", "notepad.exe");
        addMultiline("args", "Аргументы (по одному в строке):");
        break;
    case StepKind::RawCode:
        addMultiline("code", "Python-код:");
        if (auto *w = qobject_cast<QPlainTextEdit *>(m_fields.value("code"))) {
            w->setMinimumHeight(160);
            QFont f("Consolas", 10);
            w->setFont(f);
        }
        break;
    }
}

void StepEditDialog::onAccept()
{
    for (auto it = m_fields.constBegin(); it != m_fields.constEnd(); ++it) {
        QWidget *w = it.value();
        const QString &key = it.key();
        if (auto *sb = qobject_cast<QSpinBox *>(w)) {
            m_step.params[key] = sb->value();
        } else if (auto *dsb = qobject_cast<QDoubleSpinBox *>(w)) {
            m_step.params[key] = dsb->value();
        } else if (auto *le = qobject_cast<QLineEdit *>(w)) {
            m_step.params[key] = le->text();
        } else if (auto *pe = qobject_cast<QPlainTextEdit *>(w)) {
            m_step.params[key] = pe->toPlainText();
        } else if (auto *cb = qobject_cast<QComboBox *>(w)) {
            m_step.params[key] = cb->currentText();
        }
    }
    accept();
}

// =========== ScriptBuilder ===========

ScriptBuilder::ScriptBuilder(QWidget *parent) : QWidget(parent)
{
    setupUI();
    updateButtons();
}

void ScriptBuilder::setupUI()
{
    QVBoxLayout *outer = new QVBoxLayout(this);
    outer->setContentsMargins(4, 4, 4, 4);

    QHBoxLayout *body = new QHBoxLayout;

    // ── Левая колонка: список + кнопки редактирования
    QVBoxLayout *leftCol = new QVBoxLayout;
    leftCol->addWidget(new QLabel("Шаги сценария:"));

    m_list = new QListWidget;
    m_list->setAlternatingRowColors(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &ScriptBuilder::onItemDoubleClicked);
    connect(m_list, &QListWidget::itemSelectionChanged,
            this, &ScriptBuilder::onSelectionChanged);
    leftCol->addWidget(m_list, 1);

    QHBoxLayout *rowBtns = new QHBoxLayout;
    m_btnUp   = new QPushButton("▲ Вверх");
    m_btnDown = new QPushButton("▼ Вниз");
    m_btnEdit = new QPushButton("✎ Изменить");
    m_btnDel  = new QPushButton("✗ Удалить");
    connect(m_btnUp,   &QPushButton::clicked, this, &ScriptBuilder::onMoveUp);
    connect(m_btnDown, &QPushButton::clicked, this, &ScriptBuilder::onMoveDown);
    connect(m_btnEdit, &QPushButton::clicked, this, &ScriptBuilder::onEdit);
    connect(m_btnDel,  &QPushButton::clicked, this, &ScriptBuilder::onDelete);
    rowBtns->addWidget(m_btnUp);
    rowBtns->addWidget(m_btnDown);
    rowBtns->addStretch();
    rowBtns->addWidget(m_btnEdit);
    rowBtns->addWidget(m_btnDel);
    leftCol->addLayout(rowBtns);

    body->addLayout(leftCol, 3);

    // ── Правая колонка: палитра кнопок добавления
    QVBoxLayout *rightCol = new QVBoxLayout;
    rightCol->addWidget(new QLabel("Добавить шаг:"));

    struct Pal { StepKind kind; const char *label; };
    const QList<Pal> palette = {
        {StepKind::Comment,    "💬  Комментарий"},
        {StepKind::Sleep,      "⏱  Пауза"},
        {StepKind::Log,        "📝  Сообщение в лог"},
        {StepKind::Keystroke,  "⌨  Ввод текста"},
        {StepKind::Click,      "🖱  Клик мышью"},
        {StepKind::Move,       "➡  Переместить мышь"},
        {StepKind::Drag,       "↔  Перетаскивание"},
        {StepKind::Rectangle,  "□  Прямоугольник мышью"},
        {StepKind::Screenshot, "📷  Скриншот области"},
        {StepKind::OpenApp,    "▶  Запустить программу"},
        {StepKind::RawCode,    "{ }  Произвольный код"},
    };
    for (const Pal &p : palette) {
        QPushButton *b = new QPushButton(p.label);
        b->setProperty("stepKind", static_cast<int>(p.kind));
        connect(b, &QPushButton::clicked, this, &ScriptBuilder::onAddRequested);
        rightCol->addWidget(b);
    }
    rightCol->addStretch();

    body->addLayout(rightCol, 2);

    outer->addLayout(body, 1);

    // ── Нижняя панель: синхронизация + запуск
    QHBoxLayout *bottom = new QHBoxLayout;
    m_btnApply  = new QPushButton("📤 Применить к коду");
    m_btnApply->setToolTip("Сгенерировать Python-код из шагов и записать в редактор");
    m_btnImport = new QPushButton("📥 Импорт из кода");
    m_btnImport->setToolTip("Прочитать текущий код в редакторе и восстановить шаги");
    m_btnRun    = new QPushButton("▶ Запустить");
    m_btnRun->setToolTip("Записать код в редактор и запустить скрипт");
    connect(m_btnApply,  &QPushButton::clicked, this, &ScriptBuilder::onApply);
    connect(m_btnImport, &QPushButton::clicked, this, &ScriptBuilder::onImport);
    connect(m_btnRun,    &QPushButton::clicked, this, [this]() {
        emit applyToEditorRequested(generatePython());
        emit runRequested();
    });
    bottom->addWidget(m_btnApply);
    bottom->addWidget(m_btnImport);
    bottom->addStretch();
    bottom->addWidget(m_btnRun);
    outer->addLayout(bottom);

    // Шорткат на удаление выбранного шага
    QShortcut *delSc = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(delSc, &QShortcut::activated, this, &ScriptBuilder::onDelete);
}

void ScriptBuilder::updateButtons()
{
    const int row = m_list->currentRow();
    const bool hasSel = row >= 0 && row < m_steps.size();
    m_btnEdit->setEnabled(hasSel);
    m_btnDel->setEnabled(hasSel);
    m_btnUp->setEnabled(hasSel && row > 0);
    m_btnDown->setEnabled(hasSel && row < m_steps.size() - 1);
}

void ScriptBuilder::onSelectionChanged()
{
    updateButtons();
}

void ScriptBuilder::onAddRequested()
{
    QPushButton *src = qobject_cast<QPushButton *>(sender());
    if (!src) return;
    const StepKind kind = static_cast<StepKind>(src->property("stepKind").toInt());
    addStep(kind);
}

void ScriptBuilder::addStep(StepKind kind)
{
    ScriptStep step = ScriptStep::makeDefault(kind);
    // Сразу открываем диалог редактирования, чтобы новый шаг был осмысленным.
    if (!runEditDialog(step)) return;

    int insertAt = m_list->currentRow();
    if (insertAt < 0) insertAt = m_steps.size();
    else insertAt += 1;
    m_steps.insert(insertAt, step);
    refreshList();
    m_list->setCurrentRow(insertAt);
}

bool ScriptBuilder::runEditDialog(ScriptStep &step)
{
    StepEditDialog dlg(step, this);
    if (dlg.exec() != QDialog::Accepted) return false;
    step = dlg.step();
    return true;
}

void ScriptBuilder::onEdit()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_steps.size()) return;
    ScriptStep step = m_steps[row];
    if (!runEditDialog(step)) return;
    m_steps[row] = step;
    refreshRow(row);
}

void ScriptBuilder::onItemDoubleClicked(QListWidgetItem *)
{
    onEdit();
}

void ScriptBuilder::onDelete()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_steps.size()) return;
    m_steps.removeAt(row);
    refreshList();
    if (row < m_steps.size()) m_list->setCurrentRow(row);
    else if (!m_steps.isEmpty()) m_list->setCurrentRow(m_steps.size() - 1);
    updateButtons();
}

void ScriptBuilder::onMoveUp()
{
    const int row = m_list->currentRow();
    if (row <= 0 || row >= m_steps.size()) return;
    m_steps.swapItemsAt(row, row - 1);
    refreshList();
    m_list->setCurrentRow(row - 1);
}

void ScriptBuilder::onMoveDown()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_steps.size() - 1) return;
    m_steps.swapItemsAt(row, row + 1);
    refreshList();
    m_list->setCurrentRow(row + 1);
}

void ScriptBuilder::onApply()
{
    if (m_steps.isEmpty()) {
        QMessageBox::information(this, "Пусто",
            "Список шагов пуст — нечего применять.");
        return;
    }
    emit applyToEditorRequested(generatePython());
}

void ScriptBuilder::onImport()
{
    emit importFromEditorRequested();
}

void ScriptBuilder::refreshRow(int row)
{
    if (row < 0 || row >= m_steps.size()) return;
    QListWidgetItem *item = m_list->item(row);
    if (!item) return;
    item->setText(QString("%1. %2")
                      .arg(row + 1, 2, 10, QChar(' '))
                      .arg(m_steps[row].summary()));
}

void ScriptBuilder::refreshList()
{
    const int currentRow = m_list->currentRow();
    m_list->clear();
    for (int i = 0; i < m_steps.size(); ++i) {
        m_list->addItem(QString("%1. %2")
                            .arg(i + 1, 2, 10, QChar(' '))
                            .arg(m_steps[i].summary()));
    }
    if (currentRow >= 0 && currentRow < m_steps.size())
        m_list->setCurrentRow(currentRow);
    updateButtons();
}

void ScriptBuilder::clearSteps()
{
    m_steps.clear();
    refreshList();
}

QString ScriptBuilder::generatePython() const
{
    QStringList lines;
    lines << "# -*- coding: utf-8 -*-";
    lines << "# Сгенерировано конструктором сценариев Automator.";
    lines << "# Блок между маркерами BEGIN_CONSTRUCTOR/END_CONSTRUCTOR";
    lines << "# может быть импортирован обратно в конструктор.";
    lines << "import time";
    lines << "import subprocess";
    lines << "from wrapper_automator import automator";
    lines << "";
    lines << kBeginMarker;
    for (const ScriptStep &s : m_steps) {
        lines << s.toPython();
    }
    lines << kEndMarker;
    lines << "";
    return lines.join('\n');
}

// =========== Парсер ===========

// Регулярки для распознавания шагов. Все ожидают строку, у которой уже
// удалены ведущие пробелы.
namespace {

struct Patterns {
    QRegularExpression sleep;
    QRegularExpression timeSleep;
    QRegularExpression keystroke;
    QRegularExpression clickFull;
    QRegularExpression clickShort;
    QRegularExpression move;
    QRegularExpression dragFull;
    QRegularExpression dragShort;
    QRegularExpression rect;
    QRegularExpression capture;
    QRegularExpression popen;
    QRegularExpression logCall;
    QRegularExpression comment;

    Patterns()
        : sleep     (R"(^automator\.sleep\(\s*([-+]?\d+(?:\.\d+)?)\s*\)$)")
        , timeSleep (R"(^time\.sleep\(\s*([-+]?\d+(?:\.\d+)?)\s*\)$)")
        , keystroke (R"(^automator\.keystroke\(\s*'((?:[^'\\]|\\.)*)'\s*\)$)")
        , clickFull (R"(^automator\.click\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*button\s*=\s*'([a-z]+)'\s*\)$)")
        , clickShort(R"(^automator\.click\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)$)")
        , move      (R"(^automator\.move\(\s*(-?\d+)\s*,\s*(-?\d+)\s*\)$)")
        , dragFull  (R"(^automator\.drag\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*button\s*=\s*'([a-z]+)'\s*\)$)")
        , dragShort (R"(^automator\.drag\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\)$)")
        , rect      (R"(^automator\.rectangle\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\)$)")
        , capture   (R"(^automator\.capture_screen\(\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*'((?:[^'\\]|\\.)*)'\s*\)$)")
        , popen     (R"(^subprocess\.Popen\(\s*\[(.*)\]\s*\)$)")
        , logCall   (R"(^automator\.(info|warning|error)\(\s*'((?:[^'\\]|\\.)*)'\s*\)$)")
        , comment   (R"(^#\s?(.*)$)")
    {}
};

// Парсит Python-список строковых литералов в одинарных кавычках вида
// 'a', 'b', 'c' (с поддержкой экранирования). Возвращает QStringList.
static QStringList parsePyStringList(const QString &inner)
{
    static const QRegularExpression item(R"('((?:[^'\\]|\\.)*)')");
    QStringList out;
    QRegularExpressionMatchIterator it = item.globalMatch(inner);
    while (it.hasNext()) {
        out << pyDecodeStringBody(it.next().captured(1));
    }
    return out;
}

static bool isBoilerplate(const QString &stripped)
{
    if (stripped.isEmpty()) return true;
    if (stripped.startsWith("import "))      return true;
    if (stripped.startsWith("from "))         return true;
    if (stripped.startsWith("def "))          return true;
    if (stripped.startsWith("if __name__"))   return true;
    if (stripped == "main()")                 return true;
    if (stripped == "return")                 return true;
    if (stripped.startsWith("# -*-"))         return true;
    if (stripped.startsWith("\"\"\""))        return true;
    if (stripped.startsWith("'''"))           return true;
    return false;
}

} // namespace

void ScriptBuilder::loadFromPython(const QString &source)
{
    m_steps.clear();

    // Если в коде есть маркеры — парсим только содержимое между ними.
    QString body = source;
    const int begin = source.indexOf(kBeginMarker);
    const int end   = source.indexOf(kEndMarker);
    if (begin >= 0 && end > begin) {
        const int start = begin + static_cast<int>(qstrlen(kBeginMarker));
        body = source.mid(start, end - start);
    }

    static const Patterns P;

    const QStringList rawLines = body.split('\n');
    for (const QString &rawLine : rawLines) {
        const QString line = rawLine.trimmed();

        if (isBoilerplate(line)) continue;

        QRegularExpressionMatch m;

        // Комментарий — обязательно проверяем ПЕРЕД остальными.
        if (line.startsWith('#')) {
            m = P.comment.match(line);
            if (m.hasMatch()) {
                ScriptStep s;
                s.kind = StepKind::Comment;
                s.params["text"] = m.captured(1);
                m_steps << s;
                continue;
            }
        }

        m = P.sleep.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Sleep;
            s.params["seconds"] = m.captured(1).toDouble();
            m_steps << s; continue;
        }
        m = P.timeSleep.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Sleep;
            s.params["seconds"] = m.captured(1).toDouble();
            m_steps << s; continue;
        }
        m = P.keystroke.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Keystroke;
            s.params["text"] = pyDecodeStringBody(m.captured(1));
            m_steps << s; continue;
        }
        m = P.clickFull.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Click;
            s.params["x"] = m.captured(1).toInt();
            s.params["y"] = m.captured(2).toInt();
            s.params["button"] = m.captured(3);
            m_steps << s; continue;
        }
        m = P.clickShort.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Click;
            s.params["x"] = m.captured(1).toInt();
            s.params["y"] = m.captured(2).toInt();
            s.params["button"] = QString("left");
            m_steps << s; continue;
        }
        m = P.move.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Move;
            s.params["x"] = m.captured(1).toInt();
            s.params["y"] = m.captured(2).toInt();
            m_steps << s; continue;
        }
        m = P.dragFull.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Drag;
            s.params["x1"] = m.captured(1).toInt();
            s.params["y1"] = m.captured(2).toInt();
            s.params["x2"] = m.captured(3).toInt();
            s.params["y2"] = m.captured(4).toInt();
            s.params["button"] = m.captured(5);
            m_steps << s; continue;
        }
        m = P.dragShort.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Drag;
            s.params["x1"] = m.captured(1).toInt();
            s.params["y1"] = m.captured(2).toInt();
            s.params["x2"] = m.captured(3).toInt();
            s.params["y2"] = m.captured(4).toInt();
            s.params["button"] = QString("left");
            m_steps << s; continue;
        }
        m = P.rect.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Rectangle;
            s.params["x"] = m.captured(1).toInt();
            s.params["y"] = m.captured(2).toInt();
            s.params["w"] = m.captured(3).toInt();
            s.params["h"] = m.captured(4).toInt();
            m_steps << s; continue;
        }
        m = P.capture.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Screenshot;
            s.params["x"] = m.captured(1).toInt();
            s.params["y"] = m.captured(2).toInt();
            s.params["w"] = m.captured(3).toInt();
            s.params["h"] = m.captured(4).toInt();
            s.params["filename"] = pyDecodeStringBody(m.captured(5));
            m_steps << s; continue;
        }
        m = P.popen.match(line);
        if (m.hasMatch()) {
            const QStringList items = parsePyStringList(m.captured(1));
            if (!items.isEmpty()) {
                ScriptStep s; s.kind = StepKind::OpenApp;
                s.params["command"] = items.first();
                s.params["args"] = items.mid(1).join('\n');
                m_steps << s; continue;
            }
        }
        m = P.logCall.match(line);
        if (m.hasMatch()) {
            ScriptStep s; s.kind = StepKind::Log;
            s.params["level"] = m.captured(1);
            s.params["message"] = pyDecodeStringBody(m.captured(2));
            m_steps << s; continue;
        }

        // Не распознано — сохраняем как сырой код.
        ScriptStep s; s.kind = StepKind::RawCode;
        s.params["code"] = line;
        m_steps << s;
    }

    refreshList();
}
