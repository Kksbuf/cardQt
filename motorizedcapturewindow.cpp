#include "motorizedcapturewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QPaintEvent>
#include <QDebug>
#include <QThread>

MotorizedCaptureWindow::MotorizedCaptureWindow(QWidget *parent, const QString &surfacePath,
                                             int imagesInX, int imagesInY,
                                             const QVector<int> &sequence, bool isA4)
    : QDialog(parent)
    , surfacePath(surfacePath)
    , imagesInX(imagesInX)
    , imagesInY(imagesInY)
    , sequence(sequence)
    , currentCaptureIndex(0)
    , cameraConnected(false)
    , arduinoPort(nullptr)
    , isA4Size(isA4)
{
    setWindowTitle("Motorized Surface Capture");
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    resize(1280, 720);

    // Set window background color
    setStyleSheet("QDialog { background-color: rgb(240, 240, 240); }");
    
    // Create surface directory if it doesn't exist
    QDir().mkpath(surfacePath);
    
    setupUI();
    connectToCamera();
    connectToArduino();
}

MotorizedCaptureWindow::~MotorizedCaptureWindow()
{
    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
    }
    if (networkManager) {
        delete networkManager;
    }
    if (arduinoPort) {
        if (arduinoPort->isOpen()) {
            arduinoPort->close();
        }
        delete arduinoPort;
    }
}

void MotorizedCaptureWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Arduino port selection
    mainLayout->addLayout(setupArduinoSelection());

    // Image display
    imageLabel = new QLabel;
    imageLabel->setMinimumSize(1280, 720);
    imageLabel->setStyleSheet("QLabel { background-color: black; border-radius: 5px; }");
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mainLayout->addWidget(imageLabel);

    // Center status text
    QVBoxLayout *statusLayout = new QVBoxLayout;
    statusLayout->setSpacing(5);
    statusLayout->setContentsMargins(0, 5, 0, 5);

    // Sequence status
    QLabel *sequenceLabel = new QLabel;
    sequenceLabel->setAlignment(Qt::AlignCenter);
    sequenceLabel->setText("Arrow keys: continuous movement, WASD: step movement");
    sequenceLabel->setStyleSheet("QLabel { color: black; font-size: 13px; }");
    statusLayout->addWidget(sequenceLabel);

    // Capture count
    statusLabel = new QLabel;
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("QLabel { color: black; font-size: 13px; }");
    statusLayout->addWidget(statusLabel);

    mainLayout->addLayout(statusLayout);

    // Add motor controls
    mainLayout->addLayout(setupMotorControls());

    // Control buttons layout
    QHBoxLayout *controlLayout = new QHBoxLayout;
    controlLayout->setSpacing(10);

    // Home buttons
    homeXButton = new QPushButton("Home X");
    homeYButton = new QPushButton("Home Y");
    homeZButton = new QPushButton("Home Z");

    // Style home buttons
    QString homeButtonStyle = R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px 15px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
    )";

    homeXButton->setStyleSheet(homeButtonStyle);
    homeYButton->setStyleSheet(homeButtonStyle);
    homeZButton->setStyleSheet(homeButtonStyle);

    controlLayout->addWidget(homeXButton);
    controlLayout->addWidget(homeYButton);
    controlLayout->addWidget(homeZButton);
    controlLayout->addStretch();

    // Capture and finish buttons
    captureButton = new QPushButton("Capture (SPACE)");
    captureButton->setStyleSheet(R"(
        QPushButton {
            background-color: #007AFF;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px 20px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #0069DD;
        }
        QPushButton:pressed {
            background-color: #0058C4;
        }
    )");
    captureButton->setFixedHeight(35);
    captureButton->setMinimumWidth(200);

    finishButton = new QPushButton("Finish Capturing (Q)");
    finishButton->setStyleSheet(R"(
        QPushButton {
            background-color: white;
            color: black;
            border: 1px solid #CCCCCC;
            border-radius: 5px;
            padding: 8px 20px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #F5F5F5;
        }
        QPushButton:pressed {
            background-color: #E5E5E5;
        }
    )");
    finishButton->setFixedHeight(35);
    finishButton->setMinimumWidth(200);

    controlLayout->addWidget(captureButton);
    controlLayout->addWidget(finishButton);

    mainLayout->addLayout(controlLayout);

    // Connect button signals
    connect(captureButton, &QPushButton::clicked, this, &MotorizedCaptureWindow::captureImage);
    connect(finishButton, &QPushButton::clicked, this, &MotorizedCaptureWindow::finishCapturing);
    connect(homeXButton, &QPushButton::clicked, this, [this]() { homeAxis('X'); });
    connect(homeYButton, &QPushButton::clicked, this, [this]() { homeAxis('Y'); });
    connect(homeZButton, &QPushButton::clicked, this, [this]() { homeAxis('Z'); });

    updateStatusLabel();
}

QHBoxLayout* MotorizedCaptureWindow::setupArduinoSelection()
{
    QHBoxLayout *arduinoLayout = new QHBoxLayout();
    arduinoLayout->setSpacing(10);

    // Port selector label
    QLabel *portLabel = new QLabel("Arduino Port:");
    portLabel->setStyleSheet("QLabel { color: black; font-size: 13px; }");
    arduinoLayout->addWidget(portLabel);

    // Port selector combo box
    portSelector = new QComboBox();
    portSelector->setMinimumWidth(200);
    portSelector->setStyleSheet(R"(
        QComboBox {
            border: 1px solid #CCCCCC;
            border-radius: 5px;
            padding: 5px;
            background: white;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            image: url(down_arrow.png);
            width: 12px;
            height: 12px;
        }
    )");
    arduinoLayout->addWidget(portSelector);

    // Refresh button
    refreshPortButton = new QPushButton("Refresh");
    refreshPortButton->setStyleSheet(R"(
        QPushButton {
            background-color: #4CAF50;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 5px 15px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:pressed {
            background-color: #3d8b40;
        }
    )");
    arduinoLayout->addWidget(refreshPortButton);

    // Arduino status label
    arduinoStatusLabel = new QLabel("Not Connected");
    arduinoStatusLabel->setStyleSheet("QLabel { color: red; font-size: 13px; }");
    arduinoLayout->addWidget(arduinoStatusLabel);

    // Add stretch to push everything to the left
    arduinoLayout->addStretch();

    // Connect signals
    connect(portSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MotorizedCaptureWindow::onPortSelected);
    connect(refreshPortButton, &QPushButton::clicked,
            this, &MotorizedCaptureWindow::refreshPortList);

    // Initial port list population
    refreshPortList();

    return arduinoLayout;
}

void MotorizedCaptureWindow::refreshPortList()
{
    portSelector->clear();
    portSelector->addItem("Select Port...");

    // Get list of available ports
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &portInfo : ports) {
        QString description = portInfo.description();
        if (description.isEmpty()) {
            description = "Unknown Device";
        }
        QString portText = QString("%1 (%2)").arg(portInfo.portName()).arg(description);
        portSelector->addItem(portText, portInfo.portName());
    }
}

void MotorizedCaptureWindow::onPortSelected(int index)
{
    if (index <= 0) {
        arduinoStatusLabel->setText("No port selected");
        arduinoStatusLabel->setStyleSheet("QLabel { color: red; font-size: 13px; }");
        return;
    }

    // Close existing connection if any
    if (arduinoPort && arduinoPort->isOpen()) {
        arduinoPort->close();
        delete arduinoPort;
        arduinoPort = nullptr;
    }

    // Get the port name from the combo box data
    QString portName = portSelector->currentData().toString();
    
    // Create new serial port
    arduinoPort = new QSerialPort(this);

    // Configure port
    arduinoPort->setPortName(portName);
    arduinoPort->setBaudRate(QSerialPort::Baud9600);
    arduinoPort->setDataBits(QSerialPort::Data8);
    arduinoPort->setParity(QSerialPort::NoParity);
    arduinoPort->setStopBits(QSerialPort::OneStop);
    arduinoPort->setFlowControl(QSerialPort::NoFlowControl);

    // Try to open the port
    if (arduinoPort->open(QIODevice::ReadWrite)) {
        connect(arduinoPort, &QSerialPort::readyRead,
                this, &MotorizedCaptureWindow::handleSerialData);
        arduinoStatusLabel->setText("Connected - Initializing motors...");
        arduinoStatusLabel->setStyleSheet("QLabel { color: orange; font-size: 13px; }");
        qDebug() << "Connected to Arduino on" << portName;

        // Initialize motors after successful connection
        QTimer::singleShot(1000, this, &MotorizedCaptureWindow::initializeMotors);
    } else {
        QString errorMsg = QString("Connection Failed: %1").arg(arduinoPort->errorString());
        arduinoStatusLabel->setText(errorMsg);
        arduinoStatusLabel->setStyleSheet("QLabel { color: red; font-size: 13px; }");
        qDebug() << "Failed to open Arduino port:" << arduinoPort->errorString();
        delete arduinoPort;
        arduinoPort = nullptr;
    }
}

void MotorizedCaptureWindow::connectToCamera()
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MotorizedCaptureWindow::handleNetworkReply);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &MotorizedCaptureWindow::updateCameraFeed);
    updateTimer->start(100); // Update every 100ms
}

void MotorizedCaptureWindow::connectToArduino()
{
    // This is now handled by onPortSelected
    arduinoPort = nullptr; // Initialize to nullptr
}

void MotorizedCaptureWindow::updateCameraFeed()
{
    networkManager->get(QNetworkRequest(QUrl(cameraUrl)));
}

void MotorizedCaptureWindow::handleNetworkReply(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        QImage image = QImage::fromData(imageData);
        
        if (!image.isNull()) {
            lastFrame = image;
            cameraConnected = true;
            
            // Create a copy for display with reference box
            QImage displayImage = image.copy();
            QPainter painter(&displayImage);
            drawReferenceBox(painter);
            
            // Scale the image to fit the label while maintaining aspect ratio
            QPixmap pixmap = QPixmap::fromImage(displayImage);
            imageLabel->setPixmap(pixmap.scaled(imageLabel->size(), 
                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    } else {
        cameraConnected = false;
        imageLabel->setText("Camera connection failed. Please check camera and network settings.");
    }
    
    updateStatusLabel();
    reply->deleteLater();
}

void MotorizedCaptureWindow::drawReferenceBox(QPainter &painter)
{
    painter.setPen(QPen(Qt::green, 2));
    
    // Calculate center position
    int x = (1920 - referenceBoxWidth) / 2;
    int y = (1080 - referenceBoxHeight) / 2;
    
    // Draw rectangle
    painter.drawRect(x, y, referenceBoxWidth, referenceBoxHeight);
    
    // Draw center crosshair
    int centerX = x + referenceBoxWidth / 2;
    int centerY = y + referenceBoxHeight / 2;
    int crossSize = 10;
    
    painter.setPen(QPen(Qt::red, 2));
    painter.drawLine(centerX - crossSize, centerY, centerX + crossSize, centerY);
    painter.drawLine(centerX, centerY - crossSize, centerX, centerY + crossSize);
}

QHBoxLayout* MotorizedCaptureWindow::setupMotorControls()
{
    QHBoxLayout *motorLayout = new QHBoxLayout();
    motorLayout->setSpacing(10);

    QString buttonStyle = R"(
        QPushButton {
            background-color: #2196F3;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 8px 15px;
            font-size: 13px;
            min-width: 80px;
        }
        QPushButton:hover {
            background-color: #1976D2;
        }
        QPushButton:pressed {
            background-color: #0D47A1;
        }
    )";

    // X axis controls
    QVBoxLayout *xLayout = new QVBoxLayout();
    QLabel *xLabel = new QLabel("X Axis");
    xLabel->setAlignment(Qt::AlignCenter);
    xPlusStepButton = new QPushButton("Step +X (D)");
    xMinusStepButton = new QPushButton("Step -X (A)");
    xPlusStepButton->setStyleSheet(buttonStyle);
    xMinusStepButton->setStyleSheet(buttonStyle);
    xLayout->addWidget(xLabel);
    xLayout->addWidget(xPlusStepButton);
    xLayout->addWidget(xMinusStepButton);

    // Y axis controls
    QVBoxLayout *yLayout = new QVBoxLayout();
    QLabel *yLabel = new QLabel("Y Axis");
    yLabel->setAlignment(Qt::AlignCenter);
    yPlusStepButton = new QPushButton("Step +Y (W)");
    yMinusStepButton = new QPushButton("Step -Y (S)");
    yPlusStepButton->setStyleSheet(buttonStyle);
    yMinusStepButton->setStyleSheet(buttonStyle);
    yLayout->addWidget(yLabel);
    yLayout->addWidget(yPlusStepButton);
    yLayout->addWidget(yMinusStepButton);

    // Z axis controls
    QVBoxLayout *zLayout = new QVBoxLayout();
    QLabel *zLabel = new QLabel("Z Axis");
    zLabel->setAlignment(Qt::AlignCenter);
    zPlusStepButton = new QPushButton("Step +Z (E)");
    zMinusStepButton = new QPushButton("Step -Z (Q)");
    zPlusStepButton->setStyleSheet(buttonStyle);
    zMinusStepButton->setStyleSheet(buttonStyle);
    zLayout->addWidget(zLabel);
    zLayout->addWidget(zPlusStepButton);
    zLayout->addWidget(zMinusStepButton);

    // Add all axis controls to the main layout
    motorLayout->addLayout(xLayout);
    motorLayout->addLayout(yLayout);
    motorLayout->addLayout(zLayout);
    motorLayout->addStretch();

    // Connect step button signals
    connect(xPlusStepButton, &QPushButton::clicked, this, [this]() { moveSteps('X', true, stepSize); });
    connect(xMinusStepButton, &QPushButton::clicked, this, [this]() { moveSteps('X', false, stepSize); });
    connect(yPlusStepButton, &QPushButton::clicked, this, [this]() { moveSteps('Y', true, stepSize); });
    connect(yMinusStepButton, &QPushButton::clicked, this, [this]() { moveSteps('Y', false, stepSize); });
    connect(zPlusStepButton, &QPushButton::clicked, this, [this]() { moveSteps('Z', true, stepSize); });
    connect(zMinusStepButton, &QPushButton::clicked, this, [this]() { moveSteps('Z', false, stepSize); });

    return motorLayout;
}

void MotorizedCaptureWindow::moveMotor(char axis, bool direction)
{
    QString command;
    if (direction) {
        command = QString("MOVE %1").arg(axis);  // Uppercase for positive direction
    } else {
        command = QString("MOVE %1").arg(QChar(axis).toLower());  // Lowercase for negative direction
    }
    sendArduinoCommand(command);
    
    switch (axis) {
        case 'X': xMoving = true; break;
        case 'Y': yMoving = true; break;
        case 'Z': zMoving = true; break;
    }
}

void MotorizedCaptureWindow::moveSteps(char axis, bool direction, int steps)
{
    // Convert direction to steps (positive or negative)
    int actualSteps = direction ? steps : -steps;
    QString command = QString("STEP %1 %2").arg(axis).arg(actualSteps);
    sendArduinoCommand(command);
}

void MotorizedCaptureWindow::stopMotor(char axis)
{
    // For your Arduino code, stopping is handled by not sending movement commands
    // So we just update the state
    switch (axis) {
        case 'X': xMoving = false; break;
        case 'Y': yMoving = false; break;
        case 'Z': zMoving = false; break;
    }
}

void MotorizedCaptureWindow::homeAxis(char axis)
{
    QString command = QString("HOME %1").arg(axis);
    sendArduinoCommand(command);
}

void MotorizedCaptureWindow::sendArduinoCommand(const QString &command)
{
    if (arduinoPort && arduinoPort->isOpen()) {
        qDebug() << "Sending command:" << command;
        arduinoPort->write((command + "\n").toUtf8());
        arduinoPort->flush();
    } else {
        qDebug() << "Arduino not connected - command not sent:" << command;
    }
}

void MotorizedCaptureWindow::handleSerialData()
{
    if (!arduinoPort) return;
    
    QByteArray data = arduinoPort->readAll();
    QString message = QString::fromUtf8(data).trimmed();
    
    if (!message.isEmpty()) {
        qDebug() << "Arduino:" << message;
    }
}

void MotorizedCaptureWindow::captureImage()
{
    if (!cameraConnected || lastFrame.isNull()) {
        return;
    }

    lastSavedImagePath = QString("%1/image_%2.jpg")
        .arg(surfacePath)
        .arg(currentCaptureIndex + 1, 2, 10, QChar('0'));
    
    saveImage(lastFrame);
    capturedImages.append(lastSavedImagePath);
    
    // Emit signal for the newly captured image
    emit imageCaptured(lastSavedImagePath);
    
    currentCaptureIndex++;
    updateStatusLabel();

    if (currentCaptureIndex >= sequence.size()) {
        finishCapturing();
    } else {
        // Move to next position after successful capture
        QTimer::singleShot(500, this, &MotorizedCaptureWindow::moveToNextPosition);
    }
}

void MotorizedCaptureWindow::saveImage(const QImage &image)
{
    image.save(lastSavedImagePath, "JPG", 100);
}

QString MotorizedCaptureWindow::getCurrentCoordinates() const
{
    int currentNum = sequence[currentCaptureIndex];
    int row = (currentNum - 1) / imagesInX;
    int col = (currentNum - 1) % imagesInX;
    return QString("x%1y%2").arg(col + 1).arg(row + 1);
}

void MotorizedCaptureWindow::updateStatusLabel()
{
    QString status;
    if (!cameraConnected) {
        status = "Camera connection failed. Please check camera and network settings.";
    } else {
        status = QString("Capturing image %1/%2")
            .arg(currentCaptureIndex)
            .arg(sequence.size());
    }
    statusLabel->setText(status);
}

void MotorizedCaptureWindow::finishCapturing()
{
    saveSettings();
    accept();
}

void MotorizedCaptureWindow::saveSettings()
{
    QJsonObject settings;
    settings["imagesInX"] = imagesInX;
    settings["imagesInY"] = imagesInY;
    settings["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonArray sequenceArray;
    for (int num : sequence) {
        sequenceArray.append(num);
    }
    settings["sequence"] = sequenceArray;

    QJsonDocument doc(settings);
    QString filename = surfacePath + "/settings.json";
    QFile file(filename);
    
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}

void MotorizedCaptureWindow::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);
    
    if (!lastFrame.isNull()) {
        QPainter painter(this);
        drawReferenceBox(painter);
    }
} 

// Define step constants
const int X_STEP_DISTANCE = 265;  // Standard X-axis movement distance
const int Y_STEP_DISTANCE = 370;  // Standard Y-axis movement distance

// Define the movement sequence array
const struct MovementStep {
    char axis;
    bool direction;
    int steps;
} MOVEMENT_SEQUENCE[] = {
    {'X', true,  0},  // Home->1: Move right
    {'X', true,  X_STEP_DISTANCE},  // 1->2: Move right
    {'X', true,  X_STEP_DISTANCE},  // 2->3: Move right
    {'X', true,  X_STEP_DISTANCE},  // 3->4: Move right
    {'Y', true,  Y_STEP_DISTANCE},  // 4->5: Move down
    {'X', false, X_STEP_DISTANCE},  // 5->6: Move left
    {'X', false, X_STEP_DISTANCE},  // 6->7: Move left
    {'X', false, X_STEP_DISTANCE},  // 7->8: Move left
    {'Y', true,  Y_STEP_DISTANCE},  // 8->9: Move down
    {'X', true,  X_STEP_DISTANCE},  // 9->10: Move right
    {'X', true,  X_STEP_DISTANCE},  // 10->11: Move right
    {'X', true,  X_STEP_DISTANCE},  // 11->12: Move right
    {'Y', true,  Y_STEP_DISTANCE},  // 12->13: Move down
    {'X', false, X_STEP_DISTANCE},  // 13->14: Move left
    {'X', false, X_STEP_DISTANCE},  // 14->15: Move left
    {'X', false, X_STEP_DISTANCE}   // 15->16: Move left
};


void MotorizedCaptureWindow::initializeMotors()
{
    // Ensure we're connected
    if (!arduinoPort || !arduinoPort->isOpen()) {
        qDebug() << "Cannot initialize motors - Arduino not connected";
        return;
    }

    qDebug() << "Starting motor initialization sequence...";
    
    // Clear any pending commands
    arduinoPort->clear();
    
    // Home X axis
    qDebug() << "Homing X axis...";
    homeAxis('X');
    QThread::msleep(500);  // Longer wait for homing
    
    // Home Y axis
    qDebug() << "Homing Y axis...";
    homeAxis('Y');
    QThread::msleep(500);
    
    // Apply Y offset
    qDebug() << "Moving Y to offset position...";
    moveSteps('Y', false, yHomeOffset);  // Move Y -15 steps
    QThread::msleep(300);
    
    // Home Z axis
    qDebug() << "Homing Z axis...";
    homeAxis('Z');
    QThread::msleep(500);
    
    // Apply Z offset
    qDebug() << "Moving Z to offset position...";
    moveSteps('Z', true, zHomeOffset);   // Move Z +310 steps
    QThread::msleep(300);

    // Home X axis
    qDebug() << "Homing X axis...";
    homeAxis('X');
    QThread::msleep(500);  // Longer wait for homing
    
    // Reset position tracking
    currentX = 0;
    currentY = 0;
    
    // Update status
    arduinoStatusLabel->setText("Connected - Motors initialized");
    arduinoStatusLabel->setStyleSheet("QLabel { color: green; font-size: 13px; }");
    statusLabel->setText("Ready for first capture - Position 1");
    
    qDebug() << "Motor initialization sequence completed";
}

void MotorizedCaptureWindow::moveToNextPosition()
{
    if (currentCaptureIndex >= sequence.size()) {
        qDebug() << "All positions captured";
        return;  // All positions captured
    }
    
    // Get current and next sequence numbers
    int currentSeq = currentCaptureIndex;  // Current sequence position (0-based)
    int nextSeq = currentSeq + 1;  // Next sequence position
    
    qDebug() << "Moving from sequence position" << currentSeq + 1 << "to" << nextSeq + 1;
    qDebug() << "Moving from grid position" << sequence[currentSeq] << "to" << sequence[nextSeq];
    
    // Get the movement from the sequence array
    const MovementStep& step = MOVEMENT_SEQUENCE[currentSeq];  // Use 0-based index
    moveSteps(step.axis, step.direction, step.steps);
    
    // Update status
    statusLabel->setText(QString("Ready for capture %1 of %2 - Grid Position %3")
                        .arg(nextSeq + 1)
                        .arg(sequence.size())
                        .arg(sequence[nextSeq]));
}