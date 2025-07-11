#include "capturewindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDateTime>
#include <QPaintEvent>

CaptureWindow::CaptureWindow(QWidget *parent, const QString &surfacePath,
                           int imagesInX, int imagesInY,
                           const QVector<int> &sequence)
    : QDialog(parent)
    , surfacePath(surfacePath)
    , imagesInX(imagesInX)
    , imagesInY(imagesInY)
    , sequence(sequence)
    , currentCaptureIndex(0)
    , cameraConnected(false)
{
    setWindowTitle("Surface Capture");
    setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
    resize(1280, 720);

    // Set window background color
    setStyleSheet("QDialog { background-color: rgb(240, 240, 240); }");
    
    // Create surface directory if it doesn't exist
    QDir().mkpath(surfacePath);
    
    setupUI();
    connectToCamera();
}

CaptureWindow::~CaptureWindow()
{
    if (updateTimer) {
        updateTimer->stop();
        delete updateTimer;
    }
    if (networkManager) {
        delete networkManager;
    }
}

void CaptureWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

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
    sequenceLabel->setText("Press SPACE to capture, following the sequence");
    sequenceLabel->setStyleSheet("QLabel { color: black; font-size: 13px; }");
    statusLayout->addWidget(sequenceLabel);

    // Capture count
    statusLabel = new QLabel;
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setStyleSheet("QLabel { color: black; font-size: 13px; }");
    statusLayout->addWidget(statusLabel);

    mainLayout->addLayout(statusLayout);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 5, 0, 5);

    // Capture button
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

    // Finish button
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
    
    buttonLayout->addStretch();
    buttonLayout->addWidget(captureButton);
    buttonLayout->addWidget(finishButton);
    buttonLayout->addStretch();

    mainLayout->addLayout(buttonLayout);

    connect(captureButton, &QPushButton::clicked, this, &CaptureWindow::captureImage);
    connect(finishButton, &QPushButton::clicked, this, &CaptureWindow::finishCapturing);

    updateStatusLabel();
}

void CaptureWindow::connectToCamera()
{
    networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished,
            this, &CaptureWindow::handleNetworkReply);

    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &CaptureWindow::updateCameraFeed);
    updateTimer->start(100); // Update every 100ms
}

void CaptureWindow::updateCameraFeed()
{
    networkManager->get(QNetworkRequest(QUrl(cameraUrl)));
}

void CaptureWindow::handleNetworkReply(QNetworkReply *reply)
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

void CaptureWindow::drawReferenceBox(QPainter &painter)
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

void CaptureWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space) {
        captureImage();
    } else if (event->key() == Qt::Key_Q) {
        finishCapturing();
    } else {
        QDialog::keyPressEvent(event);
    }
}

void CaptureWindow::captureImage()
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
    }
}

void CaptureWindow::saveImage(const QImage &image)
{
    image.save(lastSavedImagePath, "JPG", 100);
}

QString CaptureWindow::getCurrentCoordinates() const
{
    int currentNum = sequence[currentCaptureIndex];
    int row = (currentNum - 1) / imagesInX;
    int col = (currentNum - 1) % imagesInX;
    return QString("x%1y%2").arg(col + 1).arg(row + 1);
}

void CaptureWindow::updateStatusLabel()
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

void CaptureWindow::finishCapturing()
{
    saveSettings();
    accept();
}

void CaptureWindow::saveSettings()
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

void CaptureWindow::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);
    
    if (!lastFrame.isNull()) {
        QPainter painter(this);
        drawReferenceBox(painter);
    }
} 