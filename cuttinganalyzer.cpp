#include "cuttinganalyzer.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>
#include <QPointF>

CuttingAnalyzer::CuttingAnalyzer(const QString &sessionPath,
                               int piecesInX,
                               int piecesInY,
                               double surfaceWidth,
                               double surfaceHeight)
    : sessionPath(sessionPath),
      piecesInX(piecesInX),
      piecesInY(piecesInY),
      surfaceWidth(surfaceWidth),
      surfaceHeight(surfaceHeight)
{
    qDebug() << "Initializing CuttingAnalyzer:";
    qDebug() << "  Session path:" << sessionPath;
    qDebug() << "  Pieces:" << piecesInX << "x" << piecesInY;
    qDebug() << "  Surface dimensions:" << surfaceWidth << "x" << surfaceHeight << "mm";

    // Calculate piece dimensions
    pieceWidth = surfaceWidth / piecesInX;
    pieceHeight = surfaceHeight / piecesInY;
    qDebug() << "  Piece dimensions:" << pieceWidth << "x" << pieceHeight << "mm";

    // Initialize pieces vector with correct dimensions
    pieces.resize(piecesInX);
    for (int x = 0; x < piecesInX; ++x) {
        pieces[x].resize(piecesInY);
        for (int y = 0; y < piecesInY; ++y) {
            pieces[x][y].x = x + 1;  // 1-based indexing
            pieces[x][y].y = y + 1;  // 1-based indexing
        }
    }
}

bool CuttingAnalyzer::analyzeSurfaces()
{
    qDebug() << "\nAnalyzing surface at path:" << sessionPath;
    
    // Clear any previous analysis
    for (auto &column : pieces) {
        for (auto &piece : column) {
            piece.defects.clear();
        }
    }
    outsideDefects = QJsonArray();
    
    // Analyze the surface
    if (!analyzeDefectsInSurface(sessionPath, 1)) {
        qWarning() << "Failed to analyze defects in surface";
        return false;
    }

    // Save analysis for this surface
    QString analysisPath = QString("%1/cutting_analysis.json").arg(sessionPath);
    qDebug() << "\nSaving analysis to:" << analysisPath;
    
    // Extract surface name from path
    QString surfaceName = QDir(sessionPath).dirName();
    
    if (!saveAnalysis(analysisPath, surfaceName)) {
        qWarning() << "Failed to save cutting analysis";
        return false;
    }
    qDebug() << "Successfully saved analysis";

    return true;
}

bool CuttingAnalyzer::analyzeDefectsInSurface(const QString &surfacePath, int surfaceIndex)
{
    QString coordFile = QString("%1/defect_coordinates.json").arg(surfacePath);
    qDebug() << "Reading defect coordinates from:" << coordFile;
    
    QFile file(coordFile);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Could not open defect coordinates file:" << coordFile;
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) {
        qWarning() << "Invalid JSON format in file:" << coordFile;
        return false;
    }

    QJsonArray defects = doc.object()["defects"].toArray();
    qDebug() << "Found" << defects.size() << "defects in surface" << surfaceIndex;

    for (const QJsonValue &defectValue : defects) {
        QJsonObject defect = defectValue.toObject();
        QJsonObject physicalPos = defect["physical_position"].toObject();

        // Get defect position and size
        double x = physicalPos["x"].toDouble();
        double y = physicalPos["y"].toDouble();
        double width = physicalPos["width"].toDouble();
        double height = physicalPos["height"].toDouble();

        qDebug() << "Processing defect at:" << x << "," << y << "with size:" << width << "x" << height << "mm";

        // Add surface index to the defect object
        QJsonObject enrichedDefect = defect;
        enrichedDefect["surface_index"] = surfaceIndex;

        // Check all corners of the defect to determine affected pieces
        QVector<QPair<int, int>> affectedPieces;
        QVector<QPointF> corners;
        corners.append(QPointF(x, y));                       // Top-left
        corners.append(QPointF(x + width, y));               // Top-right
        corners.append(QPointF(x, y + height));              // Bottom-left
        corners.append(QPointF(x + width, y + height));      // Bottom-right
        corners.append(QPointF(x + width/2, y + height/2));  // Center

        bool isOutside = true;
        for (const QPointF &corner : corners) {
            // Skip if point is outside surface bounds
            if (corner.x() < 0 || corner.x() > surfaceWidth ||
                corner.y() < 0 || corner.y() > surfaceHeight) {
                continue;
            }

            // Calculate which piece this point belongs to
            int pieceX = qMin(static_cast<int>(corner.x() / pieceWidth) + 1, piecesInX);
            int pieceY = qMin(static_cast<int>(corner.y() / pieceHeight) + 1, piecesInY);

            // Add to affected pieces if not already included
            QPair<int, int> piece(pieceX, pieceY);
            if (!affectedPieces.contains(piece)) {
                affectedPieces.append(piece);
                isOutside = false;
                qDebug() << "  Affects piece:" << pieceX << "," << pieceY;
            }
        }

        if (isOutside) {
            qDebug() << "  Defect is outside surface bounds";
            outsideDefects.append(enrichedDefect);
        } else {
            addDefectToPieces(enrichedDefect, affectedPieces);
        }
    }

    return true;
}

void CuttingAnalyzer::addDefectToPieces(const QJsonObject &defect, const QVector<QPair<int, int>> &affectedPieces)
{
    for (const auto &piece : affectedPieces) {
        int x = piece.first - 1;   // Convert to 0-based index
        int y = piece.second - 1;  // Convert to 0-based index
        if (x >= 0 && x < piecesInX && y >= 0 && y < piecesInY) {
            pieces[x][y].defects.append(defect);
        }
    }
}

bool CuttingAnalyzer::saveAnalysis(const QString &outputPath, const QString &surfaceName)
{
    qDebug() << "\n=== Starting Cutting Analysis Save Process ===";
    qDebug() << "Output path:" << outputPath;
    qDebug() << "Surface name:" << surfaceName;
    
    QJsonObject root;
    
    // Add metadata
    qDebug() << "Creating metadata...";
    QJsonObject metadata;
    metadata["surface_name"] = surfaceName;
    metadata["pieces_in_x"] = piecesInX;
    metadata["pieces_in_y"] = piecesInY;
    metadata["surface_width"] = surfaceWidth;
    metadata["surface_height"] = surfaceHeight;
    metadata["piece_width"] = pieceWidth;
    metadata["piece_height"] = pieceHeight;
    root["metadata"] = metadata;
    qDebug() << "Metadata created:" << metadata;

    // Add pieces data
    qDebug() << "Processing pieces data...";
    QJsonArray piecesArray;
    QJsonArray piecesWithDefects;  // New array to track pieces with defects
    int totalPieces = 0;
    int piecesWithDefectsCount = 0;
    
    for (int x = 0; x < piecesInX; ++x) {
        for (int y = 0; y < piecesInY; ++y) {
            totalPieces++;
            QJsonObject pieceObj;
            pieceObj["x"] = pieces[x][y].x;
            pieceObj["y"] = pieces[x][y].y;
            
            // Convert defects to JSON array
            QJsonArray defectsArray;
            for (const QJsonObject &defect : pieces[x][y].defects) {
                defectsArray.append(defect);
            }
            pieceObj["defects"] = defectsArray;
            
            // If this piece has defects, add it to the tracking array
            if (!pieces[x][y].defects.isEmpty()) {
                QString pieceId = QString("x%1y%2").arg(pieces[x][y].x).arg(pieces[x][y].y);
                piecesWithDefects.append(pieceId);
                piecesWithDefectsCount++;
                qDebug() << "Found piece with defects:" << pieceId 
                        << "(" << pieces[x][y].defects.size() << "defects)";
            }
            
            piecesArray.append(pieceObj);
        }
    }
    root["pieces"] = piecesArray;
    root["pieces_with_defects"] = piecesWithDefects;
    root["outside_defects"] = outsideDefects;

    qDebug() << "Pieces summary:";
    qDebug() << "- Total pieces:" << totalPieces;
    qDebug() << "- Pieces with defects:" << piecesWithDefectsCount;
    qDebug() << "- Outside defects:" << outsideDefects.size();

    // Write to file
    qDebug() << "Opening file for writing:" << outputPath;
    QFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Could not open output file for writing:" << outputPath;
        qWarning() << "Error:" << file.errorString();
        return false;
    }

    QJsonDocument doc(root);
    qint64 bytesWritten = file.write(doc.toJson(QJsonDocument::Indented));  // Use indented format for better readability
    file.close();

    if (bytesWritten <= 0) {
        qWarning() << "Failed to write data to file:" << outputPath;
        qWarning() << "Error:" << file.errorString();
        return false;
    }

    qDebug() << "Successfully wrote" << bytesWritten << "bytes to" << outputPath;
    qDebug() << "=== Cutting Analysis Save Process Complete ===\n";
    return true;
} 