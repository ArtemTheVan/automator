#include "automatorwidget.h"
#include <QDebug>
#include <windows.h>

// Реализация потока автоматизации
void AutomationWorker::run()
{
    emit statusChanged("Подготовка к запуску...");
    
    // Даем время переключиться на нужное окно
    emit statusChanged("Переключитесь на целевое окно (5 секунд)...");
    
    // Используем Windows Sleep, а не Qt sleep
    Sleep(5000);
    
    emit statusChanged("Выполнение сценария...");
    
    // Выполняем автоматизацию
    simulate_keystroke("hello!");
    
    // Пример клика - в центре экрана
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    simulate_mouse_click_at(screenGeometry.width() / 2, screenGeometry.height() / 2);
    
    emit statusChanged("Сохранение скриншота...");
    capture_screen_region(500, 500, 100, 100, "screenshot_qt.bmp");
    
    emit statusChanged("Автоматизация завершена!");
    emit automationFinished();
}

// Конструктор основного окна
AutomatorWidget::AutomatorWidget(QWidget *parent) 
    : QWidget(parent)
    , m_startButton(nullptr)
    , m_statusLabel(nullptr)
    , m_infoLabel(nullptr)
    , m_worker(nullptr)
{
    setupUI();
}

void AutomatorWidget::setupUI()
{
    // Основной layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Заголовок
    QLabel *titleLabel = new QLabel("GUI Automator с Qt 6");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    
    // Кнопка запуска
    m_startButton = new QPushButton("Запуск автоматизации");
    m_startButton->setMinimumHeight(50);
    
    // Статус
    m_statusLabel = new QLabel("Готов к работе");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setMinimumHeight(30);
    
    // Информация
    m_infoLabel = new QLabel(
        "После нажатия кнопки у вас будет 5 секунд\n"
        "чтобы переключиться в нужное окно.\n\n"
        "Приложение выполнит ввод текста 'hello!'\n"
        "и клик в центре экрана."
    );
    m_infoLabel->setAlignment(Qt::AlignCenter);
    
    // Сборка интерфейса
    mainLayout->addWidget(titleLabel);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(m_infoLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(m_startButton);
    mainLayout->addStretch();
    
    // Установка layout
    setLayout(mainLayout);
    
    // Соединение сигналов и слотов
    connect(m_startButton, &QPushButton::clicked, 
            this, &AutomatorWidget::startAutomation);
}

void AutomatorWidget::startAutomation()
{
    m_startButton->setEnabled(false);
    m_startButton->setText("Выполняется...");
    m_statusLabel->setText("Запуск автоматизации...");
    
    // Создаем и запускаем рабочий поток
    m_worker = new AutomationWorker();
    connect(m_worker, &AutomationWorker::automationFinished,
            this, &AutomatorWidget::onAutomationFinished);
    connect(m_worker, &AutomationWorker::statusChanged,
            this, &AutomatorWidget::updateStatus);
    connect(m_worker, &AutomationWorker::finished,
            m_worker, &QObject::deleteLater);
    
    m_worker->start();
}

void AutomatorWidget::onAutomationFinished()
{
    m_startButton->setEnabled(true);
    m_startButton->setText("Запуск автоматизации");
    m_statusLabel->setText("Автоматизация завершена!");
    
    QMessageBox::information(this, "Готово", 
        "Автоматизация успешно выполнена!\n"
        "Скриншот сохранен как screenshot_qt.bmp");
}

void AutomatorWidget::updateStatus(const QString& message)
{
    m_statusLabel->setText(message);
    qDebug() << "Status:" << message;
}