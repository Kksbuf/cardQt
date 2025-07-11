#include "welcomewindow.h"
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <QApplication>
#include <QStyle>

WelcomeWindow::WelcomeWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
}

void WelcomeWindow::setupUI()
{
    setWindowTitle("Plywood Surface Analysis");
    resize(800, 600);

    // Create central widget and main layout
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setAlignment(Qt::AlignCenter);
    mainLayout->setSpacing(20);

    // Add title label
    QLabel *titleLabel = new QLabel("Welcome to Plywood Surface Analysis", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Add description label
    QLabel *descLabel = new QLabel("This application helps you analyze plywood surfaces for defects.\nChoose an option below to begin:", this);
    descLabel->setAlignment(Qt::AlignCenter);
    QFont descFont = descLabel->font();
    descFont.setPointSize(12);
    descLabel->setFont(descFont);
    mainLayout->addWidget(descLabel);

    // Add some spacing
    mainLayout->addSpacing(20);

    // Create buttons
    startNewSessionButton = new QPushButton("Start New Session", this);
    openExistingSessionButton = new QPushButton("Open Existing Session", this);

    // Style buttons
    QFont buttonFont;
    buttonFont.setPointSize(12);
    startNewSessionButton->setFont(buttonFont);
    openExistingSessionButton->setFont(buttonFont);

    startNewSessionButton->setMinimumSize(300, 50);
    openExistingSessionButton->setMinimumSize(300, 50);

    startNewSessionButton->setStyleSheet("QPushButton { background-color: #f0f0f0; border: 1px solid #cccccc; border-radius: 4px; }");
    openExistingSessionButton->setStyleSheet("QPushButton { background-color: #f0f0f0; border: 1px solid #cccccc; border-radius: 4px; }");

    // Add buttons to layout
    mainLayout->addWidget(startNewSessionButton, 0, Qt::AlignCenter);
    mainLayout->addWidget(openExistingSessionButton, 0, Qt::AlignCenter);

    // Connect button signals
    connect(startNewSessionButton, &QPushButton::clicked, this, &WelcomeWindow::onStartNewSession);
    connect(openExistingSessionButton, &QPushButton::clicked, this, &WelcomeWindow::onOpenExistingSession);
}

void WelcomeWindow::onStartNewSession()
{
    QString sessionPath = createSessionFolder();
    if (!sessionPath.isEmpty()) {
        MainWindow *mainWindow = new MainWindow(nullptr, sessionPath);
        mainWindow->show();
        this->close();
    }
}

void WelcomeWindow::onOpenExistingSession()
{
    QString sessionDir = QFileDialog::getExistingDirectory(this,
        "Select Session Directory",
        QDir::currentPath() + "/sessions",
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!sessionDir.isEmpty()) {
        MainWindow *mainWindow = new MainWindow(nullptr, sessionDir);
        mainWindow->show();
        this->close();
    }
}

QString WelcomeWindow::getCurrentDateTime()
{
    return QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
}

QString WelcomeWindow::createSessionFolder()
{
    // Create a sessions directory in the current workspace
    QString baseDir = QDir::currentPath() + "/sessions";
    QDir().mkpath(baseDir);

    // Create a new session directory with timestamp
    QString sessionName = "session_" + getCurrentDateTime();
    QString sessionPath = baseDir + "/" + sessionName;
    
    if (QDir().mkpath(sessionPath)) {
        return sessionPath;
    } else {
        QMessageBox::critical(this, "Error", "Failed to create session directory");
        return QString();
    }
} 