#include "cuttingwindow.h"
#include "cuttinganalyzer.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>
#include <QFileInfo>
#include <QDebug>
#include <QPainter>
#include <QFont>
#include <QPen>
#include <QRectF>

CuttingWindow::CuttingWindow(QWidget *parent, 
                           const QString &sessionPath,
                           int piecesInX,
                           int piecesInY,
                           bool useXAxisStacking)
    : QDialog(parent),
      sessionPath(sessionPath),
      piecesInX(piecesInX),
      piecesInY(piecesInY),
      useXAxisStacking(useXAxisStacking),
      analyzer(nullptr)
{
    setupUI();
    loadSurfaces();
    // Note: performCuttingAnalysis() will be called after user confirms configuration
}

CuttingWindow::~CuttingWindow()
{
    delete analyzer;
}

void CuttingWindow::performCuttingAnalysis()
{
    qDebug() << "\n=== Starting Cutting Analysis Process ===";
    qDebug() << "Session path:" << sessionPath;
    qDebug() << "Configuration:";
    qDebug() << "- Pieces:" << piecesInX << "x" << piecesInY;
    qDebug() << "- Surface dimensions: 420.0 x 297.0 mm (A3)";
    qDebug() << "- Stacking method:" << (useXAxisStacking ? "X-axis" : "Single Stack");

    QStringList surfacePaths;
    QDir dir(sessionPath);
    
    // Check if the path is already a surface directory
    if (dir.dirName().startsWith("surface_")) {
        qDebug() << "Direct surface path detected";
        surfacePaths.append(sessionPath);
    } else {
        // Look for surface directories within the session
        qDebug() << "Searching for surfaces in session directory";
        QStringList surfaceDirs = dir.entryList(QStringList() << "surface_*", QDir::Dirs);
        for (const QString &surfaceDir : surfaceDirs) {
            surfacePaths.append(QString("%1/%2").arg(sessionPath).arg(surfaceDir));
        }
    }

    qDebug() << "Found" << surfacePaths.size() << "surfaces to analyze";

    for (const QString &surfacePath : surfacePaths) {
        qDebug() << "\n--- Processing Surface:" << QDir(surfacePath).dirName() << "---";
        qDebug() << "Surface path:" << surfacePath;

        // Create a new analyzer for this surface
        qDebug() << "Creating new CuttingAnalyzer instance...";
        delete analyzer; // Delete previous analyzer if it exists
        analyzer = new CuttingAnalyzer(surfacePath, piecesInX, piecesInY, 420.0, 297.0); // Standard A3 size

        // Perform the analysis for this surface
        qDebug() << "Starting surface analysis...";
        if (!analyzer->analyzeSurfaces()) {
            qWarning() << "Failed to analyze surface:" << QDir(surfacePath).dirName();
        } else {
            qDebug() << "Successfully analyzed surface:" << QDir(surfacePath).dirName();
        }
        qDebug() << "--- Finished Processing Surface:" << QDir(surfacePath).dirName() << "---\n";
    }

    qDebug() << "Refreshing UI with analysis results...";
    // Refresh the UI to show the analysis results
    if (surfaceList->topLevelItemCount() > 0) {
        surfaceList->setCurrentItem(surfaceList->topLevelItem(0));
        onSurfaceSelectionChanged();
        updateStackPreview();
    }
    qDebug() << "=== Cutting Analysis Process Complete ===\n";
}

void CuttingWindow::setupUI()
{
    setWindowTitle("Surface Cutting Preview");
    resize(1600, 900);

    mainLayout = new QHBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    setupSurfaceColumn();
    setupDefectColumn();
    setupStackingColumn();
    setupSummaryColumn();
}

void CuttingWindow::setupSurfaceColumn()
{
    QGroupBox *surfaceGroup = new QGroupBox("Surface Preview");
    surfaceColumn = new QVBoxLayout(surfaceGroup);
    surfaceColumn->setSpacing(10);

    // Navigation controls at the top
    QHBoxLayout *navLayout = new QHBoxLayout();
    prevButton = new QPushButton("← Previous Surface");
    nextButton = new QPushButton("Next Surface →");
    currentSurfaceLabel = new QLabel();
    currentSurfaceLabel->setAlignment(Qt::AlignCenter);
    currentSurfaceLabel->setStyleSheet("QLabel { font-weight: bold; }");

    // Set minimum width for buttons to prevent text truncation
    prevButton->setMinimumWidth(150);
    nextButton->setMinimumWidth(150);

    navLayout->addWidget(prevButton);
    navLayout->addWidget(currentSurfaceLabel, 1);
    navLayout->addWidget(nextButton);
    surfaceColumn->addLayout(navLayout);

    // Defect preview section
    QLabel *defectLabel = new QLabel("Defect Detection Preview");
    defectLabel->setAlignment(Qt::AlignCenter);
    defectLabel->setStyleSheet("QLabel { font-weight: bold; margin-top: 10px; }");
    surfaceColumn->addWidget(defectLabel);

    QScrollArea *defectScroll = new QScrollArea;
    defectScroll->setWidgetResizable(true);
    defectScroll->setMinimumHeight(350);
    defectScroll->setMaximumHeight(350);
    
    defectPreview = new QLabel;
    defectPreview->setMinimumSize(500, 350);
    defectPreview->setMaximumHeight(350);
    defectPreview->setAlignment(Qt::AlignCenter);
    defectPreview->setStyleSheet("QLabel { background-color: black; color: white; }");
    defectPreview->setText("No defects to display");
    
    defectScroll->setWidget(defectPreview);
    surfaceColumn->addWidget(defectScroll);

    // Cutting preview section
    QLabel *cuttingLabel = new QLabel("Cutting Grid Preview");
    cuttingLabel->setAlignment(Qt::AlignCenter);
    cuttingLabel->setStyleSheet("QLabel { font-weight: bold; margin-top: 10px; }");
    surfaceColumn->addWidget(cuttingLabel);

    QScrollArea *cuttingScroll = new QScrollArea;
    cuttingScroll->setWidgetResizable(true);
    cuttingScroll->setMinimumHeight(350);
    cuttingScroll->setMaximumHeight(350);
    
    cuttingPreview = new ClickableLabel;  // Changed to ClickableLabel
    cuttingPreview->setMinimumSize(500, 350);
    cuttingPreview->setMaximumHeight(350);
    cuttingPreview->setAlignment(Qt::AlignCenter);
    cuttingPreview->setStyleSheet("QLabel { background-color: #f0f0f0; color: #666; cursor: pointer; }");
    cuttingPreview->setText("Cutting visualization");
    
    cuttingScroll->setWidget(cuttingPreview);
    surfaceColumn->addWidget(cuttingScroll);

    mainLayout->addWidget(surfaceGroup);

    // Connect signals
    connect(prevButton, &QPushButton::clicked, this, &CuttingWindow::onPreviousSurface);
    connect(nextButton, &QPushButton::clicked, this, &CuttingWindow::onNextSurface);
    connect(cuttingPreview, &ClickableLabel::clicked, this, &CuttingWindow::onCuttingPreviewClicked);
}

void CuttingWindow::setupDefectColumn()
{
    QGroupBox *defectGroup = new QGroupBox("Defect Details");
    defectColumn = new QVBoxLayout(defectGroup);
    defectColumn->setSpacing(10);

    // Hidden surface list for navigation
    surfaceList = new QTreeWidget;
    surfaceList->setHeaderLabels({"Surface", "Status"});
    surfaceList->hide(); // Hide the list but keep it for navigation

    // Defect table with more height
    defectTable = new QTableWidget;
    defectTable->setColumnCount(5);
    defectTable->setHorizontalHeaderLabels({"Number", "Type", "Confidence", "Location", "Size"});
    defectTable->verticalHeader()->setVisible(false);
    defectTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    defectTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    defectTable->setMinimumHeight(700); // Increased height
    
    // Set column widths
    defectTable->setColumnWidth(0, 60);  // Number
    defectTable->setColumnWidth(1, 80);  // Type
    defectTable->setColumnWidth(2, 80);  // Confidence
    defectTable->setColumnWidth(3, 100); // Location
    defectTable->setColumnWidth(4, 100); // Size

    defectColumn->addWidget(defectTable);
    mainLayout->addWidget(defectGroup);

    // Connect signals
    connect(surfaceList, &QTreeWidget::itemSelectionChanged,
            this, &CuttingWindow::onSurfaceSelectionChanged);
}

void CuttingWindow::onPreviousSurface()
{
    QTreeWidgetItem *currentItem = surfaceList->currentItem();
    if (!currentItem) return;

    int currentIndex = surfaceList->indexOfTopLevelItem(currentItem);
    if (currentIndex > 0) {
        surfaceList->setCurrentItem(surfaceList->topLevelItem(currentIndex - 1));
    }
}

void CuttingWindow::onNextSurface()
{
    QTreeWidgetItem *currentItem = surfaceList->currentItem();
    if (!currentItem) return;

    int currentIndex = surfaceList->indexOfTopLevelItem(currentItem);
    if (currentIndex < surfaceList->topLevelItemCount() - 1) {
        surfaceList->setCurrentItem(surfaceList->topLevelItem(currentIndex + 1));
    }
}

void CuttingWindow::updateNavigationButtons()
{
    QTreeWidgetItem *currentItem = surfaceList->currentItem();
    if (!currentItem) {
        prevButton->setEnabled(false);
        nextButton->setEnabled(false);
        currentSurfaceLabel->setText("No surface selected");
        return;
    }

    int currentIndex = surfaceList->indexOfTopLevelItem(currentItem);
    prevButton->setEnabled(currentIndex > 0);
    nextButton->setEnabled(currentIndex < surfaceList->topLevelItemCount() - 1);
    currentSurfaceLabel->setText(currentItem->text(0));
}

void CuttingWindow::drawCuttingGrid(QLabel *label, const QPixmap &baseImage)
{
    if (baseImage.isNull()) return;

    // Create a new pixmap for drawing
    QPixmap workingImage = baseImage.scaled(label->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QPainter painter(&workingImage);
    
    // Get current surface path
    QTreeWidgetItem *currentItem = surfaceList->currentItem();
    if (!currentItem) return;
    QString surfacePath = QString("%1/%2").arg(sessionPath).arg(currentItem->text(0));
    
    // Load cutting analysis file
    QString analysisPath = QString("%1/cutting_analysis.json").arg(surfacePath);
    QFile file(analysisPath);
    QStringList piecesWithDefects;
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        QJsonObject root = doc.object();
        QJsonArray defectPieces = root["pieces_with_defects"].toArray();
        
        for (const QJsonValue &value : defectPieces) {
            piecesWithDefects.append(value.toString());
        }
        file.close();
    }

    // Calculate grid dimensions
    int width = workingImage.width();
    int height = workingImage.height();
    int cellWidth = width / piecesInX;
    int cellHeight = height / piecesInY;

    // Set up the pen for grid lines
    QPen pen(Qt::red);
    pen.setWidth(2);
    painter.setPen(pen);

    // Draw grid and fill pieces with defects
    for (int x = 0; x < piecesInX; ++x) {
        for (int y = 0; y < piecesInY; ++y) {
            // Check if this piece has defects
            QString pieceId = QString("x%1y%2").arg(x + 1).arg(y + 1);
            if (piecesWithDefects.contains(pieceId)) {
                // Fill piece with semi-transparent red
                QColor fillColor(255, 0, 0, 128); // Red with 50% opacity
                painter.fillRect(x * cellWidth, y * cellHeight, cellWidth, cellHeight, fillColor);
            }
            
            // Draw grid lines
            if (x > 0) {
                painter.drawLine(x * cellWidth, 0, x * cellWidth, height);
            }
            if (y > 0) {
                painter.drawLine(0, y * cellHeight, width, y * cellHeight);
            }
        }
    }

    // Draw outer border
    painter.drawRect(0, 0, width - 1, height - 1);

    // Update the label with the grid overlay
    label->setPixmap(workingImage);
}

void CuttingWindow::updateDefectPreview(const QString &surfacePath)
{
    // Load and display the labeled image
    QString stitchedImagePath = QString("%1/stitched_labeled.jpg").arg(surfacePath);
    QPixmap pixmap(stitchedImagePath);
    
    if (!pixmap.isNull()) {
        // Update defect preview
        QSize labelSize = defectPreview->size();
        defectPreview->setPixmap(pixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

        // Update cutting preview with grid
        drawCuttingGrid(cuttingPreview, pixmap);
    } else {
        defectPreview->setText("Failed to load surface preview");
        cuttingPreview->setText("No cutting preview available");
        return;
    }

    // Update navigation buttons
    updateNavigationButtons();

    // Load and display defect details
    QString coordFile = QString("%1/defect_coordinates.json").arg(surfacePath);
    QFile file(coordFile);
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
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
            QString size = QString("%1 × %2 mm")
                .arg(physicalPos["width"].toDouble(), 0, 'f', 1)
                .arg(physicalPos["height"].toDouble(), 0, 'f', 1);
            QTableWidgetItem *sizeItem = new QTableWidgetItem(size);
            sizeItem->setTextAlignment(Qt::AlignCenter);
            defectTable->setItem(i, 4, sizeItem);
        }
        file.close();
    }
}

void CuttingWindow::loadSurfaces()
{
    QDir sessionDir(sessionPath);
    QStringList surfaceDirs = sessionDir.entryList(QStringList() << "surface_*", QDir::Dirs);

    for (const QString &surfaceDir : surfaceDirs) {
        QString surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaceDir);
        QString coordFile = QString("%1/defect_coordinates.json").arg(surfacePath);

        QTreeWidgetItem *item = new QTreeWidgetItem(surfaceList);
        item->setText(0, surfaceDir);
        
        if (QFile::exists(coordFile)) {
            item->setText(1, "Ready");
        } else {
            item->setText(1, "Not Ready");
        }
    }

    // Select the first surface if available
    if (surfaceList->topLevelItemCount() > 0) {
        surfaceList->setCurrentItem(surfaceList->topLevelItem(0));
    }
}

void CuttingWindow::onSurfaceSelectionChanged()
{
    QTreeWidgetItem *currentItem = surfaceList->currentItem();
    if (!currentItem) return;

    QString surfacePath = QString("%1/%2").arg(sessionPath).arg(currentItem->text(0));
    updateDefectPreview(surfacePath);
    updateStackPreview();
    updateSummaryText();
}

void CuttingWindow::setupStackingColumn()
{
    QGroupBox *stackGroup = new QGroupBox("Stacking Preview");
    stackColumn = new QVBoxLayout(stackGroup);
    stackColumn->setSpacing(5);
    stackColumn->setContentsMargins(5, 5, 5, 5);

    // Add legend
    QWidget* legendWidget = new QWidget;
    QHBoxLayout* legendLayout = new QHBoxLayout(legendWidget);
    legendLayout->setSpacing(10);
    legendLayout->setContentsMargins(2, 2, 2, 2);
    
    // Normal piece example
    StackPieceWidget* normalPiece = new StackPieceWidget(1, "x1y1", false);
    QLabel* normalLabel = new QLabel("Normal");
    normalLabel->setStyleSheet("QLabel { font-size: 8pt; }");
    legendLayout->addWidget(normalPiece);
    legendLayout->addWidget(normalLabel);
    
    // Defective piece example
    StackPieceWidget* defectPiece = new StackPieceWidget(1, "x1y1", true);
    QLabel* defectLabel = new QLabel("Defective");
    defectLabel->setStyleSheet("QLabel { font-size: 8pt; }");
    legendLayout->addWidget(defectPiece);
    legendLayout->addWidget(defectLabel);
    
    legendLayout->addStretch();
    stackColumn->addWidget(legendWidget);

    // Create scroll area
    stackScroll = new QScrollArea;
    stackScroll->setWidgetResizable(true);
    stackScroll->setMinimumHeight(600);
    stackScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    stackScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // Create container for stacks
    stackContainer = new QWidget;
    stackGrid = new QHBoxLayout(stackContainer);
    stackGrid->setSpacing(5);
    stackGrid->setContentsMargins(5, 5, 5, 5);
    stackGrid->setAlignment(Qt::AlignLeft);
    
    stackScroll->setWidget(stackContainer);
    stackColumn->addWidget(stackScroll);
    mainLayout->addWidget(stackGroup);

    // Initial update
    updateStackPreview();
}

void CuttingWindow::updateStackPreview()
{
    // Clear existing stacks
    QLayoutItem* item;
    while ((item = stackGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // Get all surfaces in order
    QList<QTreeWidgetItem*> surfaces;
    for (int i = 0; i < surfaceList->topLevelItemCount(); i++) {
        surfaces.append(surfaceList->topLevelItem(i));
    }

    // Safety check
    if (surfaces.isEmpty() || piecesInX <= 0 || piecesInY <= 0) {
        QLabel* noDataLabel = new QLabel("No data to display");
        noDataLabel->setAlignment(Qt::AlignCenter);
        stackGrid->addWidget(noDataLabel);
        return;
    }

    const int maxPiecesPerStack = 50;
    const int totalSurfaces = surfaces.size();
    
    if (useXAxisStacking) {
        // Original X-axis stacking logic
        int piecesPerSurface = piecesInX;
        int totalPiecesPerY = totalSurfaces * piecesPerSurface;
        int stackGroupsNeeded = (totalPiecesPerY + maxPiecesPerStack - 1) / maxPiecesPerStack;
        
        QList<StackWidget*> allStacks;
        
        // Create stacks for each Y position and group
        for (int groupIndex = 0; groupIndex < stackGroupsNeeded; groupIndex++) {
            for (int y = 1; y <= piecesInY; y++) {
                // Calculate sequential stack number
                int stackNumber = y + (groupIndex * piecesInY);
                QString stackLabel = QString("Stack %1").arg(stackNumber);
                StackWidget* stack = new StackWidget(stackLabel, maxPiecesPerStack);
                
                // Calculate which surfaces go in this stack group
                int surfacesPerGroup = maxPiecesPerStack / piecesPerSurface;
                int startSurface = groupIndex * surfacesPerGroup;
                int endSurface = qMin(startSurface + surfacesPerGroup, totalSurfaces);
                
                // Add pieces to this stack from bottom to top
                for (int surfaceIndex = startSurface; surfaceIndex < endSurface; surfaceIndex++) {
                    QString surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaces[surfaceIndex]->text(0));
                    QString analysisPath = QString("%1/cutting_analysis.json").arg(surfacePath);
                    QFile file(analysisPath);
                    
                    if (!file.open(QIODevice::ReadOnly)) {
                        qDebug() << "Failed to open analysis file:" << analysisPath;
                        continue;
                    }

                    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                    QJsonObject root = doc.object();
                    QJsonArray defectPieces = root["pieces_with_defects"].toArray();
                    file.close();

                    // Add pieces from this surface in order (x1 at bottom)
                    for (int x = 1; x <= piecesInX; x++) {
                        QString pieceId = QString("x%1y%2").arg(x).arg(y);
                        
                        // Check if piece has defects
                        bool hasDefect = false;
                        for (const QJsonValue &value : defectPieces) {
                            if (value.toString() == pieceId) {
                                hasDefect = true;
                                break;
                            }
                        }

                        stack->addPiece(surfaceIndex + 1, pieceId, hasDefect);
                    }
                }

                allStacks.append(stack);
            }
        }

        // Add stacks in the correct order (1 to n)
        for (int i = 0; i < allStacks.size(); i++) {
            stackGrid->addWidget(allStacks[i]);
        }
    } else {
        // Single stacking implementation
        QList<StackWidget*> allStacks;
        StackWidget* currentStack = nullptr;
        int currentPieceCount = 0;
        int stackCount = 0;

        // Process each surface
        for (int surfaceIndex = 0; surfaceIndex < totalSurfaces; surfaceIndex++) {
            QString surfacePath = QString("%1/%2").arg(sessionPath).arg(surfaces[surfaceIndex]->text(0));
            QString analysisPath = QString("%1/cutting_analysis.json").arg(surfacePath);
            QFile file(analysisPath);
            
            if (!file.open(QIODevice::ReadOnly)) {
                qDebug() << "Failed to open analysis file:" << analysisPath;
                continue;
            }

            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonObject root = doc.object();
            QJsonArray defectPieces = root["pieces_with_defects"].toArray();
            file.close();

            // For each Y value (y1 first, then y2)
            for (int y = 1; y <= piecesInY; y++) {
                // For each X position
                for (int x = 1; x <= piecesInX; x++) {
                    // Create new stack if needed
                    if (currentStack == nullptr || currentPieceCount >= maxPiecesPerStack) {
                        stackCount++;
                        currentStack = new StackWidget(QString("Stack %1").arg(stackCount), maxPiecesPerStack);
                        allStacks.append(currentStack);
                        currentPieceCount = 0;
                    }

                    // Add piece to current stack
                    QString pieceId = QString("x%1y%2").arg(x).arg(y);
                    bool hasDefect = false;
                    for (const QJsonValue &value : defectPieces) {
                        if (value.toString() == pieceId) {
                            hasDefect = true;
                            break;
                        }
                    }

                    currentStack->addPiece(surfaceIndex + 1, pieceId, hasDefect);
                    currentPieceCount++;
                }
            }
        }

        // Add all stacks to the grid
        for (StackWidget* stack : allStacks) {
            stackGrid->addWidget(stack);
        }
    }

    // Set minimum width for container to ensure horizontal scrolling works
    int totalStacks = stackGrid->count();
    int totalWidth = (totalStacks * 120) + 20; // 120 = stack width + spacing
    stackContainer->setMinimumWidth(totalWidth);
}

void CuttingWindow::setupSummaryColumn()
{
    QGroupBox *summaryGroup = new QGroupBox("Cutting Summary");
    summaryColumn = new QVBoxLayout(summaryGroup);
    summaryColumn->setSpacing(10);

    // Create the label and scroll area first
    summaryLabel = new QLabel;
    summaryLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    summaryLabel->setWordWrap(true);

    QScrollArea* scrollArea = new QScrollArea;
    scrollArea->setWidget(summaryLabel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumHeight(300);
    
    summaryColumn->addWidget(scrollArea);
    mainLayout->addWidget(summaryGroup);

    // Update the summary text
    updateSummaryText();
}

void CuttingWindow::updateSummaryText()
{
    if (!summaryLabel) return;

    // Calculate total pieces and stack information
    const int maxPiecesPerStack = 50;
    const int totalSurfaces = surfaceList->topLevelItemCount();
    const int piecesPerSurface = piecesInX * piecesInY;
    const int totalPieces = totalSurfaces * piecesPerSurface;
    
    int totalStacks;
    if (useXAxisStacking) {
        // For X-axis stacking:
        // - Each Y position gets its own stack
        // - Number of pieces per Y position = totalSurfaces * piecesInX
        int piecesPerY = totalSurfaces * piecesInX;
        int stackGroupsNeeded = (piecesPerY + maxPiecesPerStack - 1) / maxPiecesPerStack;
        totalStacks = piecesInY * stackGroupsNeeded;
    } else {
        // For single stacking:
        // - Simply divide total pieces by max pieces per stack
        totalStacks = (totalPieces + maxPiecesPerStack - 1) / maxPiecesPerStack;
    }
    
    const int fullStacks = totalPieces / maxPiecesPerStack;
    
    // Calculate piece size (A3 paper size: 420x297mm)
    const double pieceWidth = 420.0 / piecesInX;
    const double pieceHeight = 297.0 / piecesInY;

    // Build the basic summary text
    QString summaryText = QString(
        "Cutting Configuration:\n\n"
        "• Pieces per surface: %1 × %2\n"
        "• Surface size: 420.0 × 297.0 mm (A3)\n"
        "• Cut piece size: %3 × %4 mm\n"
        "• Stacking method: %5\n"
        "• Total surfaces: %6\n"
        "• Total pieces: %7\n"
        "• Number of stacks: %8\n"
        "• Full stacks (50 pieces): %9\n\n"
        "Defective Pieces by Stack:\n")
        .arg(piecesInX)
        .arg(piecesInY)
        .arg(pieceWidth, 0, 'f', 1)
        .arg(pieceHeight, 0, 'f', 1)
        .arg(useXAxisStacking ? "X-axis" : "Single Stack")
        .arg(totalSurfaces)
        .arg(totalPieces)
        .arg(totalStacks)
        .arg(fullStacks);

    // Add defective pieces information by stack
    QStringList defectiveByStack;
    
    if (useXAxisStacking) {
        // For X-axis stacking:
        // Calculate parameters for stack organization
        int piecesPerSurface = piecesInX;
        int surfacesPerGroup = maxPiecesPerStack / piecesPerSurface;
        
        // Process each surface to find defective pieces
        for (int i = 0; i < surfaceList->topLevelItemCount(); i++) {
            QTreeWidgetItem* item = surfaceList->topLevelItem(i);
            QString surfacePath = QString("%1/%2").arg(sessionPath).arg(item->text(0));
            QString analysisPath = QString("%1/cutting_analysis.json").arg(surfacePath);
            QFile file(analysisPath);
            
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                QJsonObject root = doc.object();
                QJsonArray defectPieces = root["pieces_with_defects"].toArray();
                file.close();

                // Process defective pieces
                for (const QJsonValue &value : defectPieces) {
                    QString pieceId = value.toString();
                    
                    // Extract x and y from pieceId (format: "x1y2")
                    int x = pieceId.mid(1, pieceId.indexOf('y') - 1).toInt();
                    int y = pieceId.mid(pieceId.indexOf('y') + 1).toInt();
                    
                    // Calculate stack number and position
                    int groupIndex = i / surfacesPerGroup;
                    int stackNumber = y + (groupIndex * piecesInY);
                    
                    // Calculate position within stack (x pieces are stacked from bottom)
                    int surfaceInGroup = i % surfacesPerGroup;
                    int positionInStack = surfaceInGroup * piecesInX + (piecesInX - x + 1);
                    
                    // Add to defect list with stack position
                    defectiveByStack.append(QString("Stack %1 z%2 (s%3%4)")
                        .arg(stackNumber)
                        .arg(positionInStack)
                        .arg(i + 1)
                        .arg(pieceId));
                }
            }
        }
    } else {
        // Single stacking implementation
        int currentStack = 1;
        int pieceInStack = 0;

        // Process each surface to find defective pieces
        for (int i = 0; i < surfaceList->topLevelItemCount(); i++) {
            QTreeWidgetItem* item = surfaceList->topLevelItem(i);
            QString surfacePath = QString("%1/%2").arg(sessionPath).arg(item->text(0));
            QString analysisPath = QString("%1/cutting_analysis.json").arg(surfacePath);
            QFile file(analysisPath);
            
            if (file.open(QIODevice::ReadOnly)) {
                QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
                QJsonObject root = doc.object();
                QJsonArray defectPieces = root["pieces_with_defects"].toArray();
                file.close();

                // Process defective pieces
                for (const QJsonValue &value : defectPieces) {
                    QString pieceId = value.toString();
                    int stackPosition = pieceInStack + 1;
                    
                    // Add to defect list with stack position
                    defectiveByStack.append(QString("Stack %1 z%2 (s%3%4)")
                        .arg(currentStack)
                        .arg(stackPosition)
                        .arg(i + 1)
                        .arg(pieceId));

                    pieceInStack++;
                    if (pieceInStack >= maxPiecesPerStack) {
                        currentStack++;
                        pieceInStack = 0;
                    }
                }
            }

            // Update piece counter for non-defective pieces too
            pieceInStack += piecesPerSurface;
            if (pieceInStack >= maxPiecesPerStack) {
                currentStack++;
                pieceInStack = pieceInStack % maxPiecesPerStack;
            }
        }
    }

    // Add defective pieces list to summary
    if (defectiveByStack.isEmpty()) {
        summaryText += "No defective pieces found.";
    } else {
        summaryText += defectiveByStack.join("\n");
    }

    summaryLabel->setText(summaryText);
}

void CuttingWindow::onCuttingPreviewClicked(QPoint pos)
{
    // Get the current pixmap
    QPixmap pixmap = cuttingPreview->pixmap(Qt::ReturnByValue);
    if (pixmap.isNull()) return;

    // Calculate the grid cell size
    int width = pixmap.width();
    int height = pixmap.height();
    int cellWidth = width / piecesInX;
    int cellHeight = height / piecesInY;

    // Calculate which piece was clicked (1-based indexing)
    int pieceX = (pos.x() / cellWidth) + 1;
    int pieceY = (pos.y() / cellHeight) + 1;

    // Ensure we're within bounds
    if (pieceX > 0 && pieceX <= piecesInX && pieceY > 0 && pieceY <= piecesInY) {
        showPieceDefects(pieceX, pieceY);
    }
}

void CuttingWindow::showPieceDefects(int pieceX, int pieceY)
{
    // Get current surface path
    QTreeWidgetItem *currentItem = surfaceList->currentItem();
    if (!currentItem) return;
    QString surfacePath = QString("%1/%2").arg(sessionPath).arg(currentItem->text(0));
    
    // Load cutting analysis file
    QString analysisPath = QString("%1/cutting_analysis.json").arg(surfacePath);
    QFile file(analysisPath);
    
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();
    QJsonArray pieces = root["pieces"].toArray();
    
    // Find the selected piece
    for (const QJsonValue &pieceValue : pieces) {
        QJsonObject piece = pieceValue.toObject();
        if (piece["x"].toInt() == pieceX && piece["y"].toInt() == pieceY) {
            QJsonArray defects = piece["defects"].toArray();
            
            // Update defect table with only this piece's defects
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

                // Physical position
                QJsonObject physicalPos = defect["physical_position"].toObject();
                
                // Location (x, y) in mm
                QString location = QString("(%1, %2) mm")
                    .arg(physicalPos["x"].toDouble(), 0, 'f', 1)
                    .arg(physicalPos["y"].toDouble(), 0, 'f', 1);
                QTableWidgetItem *locItem = new QTableWidgetItem(location);
                locItem->setTextAlignment(Qt::AlignCenter);
                defectTable->setItem(i, 3, locItem);

                // Size (width × height) in mm
                QString size = QString("%1 × %2 mm")
                    .arg(physicalPos["width"].toDouble(), 0, 'f', 1)
                    .arg(physicalPos["height"].toDouble(), 0, 'f', 1);
                QTableWidgetItem *sizeItem = new QTableWidgetItem(size);
                sizeItem->setTextAlignment(Qt::AlignCenter);
                defectTable->setItem(i, 4, sizeItem);
            }
            break;
        }
    }
    file.close();
} 