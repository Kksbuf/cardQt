#include "mainwindow.h"
#include "dimensionsdialog.h"
#include "capturesettingsdialog.h"
#include "capturewindow.h"
#include "imagestitcher.h"
#include "cuttingconfigdialog.h"
#include "cuttingwindow.h"
#include <QLabel>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QFrame>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTreeWidgetItem>
#include <QDir>
#include <QPixmap>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDebug>
#include <QTimer>
#include <QFileSystemWatcher>
#include <functional>

MainWindow::MainWindow(QWidget *parent, const QString &sessionPath)
    : QMainWindow(parent), sessionPath(sessionPath), defectDetector(nullptr), currentStitcher(nullptr)
{
    loadDimensions();
    setupUI();
    loadSurfaces();
    initializeDefectDetector();
}

MainWindow::~MainWindow()
{
    saveDimensions();
}

void MainWindow::setupUI()
{
    setWindowTitle("Plywood Surface Analysis");
    resize(1400, 800);

    // Create central widget and main layout
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Setup top bar
    setupTopBar();
    
    // Setup main area
    setupMainArea();

    // Setup debug area
    setupDebugArea();
}

void MainWindow::setupTopBar()
{
    QHBoxLayout *topBarLayout = new QHBoxLayout();
    
    // Session info and dimensions
    QString sessionName = sessionPath.split("/").last();
    QString dimensionsText = QString("Active session: %1\nSurface: %2 x %3 mm | Captured: %4 x %5 mm")
        .arg(sessionName)
        .arg(dimensions.actualWidth, 0, 'f', 1)
        .arg(dimensions.actualHeight, 0, 'f', 1)
        .arg(dimensions.capturedWidth, 0, 'f', 1)
        .arg(dimensions.capturedHeight, 0, 'f', 1);
    
    dimensionsLabel = new QLabel(dimensionsText);
    topBarLayout->addWidget(dimensionsLabel);

    // Add stretch to push cut button to the right
    topBarLayout->addStretch();

    // Cut Surface button at top right
    cutSurfaceButton = new QPushButton("Cut Surface");
    cutSurfaceButton->setMinimumWidth(100);
    topBarLayout->addWidget(cutSurfaceButton);

    // Add top bar to main layout
    qobject_cast<QVBoxLayout *>(centralWidget->layout())->addLayout(topBarLayout);

    // Connect cut button
    connect(cutSurfaceButton, &QPushButton::clicked, this, &MainWindow::onCutSurface);
}

void MainWindow::setupMainArea()
{
    QHBoxLayout *mainAreaLayout = new QHBoxLayout();
    
    // Left column - Surface list
    QVBoxLayout *leftColumnLayout = new QVBoxLayout();
    
    // Surface list buttons
    QHBoxLayout *surfaceButtonsLayout = new QHBoxLayout();
    addSurfaceButton = new QPushButton("Add Surface");
    deleteSurfaceButton = new QPushButton("Delete Surface");
    deleteSurfaceButton->setEnabled(false); // Initially disabled
    surfaceButtonsLayout->addWidget(addSurfaceButton);
    surfaceButtonsLayout->addWidget(deleteSurfaceButton);
    leftColumnLayout->addLayout(surfaceButtonsLayout);
    
    // Surface tree
    surfaceTree = new QTreeWidget();
    surfaceTree->setHeaderLabels({"Name", "Status", "Defects"});
    surfaceTree->setColumnWidth(0, 200); // Name column
    surfaceTree->setColumnWidth(1, 120); // Status column
    surfaceTree->setColumnWidth(2, 60);  // Defects count column
    surfaceTree->setSelectionMode(QAbstractItemView::SingleSelection);
    surfaceTree->setStyleSheet(R"(
        QTreeWidget {
            border: 1px solid #CCCCCC;
            border-radius: 5px;
            background-color: white;
        }
        QTreeWidget::item {
            height: 25px;
        }
        QTreeWidget::item:selected {
            background-color: #007AFF;
            color: white;
        }
    )");
    leftColumnLayout->addWidget(surfaceTree);
    
    QWidget *leftColumn = new QWidget();
    leftColumn->setLayout(leftColumnLayout);
    leftColumn->setFixedWidth(400);
    mainAreaLayout->addWidget(leftColumn);
    
    // Center column - Images
    QVBoxLayout *centerColumnLayout = new QVBoxLayout();
    
    // Labels for the images
    QLabel *originalLabel = new QLabel("Original Image");
    QLabel *defectLabel = new QLabel("Detected Defects");
    originalLabel->setAlignment(Qt::AlignCenter);
    defectLabel->setAlignment(Qt::AlignCenter);
    
    // Original image
    originalImageLabel = new QLabel();
    originalImageLabel->setMinimumSize(500, 350);
    originalImageLabel->setStyleSheet("QLabel { background-color: black; color: white; border: 1px solid #cccccc; border-radius: 5px; }");
    originalImageLabel->setAlignment(Qt::AlignCenter);
    originalImageLabel->setText("Select an image to preview");
    
    // Defect image
    defectImageLabel = new QLabel();
    defectImageLabel->setMinimumSize(500, 350);
    defectImageLabel->setStyleSheet("QLabel { background-color: black; color: white; border: 1px solid #cccccc; border-radius: 5px; }");
    defectImageLabel->setAlignment(Qt::AlignCenter);
    defectImageLabel->setText("No defects detected yet");
    
    centerColumnLayout->addWidget(originalLabel);
    centerColumnLayout->addWidget(originalImageLabel);
    centerColumnLayout->addWidget(defectLabel);
    centerColumnLayout->addWidget(defectImageLabel);
    
    QWidget *centerColumn = new QWidget();
    centerColumn->setLayout(centerColumnLayout);
    mainAreaLayout->addWidget(centerColumn);
    
    // Right column - Defect details
    QVBoxLayout *rightColumnLayout = new QVBoxLayout();
    
    QLabel *defectListLabel = new QLabel("Defect Details");
    defectListLabel->setAlignment(Qt::AlignCenter);
    rightColumnLayout->addWidget(defectListLabel);
    
    // Defect table
    defectTable = new QTableWidget();
    defectTable->setColumnCount(5);  // Update column count
    defectTable->setHorizontalHeaderLabels({"Number", "Type", "Confidence", "Location", "Size"});  // Remove Area header
    defectTable->verticalHeader()->setVisible(false);
    defectTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    
    // Make columns resizable and set initial widths
    defectTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    defectTable->setColumnWidth(0, 50);  // Number
    defectTable->setColumnWidth(1, 70);  // Type
    defectTable->setColumnWidth(2, 70);  // Confidence
    defectTable->setColumnWidth(3, 100); // Location
    defectTable->setColumnWidth(4, 100);  // Size
    
    defectTable->setStyleSheet(R"(
        QTableWidget {
            border: 1px solid #CCCCCC;
            border-radius: 5px;
            background-color: white;
        }
        QHeaderView::section {
            background-color: #F5F5F5;
            padding: 5px;
            border: none;
            border-bottom: 1px solid #CCCCCC;
        }
    )");
    rightColumnLayout->addWidget(defectTable);
    
    QWidget *rightColumn = new QWidget();
    rightColumn->setLayout(rightColumnLayout);
    rightColumn->setFixedWidth(400);
    mainAreaLayout->addWidget(rightColumn);

    // Add to main layout
    qobject_cast<QVBoxLayout *>(centralWidget->layout())->addLayout(mainAreaLayout);

    // Connect signals
    connect(addSurfaceButton, &QPushButton::clicked, this, &MainWindow::onAddNewSurface);
    connect(deleteSurfaceButton, &QPushButton::clicked, this, &MainWindow::onDeleteSurface);
    connect(surfaceTree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onItemSelectionChanged);
    connect(surfaceTree, &QTreeWidget::itemExpanded, this, &MainWindow::onItemExpanded);
    connect(surfaceTree, &QTreeWidget::itemCollapsed, this, &MainWindow::onItemCollapsed);

   
}

void MainWindow::setupDebugArea()
{
    QGroupBox *debugGroup = new QGroupBox("Debug Output", centralWidget);
    QVBoxLayout *debugLayout = new QVBoxLayout(debugGroup);

    debugOutput = new QTextEdit();
    debugOutput->setReadOnly(true);
    debugOutput->setMinimumHeight(100);
    debugOutput->setMaximumHeight(200);
    debugOutput->setStyleSheet("QTextEdit { background-color: #f0f0f0; font-family: monospace; }");
    
    debugLayout->addWidget(debugOutput);
    
    // Add to main layout
    qobject_cast<QVBoxLayout *>(centralWidget->layout())->addWidget(debugGroup);
}

void MainWindow::initializeDefectDetector()
{
    defectDetector = new DefectDetector(this);
    
    // Connect signals
    connect(defectDetector, &DefectDetector::statusMessage,
            this, &MainWindow::onModelStatusMessage);
    connect(defectDetector, &DefectDetector::modelInitializationComplete,
            this, &MainWindow::onModelInitComplete);
    connect(defectDetector, &DefectDetector::modelInitializationFailed,
            this, &MainWindow::onModelInitFailed);
    connect(defectDetector, &DefectDetector::detectionComplete,
            this, &MainWindow::onDetectionComplete);
    
    // Start initialization
    defectDetector->initializeDetectionProcess();
}

void MainWindow::onModelStatusMessage(const QString &message)
{
    debugOutput->append(message);
    qDebug() << "Received message:" << message;
    
    // Split message into lines to handle multiple messages
    QStringList lines = message.split("\n");
    for (const QString &line : lines) {
        // Check if this is a processing status message
        if (line.contains("[STATUS] Processing image:")) {
            QString imagePath = line.mid(line.indexOf(":") + 2).trimmed();
            qDebug() << "Processing status - Found image path:" << imagePath;
            updateImageStatus(imagePath, "Processing", -1);
        }
        // Check if this is a detection complete message
        else if (line.contains("[SUCCESS] Detection results saved")) {
            qDebug() << "Detection success message received:" << line;
            
            // Extract image name from "Detection results saved for image_XX.jpg"
            QString imageName;
            if (line.contains("for ")) {
                imageName = line.mid(line.indexOf("for ") + 4);
                imageName.remove(".jpg");
                qDebug() << "Extracted image name:" << imageName;
                
                // Get the current surface directory
                QString surfacePath;
                for (int i = 0; i < surfaceTree->topLevelItemCount(); ++i) {
                    QTreeWidgetItem* surfaceItem = surfaceTree->topLevelItem(i);
                    qDebug() << "Checking surface:" << surfaceItem->text(0) << "with status:" << surfaceItem->text(1);
                    if (surfaceItem->text(1) == "Processing" || surfaceItem->text(1) == "Pending") {
                        surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaceItem->text(0));
                        qDebug() << "Found active surface path:" << surfacePath;
                        break;
                    }
                }
                
                if (!surfacePath.isEmpty()) {
                    QString imagePath = QString("%1/%2.jpg").arg(surfacePath).arg(imageName);
                    QString detectionFile = QString("%1/%2_detections.json").arg(surfacePath).arg(imageName);
                    qDebug() << "Looking for detection file:" << detectionFile;
                    
                    // Read detection file to get defect count
                    if (QFile::exists(detectionFile)) {
                        qDebug() << "Detection file exists";
                        QFile file(detectionFile);
                        if (file.open(QIODevice::ReadOnly)) {
                            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                            QJsonObject obj = doc.object();
                            int defectCount = obj["detections"].toArray().size();
                            file.close();
                            
                            qDebug() << "Detection complete - Path:" << imagePath;
                            qDebug() << "Found defects:" << defectCount;
                            updateImageStatus(imagePath, "Analyzed", defectCount);

                            // Check if this is the currently selected image
                            QTreeWidgetItem* currentItem = surfaceTree->currentItem();
                            if (currentItem && !isSurfaceItem(currentItem)) {
                                QString currentImagePath = QString("%1/%2").arg(surfacePath).arg(currentItem->text(0));
                                if (currentImagePath == imagePath) {
                                    // Update the defect image preview
                                    QString detectedImagePath = imagePath;
                                    detectedImagePath.replace(".jpg", "_detected.jpg");
                                    QPixmap detectedPixmap(detectedImagePath);
                                    if (!detectedPixmap.isNull()) {
                                        QSize labelSize = defectImageLabel->size();
                                        QPixmap scaledPixmap = detectedPixmap.scaled(labelSize, 
                                            Qt::KeepAspectRatio, 
                                            Qt::FastTransformation);
                                        defectImageLabel->setPixmap(scaledPixmap);
                                    }

                                    // Update defect details table
                                    if (file.open(QIODevice::ReadOnly)) {
                                        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                                        QJsonObject obj = doc.object();
                                        QJsonArray detections = obj["detections"].toArray();

                                        defectTable->setRowCount(detections.size());
                                        for (int i = 0; i < detections.size(); ++i) {
                                            QJsonObject detection = detections[i].toObject();
                                            
                                            // Number
                                            QTableWidgetItem *numberItem = new QTableWidgetItem(QString::number(i + 1));
                                            numberItem->setTextAlignment(Qt::AlignCenter);
                                            defectTable->setItem(i, 0, numberItem);

                                            // Type
                                            QTableWidgetItem *typeItem = new QTableWidgetItem(detection["class_name"].toString());
                                            typeItem->setTextAlignment(Qt::AlignCenter);
                                            defectTable->setItem(i, 1, typeItem);

                                            // Confidence
                                            double confidence = detection["confidence"].toDouble();
                                            // If confidence is > 1, assume it's already in percentage
                                            if (confidence > 1) {
                                                confidence = confidence / 100.0;
                                            }
                                            QTableWidgetItem *confItem = new QTableWidgetItem(
                                                QString("%1%").arg(confidence * 100, 0, 'f', 1));
                                            confItem->setTextAlignment(Qt::AlignCenter);
                                            defectTable->setItem(i, 2, confItem);

                                            // Location
                                            QString location = QString("(%1, %2) mm")
                                                .arg(detection["center_x"].toInt())
                                                .arg(detection["center_y"].toInt());
                                            QTableWidgetItem *locItem = new QTableWidgetItem(location);
                                            locItem->setTextAlignment(Qt::AlignCenter);
                                            defectTable->setItem(i, 3, locItem);

                                            // Size
                                            QString size = QString("%1 × %2 mm")
                                                .arg(detection["width"].toInt())
                                                .arg(detection["height"].toInt());
                                            QTableWidgetItem *sizeItem = new QTableWidgetItem(size);
                                            sizeItem->setTextAlignment(Qt::AlignCenter);
                                            defectTable->setItem(i, 4, sizeItem);
                                        }
                                        file.close();
                                    }
                                }
                            }
                        } else {
                            qDebug() << "Failed to open detection file";
                        }
                    } else {
                        qDebug() << "Detection file does not exist";
                    }
                } else {
                    qDebug() << "No active surface found";
                }
            } else {
                qDebug() << "Failed to extract image name from message";
            }
        }
    }
}

void MainWindow::onModelInitComplete()
{
    debugOutput->append("<font color='green'><b>Model initialization completed successfully!</b></font>");
    
    // Process any pending images
    for (int i = 0; i < surfaceTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* surfaceItem = surfaceTree->topLevelItem(i);
        for (int j = 0; j < surfaceItem->childCount(); ++j) {
            QTreeWidgetItem* imageItem = surfaceItem->child(j);
            if (imageItem->text(1) == "Pending") {
                QString surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaceItem->text(0));
                QString imagePath = QString("%1/%2").arg(surfacePath).arg(imageItem->text(0));
                
                // Queue the image for detection with a small delay
                QTimer::singleShot(100 * (j + 1), this, [this, imagePath]() {
                    defectDetector->detectImage(imagePath);
                });
            }
        }
    }
}

void MainWindow::onModelInitFailed(const QString &error)
{
    debugOutput->append("<font color='red'><b>Model initialization failed: " + error + "</b></font>");
}

void MainWindow::onDetectionComplete(const QStringList &results)
{
    // Update preview if the current selected item is the processed image
    QString imagePath;
    for (const QString &line : results) {
        if (line.contains("[STATUS] Processing image:")) {
            imagePath = line.mid(line.indexOf(":") + 2).trimmed();
            break;
        }
    }
    
    if (!imagePath.isEmpty()) {
        QTreeWidgetItem* imageItem = findImageItem(imagePath);
        if (imageItem && imageItem == surfaceTree->currentItem()) {
            QString detectedImagePath = imagePath;
            detectedImagePath.replace(".jpg", "_detected.jpg");
            QPixmap pixmap(detectedImagePath);
            if (!pixmap.isNull()) {
                defectImageLabel->setPixmap(pixmap.scaled(defectImageLabel->size(),
                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    }
}

QTreeWidgetItem *MainWindow::findImageItem(const QString &imagePath)
{
    QFileInfo fileInfo(imagePath);
    QString fileName = fileInfo.fileName();
    QString surfaceName = fileInfo.dir().dirName();

    // Find surface item
    QTreeWidgetItem *surfaceItem = nullptr;
    for (int i = 0; i < surfaceTree->topLevelItemCount(); ++i)
    {
        if (surfaceTree->topLevelItem(i)->text(0) == surfaceName)
        {
            surfaceItem = surfaceTree->topLevelItem(i);
            break;
        }
    }

    if (!surfaceItem)
        return nullptr;

    // Find image item
    for (int i = 0; i < surfaceItem->childCount(); ++i)
    {
        if (surfaceItem->child(i)->text(0) == fileName)
        {
            return surfaceItem->child(i);
        }
    }

    return nullptr;
}

void MainWindow::updateImageStatus(const QString &imagePath, const QString &status, int defectCount)
{
    QTreeWidgetItem *imageItem = findImageItem(imagePath);
    if (!imageItem) {
        qDebug() << "Failed to find image item for path:" << imagePath;
        return;
    }

    qDebug() << "Updating status for" << imagePath;
    qDebug() << "Current status:" << imageItem->text(1) << "New status:" << status;
    qDebug() << "Current defects:" << imageItem->text(2) << "New defects:" << defectCount;

    imageItem->setText(1, status);

    if (defectCount >= 0) {
        imageItem->setText(2, QString::number(defectCount));

        // Update parent surface's total defect count
        QTreeWidgetItem *surfaceItem = imageItem->parent();
        if (surfaceItem) {
            int totalDefects = 0;
            int analyzedCount = 0;
            int totalExpectedImages = currentCaptureSettings.imagesInX * currentCaptureSettings.imagesInY;

            for (int i = 0; i < surfaceItem->childCount(); ++i) {
                QTreeWidgetItem *child = surfaceItem->child(i);
                if (child->text(1) == "Analyzed") {
                    analyzedCount++;
                    totalDefects += child->text(2).toInt();
                }
            }

            qDebug() << "Surface status update:";
            qDebug() << "Analyzed count:" << analyzedCount;
            qDebug() << "Total expected images:" << totalExpectedImages;
            qDebug() << "Total defects:" << totalDefects;

            surfaceItem->setText(2, QString::number(totalDefects));

            // Update surface status based on capture completion
            QString newStatus;
            if (analyzedCount == 0) {
                newStatus = "Pending";
            } else if (analyzedCount < totalExpectedImages) {
                newStatus = "Processing";
            } else if (analyzedCount == totalExpectedImages) {
                newStatus = "Analyzed";
                
                // If surface is now fully analyzed, create labeled stitched image
                QString surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaceItem->text(0));
                if (QFile::exists(surfacePath + "/stitched.jpg")) {
                    // Create new stitcher for labeling
                    ImageStitcher *labelStitcher = new ImageStitcher(surfacePath,
                        currentCaptureSettings.imagesInX,
                        currentCaptureSettings.imagesInY,
                        currentCaptureSettings.sequence,
                        dimensions.actualWidth,
                        dimensions.actualHeight);
                    
                    // Connect to finished signal to update UI if needed
                    connect(labelStitcher, &ImageStitcher::finished, this, [this, surfaceItem, surfacePath]() {
                        qDebug() << "Labeling complete, updating UI...";
                        QString labeledPath = surfacePath + "/stitched_labeled.jpg";
                        QString coordPath = surfacePath + "/defect_coordinates.json";
                        
                        // Add a small delay to ensure files are fully written
                        QTimer::singleShot(500, this, [this, surfaceItem, labeledPath, coordPath]() {
                            if (QFile::exists(labeledPath)) {
                                // Update defect image preview if this surface is selected
                                if (surfaceItem == surfaceTree->currentItem()) {
                                    QPixmap labeledPixmap(labeledPath);
                                    if (!labeledPixmap.isNull()) {
                                        defectImageLabel->setPixmap(labeledPixmap.scaled(defectImageLabel->size(),
                                            Qt::KeepAspectRatio, Qt::SmoothTransformation));
                                        
                                        // Update defect details from JSON
                                        QFile coordFile(coordPath);
                                        if (coordFile.open(QIODevice::ReadOnly)) {
                                            QJsonDocument doc = QJsonDocument::fromJson(coordFile.readAll());
                                            QJsonObject mainObj = doc.object();
                                            QJsonArray defects = mainObj["defects"].toArray();
                                            
                                            defectTable->setRowCount(defects.size());
                                            for (int i = 0; i < defects.size(); ++i) {
                                                QJsonObject defect = defects[i].toObject();
                                                
                                                // Number
                                                QTableWidgetItem *numberItem = new QTableWidgetItem(QString::number(i + 1));
                                                numberItem->setTextAlignment(Qt::AlignCenter);
                                                defectTable->setItem(i, 0, numberItem);

                                                // Type
                                                QTableWidgetItem *typeItem = new QTableWidgetItem(defect["type"].toString());
                                                typeItem->setTextAlignment(Qt::AlignCenter);
                                                defectTable->setItem(i, 1, typeItem);

                                                // Confidence
                                                double confidence = defect["confidence"].toDouble();
                                                if (confidence > 1) {
                                                    confidence = confidence / 100.0;
                                                }
                                                QTableWidgetItem *confItem = new QTableWidgetItem(
                                                    QString("%1%").arg(confidence * 100, 0, 'f', 1));
                                                confItem->setTextAlignment(Qt::AlignCenter);
                                                defectTable->setItem(i, 2, confItem);

                                                // For surface groups, we have physical_position object
                                                QJsonObject physicalPos = defect["physical_position"].toObject();
                                                
                                                // Location (x, y) in mm
                                                QString location = QString("(%1, %2) mm")
                                                    .arg(physicalPos["x"].toDouble(), 0, 'f', 1)
                                                    .arg(physicalPos["y"].toDouble(), 0, 'f', 1);
                                                QTableWidgetItem *locItem = new QTableWidgetItem(location);
                                                locItem->setTextAlignment(Qt::AlignCenter);
                                                defectTable->setItem(i, 3, locItem);

                                                // Size (width × height) in mm
                                                QString size_str = QString("%1 × %2 mm")
                                                    .arg(physicalPos["width"].toDouble(), 0, 'f', 1)
                                                    .arg(physicalPos["height"].toDouble(), 0, 'f', 1);
                                                QTableWidgetItem *sizeItem = new QTableWidgetItem(size_str);
                                                sizeItem->setTextAlignment(Qt::AlignCenter);
                                                defectTable->setItem(i, 4, sizeItem);
                                            }
                                            coordFile.close();
                                        }
                                    }
                                }
                            }
                        });
                    });
                    
                    // Label defects and clean up
                    labelStitcher->labelDefects();
                    delete labelStitcher;
                }
            }

            qDebug() << "Surface status changing from" << surfaceItem->text(1) << "to" << newStatus;
            surfaceItem->setText(1, newStatus);
        }
    }
}

void MainWindow::saveDimensions()
{
    QJsonObject json;
    json["actualWidth"] = dimensions.actualWidth;
    json["actualHeight"] = dimensions.actualHeight;
    json["capturedWidth"] = dimensions.capturedWidth;
    json["capturedHeight"] = dimensions.capturedHeight;

    QJsonDocument doc(json);
    QFile file(sessionPath + "/dimensions.json");
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::loadDimensions()
{
    // Set default values
    dimensions = {420.0, 297.0, 140.0, 99.0};

    // Try to load saved values
    QFile file(sessionPath + "/dimensions.json");
    if (file.open(QIODevice::ReadOnly))
    {
        QByteArray data = file.readAll();
        QJsonDocument doc(QJsonDocument::fromJson(data));
        QJsonObject json = doc.object();
        
        if (!json.isEmpty())
        {
            dimensions.actualWidth = json["actualWidth"].toDouble();
            dimensions.actualHeight = json["actualHeight"].toDouble();
            dimensions.capturedWidth = json["capturedWidth"].toDouble();
            dimensions.capturedHeight = json["capturedHeight"].toDouble();
        }
        file.close();
    }
    else
    {
        // Show dimensions dialog for new sessions
        DimensionsDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted)
        {
            dimensions.actualWidth = dialog.getActualWidth();
            dimensions.actualHeight = dialog.getActualHeight();
            dimensions.capturedWidth = dialog.getCapturedWidth();
            dimensions.capturedHeight = dialog.getCapturedHeight();
            saveDimensions();
        }
    }
}

void MainWindow::onAddNewSurface()
{
    CaptureSettingsDialog settingsDialog(this);
    if (settingsDialog.exec() == QDialog::Accepted)
    {
        // Store the capture settings
        currentCaptureSettings.imagesInX = settingsDialog.getImagesInX();
        currentCaptureSettings.imagesInY = settingsDialog.getImagesInY();
        currentCaptureSettings.sequence = settingsDialog.getCaptureSequence();

        // Create a new surface directory
        int surfaceNumber = surfaceTree->topLevelItemCount() + 1;
        QString surfaceName = QString("surface_%1").arg(surfaceNumber, 2, 10, QChar('0'));
        QString surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaceName);

        // Create the directory if it doesn't exist
        QDir dir;
        if (!dir.exists(surfacePath)) {
            dir.mkpath(surfacePath);
        }

        // Create and add the surface item before starting capture
        QTreeWidgetItem *surfaceItem = new QTreeWidgetItem(surfaceTree);
        surfaceItem->setText(0, surfaceName);
        surfaceItem->setText(1, "Pending");
        surfaceItem->setText(2, "0");
        surfaceTree->setCurrentItem(surfaceItem);
        surfaceItem->setExpanded(true);

        // Set up file watcher for the new surface directory
        if (defectWatcher) {
            disconnect(defectWatcher, nullptr, this, nullptr);
            delete defectWatcher;
        }
        defectWatcher = new QFileSystemWatcher(this);
        
        // Add the directory to watch
        if (defectWatcher->addPath(surfacePath)) {
            qDebug() << "Started watching directory:" << surfacePath;
        } else {
            qDebug() << "Failed to watch directory:" << surfacePath;
        }

        // Connect the file watcher with debounce timer
        connect(defectWatcher, &QFileSystemWatcher::directoryChanged, this, 
            [this, surfacePath]() {
                static QTimer *debounceTimer = nullptr;
                if (debounceTimer) {
                    debounceTimer->stop();
                    delete debounceTimer;
                }
                debounceTimer = new QTimer(this);
                debounceTimer->setSingleShot(true);
                debounceTimer->setInterval(500);  // 500ms delay
                
                // Create a new updateDefectView lambda for this timer
                auto updateDefectView = [this, surfacePath]() {
                    QString labeledImagePath = QString("%1/stitched_labeled.jpg").arg(surfacePath);
                    QString coordPath = QString("%1/defect_coordinates.json").arg(surfacePath);
                    
                    // Update UI if this surface is currently selected
                    QTreeWidgetItem *currentItem = surfaceTree->currentItem();
                    if (currentItem && isSurfaceItem(currentItem) && 
                        QString("%1/%2").arg(sessionPath).arg(currentItem->text(0)) == surfacePath) {
                        
                        if (QFile::exists(labeledImagePath)) {
                            QPixmap labeledPixmap(labeledImagePath);
                            if (!labeledPixmap.isNull()) {
                                defectImageLabel->setPixmap(labeledPixmap.scaled(defectImageLabel->size(),
                                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
                                
                                // Update defect details from JSON
                                QFile coordFile(coordPath);
                                if (coordFile.open(QIODevice::ReadOnly)) {
                                    QJsonDocument doc = QJsonDocument::fromJson(coordFile.readAll());
                                    QJsonObject mainObj = doc.object();
                                    QJsonArray defects = mainObj["defects"].toArray();
                                    
                                    defectTable->setRowCount(defects.size());
                                    for (int i = 0; i < defects.size(); ++i) {
                                        QJsonObject defect = defects[i].toObject();
                                        
                                        // Number
                                        QTableWidgetItem *numberItem = new QTableWidgetItem(QString::number(i + 1));
                                        numberItem->setTextAlignment(Qt::AlignCenter);
                                        defectTable->setItem(i, 0, numberItem);

                                        // Type
                                        QTableWidgetItem *typeItem = new QTableWidgetItem(defect["type"].toString());
                                        typeItem->setTextAlignment(Qt::AlignCenter);
                                        defectTable->setItem(i, 1, typeItem);

                                        // Confidence
                                        double confidence = defect["confidence"].toDouble();
                                        if (confidence > 1) {
                                            confidence = confidence / 100.0;
                                        }
                                        QTableWidgetItem *confItem = new QTableWidgetItem(
                                            QString("%1%").arg(confidence * 100, 0, 'f', 1));
                                        confItem->setTextAlignment(Qt::AlignCenter);
                                        defectTable->setItem(i, 2, confItem);

                                        // For surface groups, we have physical_position object
                                        QJsonObject physicalPos = defect["physical_position"].toObject();
                                        
                                        // Location (x, y) in mm
                                        QString location = QString("(%1, %2) mm")
                                            .arg(physicalPos["x"].toDouble(), 0, 'f', 1)
                                            .arg(physicalPos["y"].toDouble(), 0, 'f', 1);
                                        QTableWidgetItem *locItem = new QTableWidgetItem(location);
                                        locItem->setTextAlignment(Qt::AlignCenter);
                                        defectTable->setItem(i, 3, locItem);

                                        // Size (width × height) in mm
                                        QString size_str = QString("%1 × %2 mm")
                                            .arg(physicalPos["width"].toDouble(), 0, 'f', 1)
                                            .arg(physicalPos["height"].toDouble(), 0, 'f', 1);
                                        QTableWidgetItem *sizeItem = new QTableWidgetItem(size_str);
                                        sizeItem->setTextAlignment(Qt::AlignCenter);
                                        defectTable->setItem(i, 4, sizeItem);
                                    }
                                    coordFile.close();
                                    qDebug() << "Updated defect view with" << defects.size() << "defects";
                                }
                            }
                        }
                    }
                };
                
                connect(debounceTimer, &QTimer::timeout, this, updateDefectView);
                debounceTimer->start();
                
                // Re-add the path to watch as it might have been removed
                if (!defectWatcher->directories().contains(surfacePath)) {
                    if (defectWatcher->addPath(surfacePath)) {
                        qDebug() << "Re-added watch for directory:" << surfacePath;
                    }
                }
            });
        
        // Open capture window
        CaptureWindow captureWindow(this, surfacePath,
                                  currentCaptureSettings.imagesInX,
                                  currentCaptureSettings.imagesInY,
                                  currentCaptureSettings.sequence);
        
        // Connect signals for real-time updates
        QTreeWidgetItem* captureItem = surfaceItem;  // Create a copy for the lambda
        connect(&captureWindow, &CaptureWindow::imageCaptured,
                [this, captureItem](const QString &imagePath) {
                    // Add image to tree immediately
                    QFileInfo fileInfo(imagePath);
                    QTreeWidgetItem *imageItem = new QTreeWidgetItem(captureItem);
                    imageItem->setText(0, fileInfo.fileName());
                    imageItem->setText(1, "Pending");
                    imageItem->setText(2, "-");

                    // Start defect detection with a small delay
                    if (defectDetector && defectDetector->isModelInitialized()) {
                        QTimer::singleShot(100, this, [this, imagePath]() {
                            defectDetector->detectImage(imagePath);
                        });
                    }
                });

        if (captureWindow.exec() == QDialog::Accepted)
        {
            // Delete any existing stitcher
            if (currentStitcher) {
                delete currentStitcher;
                currentStitcher = nullptr;
            }

            // Create new stitcher
            currentStitcher = new ImageStitcher(surfacePath,
                                 currentCaptureSettings.imagesInX,
                                 currentCaptureSettings.imagesInY,
                                 currentCaptureSettings.sequence,
                                 dimensions.actualWidth,
                                 dimensions.actualHeight);

            // Connect stitching completion signal
            connect(currentStitcher, &ImageStitcher::finished, this, [this, surfacePath]() {
                qDebug() << "Stitching complete signal received for:" << surfacePath;
                
                // Get the current surface item
                QTreeWidgetItem *currentItem = surfaceTree->currentItem();
                QString currentSurfacePath;
                
                if (currentItem && isSurfaceItem(currentItem)) {
                    currentSurfacePath = QString("%1/%2").arg(sessionPath).arg(currentItem->text(0));
                    qDebug() << "Current surface path:" << currentSurfacePath;
                    qDebug() << "Stitched surface path:" << surfacePath;
                    
                    if (currentSurfacePath == surfacePath) {
                        // Update the stitched image preview
                        QString stitchedImagePath = QString("%1/stitched.jpg").arg(surfacePath);
                        qDebug() << "Loading stitched image from:" << stitchedImagePath;
                        
                        if (QFile::exists(stitchedImagePath)) {
                            QPixmap pixmap(stitchedImagePath);
                            if (!pixmap.isNull()) {
                                QSize labelSize = originalImageLabel->size();
                                QPixmap scaledPixmap = pixmap.scaled(labelSize,
                                    Qt::KeepAspectRatio,
                                    Qt::FastTransformation);
                                originalImageLabel->setPixmap(scaledPixmap);
                                qDebug() << "Successfully updated stitched image preview";
                            } else {
                                qDebug() << "Failed to load stitched image pixmap";
                                originalImageLabel->setText("Failed to load stitched image");
                            }
                        } else {
                            qDebug() << "Stitched image file does not exist at:" << stitchedImagePath;
                            originalImageLabel->setText("No stitched image available");
                        }
                    }
                }

                // Cleanup stitcher
                delete currentStitcher;
                currentStitcher = nullptr;
            });

            // Start stitching process
            qDebug() << "Starting image stitching process for:" << surfacePath;
            currentStitcher->stitchImages();
        }
        else
        {
            // If capture was cancelled, remove the surface item and cleanup watcher
            delete surfaceItem;
            if (defectWatcher) {
                disconnect(defectWatcher, nullptr, this, nullptr);
                delete defectWatcher;
                defectWatcher = nullptr;
            }
        }
    }
}

void MainWindow::loadSurfaces()
{
    // Clear existing items first
    surfaceTree->clear();

    QDir sessionDir(sessionPath);
    QStringList surfaceDirs = sessionDir.entryList(QStringList() << "surface_*", QDir::Dirs);
    
    for (const QString &surfaceDir : surfaceDirs)
    {
        QString surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaceDir);
        qDebug() << "Loading surface from path:" << surfacePath;
        
        QTreeWidgetItem *surfaceItem = new QTreeWidgetItem(surfaceTree);
        surfaceItem->setText(0, surfaceDir);
        
        // Load surface status and defect count
        QDir dir(surfacePath);
        QStringList images = dir.entryList(QStringList() << "image_*.jpg", QDir::Files);
        images = images.filter(QRegularExpression("^image_\\d+\\.jpg$")); // Only get original images
        
        qDebug() << "Found" << images.size() << "images for surface:" << surfaceDir;
        
        int totalDefects = 0;
        int analyzedCount = 0;
        
        // Add image items
        for (const QString &image : images)
        {
            QTreeWidgetItem *imageItem = new QTreeWidgetItem(surfaceItem);
            imageItem->setText(0, image);
            
            QString baseName = image;
            baseName.chop(4); // Remove .jpg
            QString detectionFile = QString("%1/%2_detections.json").arg(surfacePath).arg(baseName);
            
            qDebug() << "Checking detection file:" << detectionFile;
            
            if (QFile::exists(detectionFile))
            {
                QFile file(detectionFile);
                if (file.open(QIODevice::ReadOnly))
                {
                    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                    QJsonObject obj = doc.object();
                    int defectCount = obj["detections"].toArray().size();
                    totalDefects += defectCount;
                    analyzedCount++;
                    
                    imageItem->setText(1, "Analyzed");
                    imageItem->setText(2, QString::number(defectCount));
                    qDebug() << "Image" << image << "is analyzed with" << defectCount << "defects";
                    file.close();
                }
            }
            else
            {
                QString processingFile = QString("%1/%2_processing").arg(surfacePath).arg(baseName);
                if (QFile::exists(processingFile))
                {
                    imageItem->setText(1, "Processing");
                    qDebug() << "Image" << image << "is currently processing";
                }
                else
                {
                    imageItem->setText(1, "Pending");
                    qDebug() << "Image" << image << "is pending analysis";
                }
                imageItem->setText(2, "-");
            }
        }
        
        // Update surface status
        if (images.isEmpty())
        {
            surfaceItem->setText(1, "Empty");
            qDebug() << "Surface" << surfaceDir << "is empty";
        }
        else if (analyzedCount == 0)
        {
            surfaceItem->setText(1, "Pending");
            qDebug() << "Surface" << surfaceDir << "is pending";
        }
        else if (analyzedCount < images.size())
        {
            surfaceItem->setText(1, "Processing");
            qDebug() << "Surface" << surfaceDir << "is processing (" << analyzedCount << "/" << images.size() << " analyzed)";
        }
        else
        {
            surfaceItem->setText(1, "Analyzed");
            qDebug() << "Surface" << surfaceDir << "is fully analyzed with" << totalDefects << "total defects";
        }
        
        surfaceItem->setText(2, QString::number(totalDefects));
    }
}

void MainWindow::onItemSelectionChanged()
{
    QTreeWidgetItem *currentItem = surfaceTree->currentItem();
    if (!currentItem)
    {
        deleteSurfaceButton->setEnabled(false);
        return;
    }
    
    // Enable delete button only for surface items
    deleteSurfaceButton->setEnabled(isSurfaceItem(currentItem));

    // Clear defect table
    defectTable->setRowCount(0);
    defectTable->setColumnCount(5);
    defectTable->setHorizontalHeaderLabels({"Number", "Type", "Confidence", "Location", "Size"});
    
    // Update image preview
    if (isSurfaceItem(currentItem))
    {
        // For surface items, show stitched image if available
        QString surfacePath = QString("%1/%2").arg(sessionPath).arg(currentItem->text(0));
        QString stitchedImagePath = QString("%1/stitched.jpg").arg(surfacePath);
        QString labeledImagePath = QString("%1/stitched_labeled.jpg").arg(surfacePath);
        QString coordPath = QString("%1/defect_coordinates.json").arg(surfacePath);
        
        // Show original stitched image in the original area
        if (QFile::exists(stitchedImagePath)) {
            QPixmap pixmap(stitchedImagePath);
            if (!pixmap.isNull()) {
                originalImageLabel->setPixmap(pixmap.scaled(originalImageLabel->size(),
                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
                originalImageLabel->setText("Failed to load stitched image");
            }
        } else {
            originalImageLabel->setText("No stitched image available");
        }
        
        // Function to update defect view
        std::function<void()> updateDefectView = [this, labeledImagePath, coordPath]() {
            qDebug() << "Updating defect view...";
            if (QFile::exists(labeledImagePath)) {
                QPixmap labeledPixmap(labeledImagePath);
                if (!labeledPixmap.isNull()) {
                    defectImageLabel->setPixmap(labeledPixmap.scaled(defectImageLabel->size(),
                        Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    
                    // Load and display defect details from JSON
                    QFile coordFile(coordPath);
                    if (coordFile.open(QIODevice::ReadOnly)) {
                        QJsonDocument doc = QJsonDocument::fromJson(coordFile.readAll());
                        QJsonObject mainObj = doc.object();
                        QJsonArray defects = mainObj["defects"].toArray();
                        
                        defectTable->setRowCount(defects.size());
                        for (int i = 0; i < defects.size(); ++i) {
                            QJsonObject defect = defects[i].toObject();
                            
                            // Number
                            QTableWidgetItem *numberItem = new QTableWidgetItem(QString::number(i + 1));
                            numberItem->setTextAlignment(Qt::AlignCenter);
                            defectTable->setItem(i, 0, numberItem);

                            // Type
                            QTableWidgetItem *typeItem = new QTableWidgetItem(defect["type"].toString());
                            typeItem->setTextAlignment(Qt::AlignCenter);
                            defectTable->setItem(i, 1, typeItem);

                            // Confidence (ensure it's in 0-100% range)
                            double confidence = defect["confidence"].toDouble();
                            if (confidence > 1) {
                                confidence = confidence / 100.0;
                            }
                            QTableWidgetItem *confItem = new QTableWidgetItem(
                                QString("%1%").arg(confidence * 100, 0, 'f', 1));
                            confItem->setTextAlignment(Qt::AlignCenter);
                            defectTable->setItem(i, 2, confItem);

                            // For surface groups, we have physical_position object
                            QJsonObject physicalPos = defect["physical_position"].toObject();
                            
                            // Location (x, y) in mm
                            QString location = QString("(%1, %2) mm")
                                .arg(physicalPos["x"].toDouble(), 0, 'f', 1)
                                .arg(physicalPos["y"].toDouble(), 0, 'f', 1);
                            QTableWidgetItem *locItem = new QTableWidgetItem(location);
                            locItem->setTextAlignment(Qt::AlignCenter);
                            defectTable->setItem(i, 3, locItem);

                            // Size (width × height) in mm
                            QString size_str = QString("%1 × %2 mm")
                                .arg(physicalPos["width"].toDouble(), 0, 'f', 1)
                                .arg(physicalPos["height"].toDouble(), 0, 'f', 1);
                            QTableWidgetItem *sizeItem = new QTableWidgetItem(size_str);
                            sizeItem->setTextAlignment(Qt::AlignCenter);
                            defectTable->setItem(i, 4, sizeItem);
                        }
                        coordFile.close();
                        qDebug() << "Updated defect view with" << defects.size() << "defects";
                    }
                } else {
                    defectImageLabel->setText("Failed to load labeled image");
                }
            } else {
                defectImageLabel->setText("No labeled image available");
            }
        };

        // Initial update
        updateDefectView();

        // Set up file watcher for the labeled image and coordinates file
        if (defectWatcher) {
            disconnect(defectWatcher, nullptr, this, nullptr);  // Disconnect all previous connections
            delete defectWatcher;
        }
        defectWatcher = new QFileSystemWatcher(this);
        defectWatcher->addPath(surfacePath);
        
        // Connect the file watcher with a small delay to avoid rapid updates
        connect(defectWatcher, &QFileSystemWatcher::directoryChanged, this, 
            [this, updateDefectView, labeledImagePath]() {
                static QTimer *debounceTimer = nullptr;
                if (debounceTimer) {
                    debounceTimer->stop();
                    delete debounceTimer;
                }
                debounceTimer = new QTimer(this);
                debounceTimer->setSingleShot(true);
                debounceTimer->setInterval(500);  // 500ms delay
                connect(debounceTimer, &QTimer::timeout, this, [updateDefectView, labeledImagePath]() {
                    if (QFile::exists(labeledImagePath)) {
                        updateDefectView();
                    }
                });
                debounceTimer->start();
            });
    }
    else
    {
        // For image items, show both original and detected images
        QString surfacePath = QString("%1/%2").arg(sessionPath).arg(currentItem->parent()->text(0));
        QString imagePath = QString("%1/%2").arg(surfacePath).arg(currentItem->text(0));
        
        // Show original image
        updatePreviewImage(imagePath);

        // Show detected image
        QString detectedImagePath = imagePath;
        detectedImagePath.replace(".jpg", "_detected.jpg");
        QPixmap detectedPixmap(detectedImagePath);
        if (!detectedPixmap.isNull()) {
            QSize labelSize = defectImageLabel->size();
            QPixmap scaledPixmap = detectedPixmap.scaled(labelSize, 
                Qt::KeepAspectRatio, 
                Qt::FastTransformation);
            defectImageLabel->setPixmap(scaledPixmap);
        } else {
            defectImageLabel->setText("No defects detected yet");
        }

        // Load defect details if available
        QString detectionFile = imagePath;
        detectionFile.replace(".jpg", "_detections.json");
        QFile file(detectionFile);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject obj = doc.object();
            QJsonArray detections = obj["detections"].toArray();

            // Populate defect table
            defectTable->setRowCount(detections.size());
            for (int i = 0; i < detections.size(); ++i) {
                QJsonObject detection = detections[i].toObject();
                
                // Number
                QTableWidgetItem *numberItem = new QTableWidgetItem(QString::number(i + 1));
                numberItem->setTextAlignment(Qt::AlignCenter);
                defectTable->setItem(i, 0, numberItem);

                // Type
                QTableWidgetItem *typeItem = new QTableWidgetItem(detection["class_name"].toString());
                typeItem->setTextAlignment(Qt::AlignCenter);
                defectTable->setItem(i, 1, typeItem);

                // Confidence (ensure it's in 0-100% range)
                double confidence = detection["confidence"].toDouble();
                if (confidence > 1) {
                    confidence = confidence / 100.0;
                }
                QTableWidgetItem *confItem = new QTableWidgetItem(QString("%1%").arg(confidence * 100, 0, 'f', 1));
                confItem->setTextAlignment(Qt::AlignCenter);
                defectTable->setItem(i, 2, confItem);

                // For individual pieces, coordinates are directly in the detection object
                // Location (center x, y)
                QString location = QString("(%1, %2)")
                    .arg(detection["center_x"].toInt())
                    .arg(detection["center_y"].toInt());
                QTableWidgetItem *locItem = new QTableWidgetItem(location);
                locItem->setTextAlignment(Qt::AlignCenter);
                defectTable->setItem(i, 3, locItem);

                // Size (width x height)
                QString size = QString("%1 × %2")
                    .arg(detection["width"].toInt())
                    .arg(detection["height"].toInt());
                QTableWidgetItem *sizeItem = new QTableWidgetItem(size);
                sizeItem->setTextAlignment(Qt::AlignCenter);
                defectTable->setItem(i, 4, sizeItem);
            }
            file.close();
        }
    }
}

void MainWindow::updatePreviewImage(const QString &imagePath)
{
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull())
    {
        originalImageLabel->setPixmap(pixmap.scaled(originalImageLabel->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
}

bool MainWindow::isSurfaceItem(QTreeWidgetItem *item) const
{
    return item->parent() == nullptr;
}

void MainWindow::onItemExpanded(QTreeWidgetItem *item)
{
    // No need to reload images as they are already loaded in loadSurfaces()
    // Just update the UI if needed
    if (isSurfaceItem(item))
    {
        QString surfacePath = QString("%1/%2").arg(sessionPath).arg(item->text(0));
        QString stitchedImagePath = QString("%1/stitched.jpg").arg(surfacePath);
        
        if (QFile::exists(stitchedImagePath))
        {
            QPixmap pixmap(stitchedImagePath);
            if (!pixmap.isNull())
            {
                originalImageLabel->setPixmap(pixmap.scaled(originalImageLabel->size(),
                    Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    }
}

void MainWindow::onItemCollapsed(QTreeWidgetItem *item)
{
    // Optional: You can add any special handling for collapsed items here
}

void MainWindow::onDeleteSurface()
{
    QTreeWidgetItem *currentItem = surfaceTree->currentItem();
    if (!currentItem || !isSurfaceItem(currentItem))
    {
        return;
    }

    QString surfacePath = QString("%1/%2").arg(sessionPath).arg(currentItem->text(0));
    QDir surfaceDir(surfacePath);
    
    if (QMessageBox::question(this, "Delete Surface",
        QString("Are you sure you want to delete %1 and all its images?").arg(currentItem->text(0)),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
    {
        
        if (surfaceDir.removeRecursively())
        {
            delete currentItem;
            originalImageLabel->setText("Select an image to preview");
            defectImageLabel->setText("No defects detected yet");
        }
        else
        {
            QMessageBox::warning(this, "Error", "Failed to delete surface directory");
        }
    }
}

void MainWindow::onCutSurface()
{
    // Get the currently selected surface group
    QTreeWidgetItem* selectedItem = surfaceTree->currentItem();
    if (!selectedItem || selectedItem->parent() != nullptr) {
        QMessageBox::warning(this, "Warning", "Please select a surface group to cut.");
        return;
    }

    // Count total number of surface groups
    int surfaceCount = surfaceTree->topLevelItemCount();
    if (surfaceCount == 0) {
        QMessageBox::warning(this, "Error", "No surfaces found in the session.");
        return;
    }

    // Get surface dimensions from the JSON data
    QString surfaceGroupName = selectedItem->text(0);
    QString surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaceGroupName);
    QString coordFile = QString("%1/defect_coordinates.json").arg(surfacePath);
    
    // Read the JSON file
    QFile file(coordFile);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not read surface data.");
        return;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject surfaceGroup = doc.object();
    file.close();

    // Create and show the cutting configuration dialog
    CuttingConfigDialog dialog(this, 
                              dimensions.actualWidth,
                              dimensions.actualHeight,
                              dimensions.capturedWidth,
                              dimensions.capturedHeight,
                              surfaceCount);
    
    if (dialog.exec() == QDialog::Accepted) {
        int piecesInX = dialog.getPiecesInX();
        int piecesInY = dialog.getPiecesInY();
        bool useXAxisStacking = dialog.isXAxisStacking();
        
        qDebug() << "Opening cutting window with configuration:";
        qDebug() << "Pieces:" << piecesInX << "x" << piecesInY;
        qDebug() << "X-axis stacking:" << useXAxisStacking;
        
        // Create and show the cutting window modally
        // Pass sessionPath instead of surfacePath to analyze all surfaces
        CuttingWindow *cuttingWindow = new CuttingWindow(this, sessionPath, piecesInX, piecesInY, useXAxisStacking);
        cuttingWindow->setAttribute(Qt::WA_DeleteOnClose); // Automatically delete when closed
        
        // Perform the cutting analysis before showing the window
        cuttingWindow->performCuttingAnalysis();
        
        cuttingWindow->exec(); // Show modal dialog and wait for it to close
    }
} 