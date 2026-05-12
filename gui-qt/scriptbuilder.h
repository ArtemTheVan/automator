#ifndef SCRIPTBUILDER_H
#define SCRIPTBUILDER_H

#include <QDialog>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariantMap>
#include <QWidget>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QFormLayout;

enum class StepKind {
    Comment,
    Sleep,
    Log,
    Keystroke,
    Click,
    Move,
    Drag,
    Rectangle,
    Screenshot,
    OpenApp,
    RawCode
};

struct ScriptStep {
    StepKind kind = StepKind::Comment;
    QVariantMap params;

    QString toPython() const;
    QString summary() const;

    static QString kindName(StepKind k);
    static ScriptStep makeDefault(StepKind k);
};

class ScriptBuilder : public QWidget
{
    Q_OBJECT
public:
    explicit ScriptBuilder(QWidget *parent = nullptr);

    QString generatePython() const;
    void loadFromPython(const QString &source);
    void clearSteps();
    bool isEmpty() const { return m_steps.isEmpty(); }

signals:
    void applyToEditorRequested(const QString &code);
    void importFromEditorRequested();
    void runRequested();

private slots:
    void onAddRequested();
    void onEdit();
    void onDelete();
    void onMoveUp();
    void onMoveDown();
    void onItemDoubleClicked(QListWidgetItem *item);
    void onSelectionChanged();
    void onApply();
    void onImport();

private:
    void setupUI();
    void addStep(StepKind kind);
    void refreshRow(int row);
    void refreshList();
    bool runEditDialog(ScriptStep &step);
    void updateButtons();

    QListWidget *m_list = nullptr;
    QList<ScriptStep> m_steps;

    QPushButton *m_btnUp = nullptr;
    QPushButton *m_btnDown = nullptr;
    QPushButton *m_btnEdit = nullptr;
    QPushButton *m_btnDel = nullptr;

    QPushButton *m_btnApply = nullptr;
    QPushButton *m_btnImport = nullptr;
    QPushButton *m_btnRun = nullptr;
};

class StepEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StepEditDialog(const ScriptStep &step, QWidget *parent = nullptr);
    ScriptStep step() const { return m_step; }

private slots:
    void onAccept();

private:
    void buildForm();

    ScriptStep m_step;
    QFormLayout *m_form = nullptr;
    QMap<QString, QWidget *> m_fields;
};

#endif // SCRIPTBUILDER_H
