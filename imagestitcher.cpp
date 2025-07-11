#include "imagestitcher.h"
#include <QDir>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

ImageStitcher::ImageStitcher(const QString &surfacePath,
                           int imagesInX, int imagesInY,
                           const QVector<int> &sequence,
                           double actualWidth, double actualHeight)
    : QObject(nullptr)  // Add parent initialization
    , surfacePath(surfacePath)
    , imagesInX(imagesInX)
    , imagesInY(imagesInY)
    , sequence(sequence)
    , actualWidth(actualWidth)
    , actualHeight(actualHeight)
{
}

bool ImageStitcher::stitchImages()
{
    // Calculate canvas size maintaining aspect ratio
    const int baseWidth = 2910;  // 3 * 970 for 3x3 grid
    const double aspectRatio = actualWidth / actualHeight;
    const int canvasWidth = baseWidth;
    const int canvasHeight = static_cast<int>(baseWidth / aspectRatio);
    
    // Create canvas
    QImage canvas(canvasWidth, canvasHeight, QImage::Format_RGB888);
    canvas.fill(Qt::black);
    
    QPainter painter(&canvas);
    
    // Process each position in the sequence
    for (int position = 0; position < sequence.size(); ++position) {
        // sequence[position] tells us which image number goes at this position
        int imageNumber = sequence[position];  // This is the image number we want at this position
        
        // Load the image
        QString imagePath = QString("%1/image_%2.jpg").arg(surfacePath).arg(imageNumber, 2, 10, QChar('0'));
        
        QImage img(imagePath);
        if (img.isNull()) continue;
        
        // Crop center region
        QImage croppedImg = cropCenterRegion(img);
        
        // Calculate position based on sequence position
        QPoint imagePos = getImagePosition(position, canvasWidth, canvasHeight);
        
        // Draw image
        painter.drawImage(imagePos, croppedImg);
    }
    
    painter.end();
    
    // Save stitched image
    bool success = canvas.save(QString("%1/stitched.jpg").arg(surfacePath), "JPG", 100);
    
    // Emit finished signal
    emit finished();
    
    return success;
}

QImage ImageStitcher::cropCenterRegion(const QImage &source)
{
    const int cropWidth = 970;
    const int cropHeight = 686;
    
    // Calculate center position
    int x = (source.width() - cropWidth) / 2;
    int y = (source.height() - cropHeight) / 2;
    
    // Ensure valid crop region
    if (x < 0 || y < 0 || x + cropWidth > source.width() || y + cropHeight > source.height()) {
        return source.scaled(cropWidth, cropHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    return source.copy(x, y, cropWidth, cropHeight);
}

QPoint ImageStitcher::getImagePosition(int sequencePosition, int canvasWidth, int canvasHeight)
{
    // Calculate grid cell size
    int cellWidth = canvasWidth / imagesInX;
    int cellHeight = canvasHeight / imagesInY;
    
    // For sequence [3,4,9,2,5,8,1,6,7]
    // Position 0 -> top-left (0,0)
    // Position 1 -> top-middle (1,0)
    // Position 2 -> top-right (2,0)
    // Position 3 -> middle-left (0,1)
    // etc.
    
    // Calculate row and column
    int row = sequencePosition / imagesInX;    // Integer division for row number
    int col = sequencePosition % imagesInX;    // Remainder for column number
    
    // Convert to pixel coordinates
    int x = col * cellWidth;
    int y = row * cellHeight;
    
    return QPoint(x, y);
}

void ImageStitcher::labelDefects()
{
    // Load the stitched image
    QString stitchedPath = QString("%1/stitched.jpg").arg(surfacePath);
    QImage stitchedImage(stitchedPath);
    if (stitchedImage.isNull()) {
        qDebug() << "Failed to load stitched image for labeling";
        return;
    }

    // Create labeled copy of stitched image
    QImage labeledImage = stitchedImage.copy();
    QPainter labeledPainter(&labeledImage);

    // Clear any existing defect coordinates
    defectCoordinates = QJsonArray();

    // Create sequence to position map
    QMap<int, QPair<int, int>> seqToPos = createSequencePositionMap();

    // Process each image in sequence
    for (int i = 0; i < sequence.size(); ++i) {
        int imageNumber = sequence[i];
        QString imagePath = QString("%1/image_%2.jpg").arg(surfacePath).arg(imageNumber, 2, 10, QChar('0'));
        QString detectionPath = imagePath;
        detectionPath.replace(".jpg", "_detections.json");

        // Get grid position for this sequence number
        auto pos = seqToPos[i + 1]; // sequence is 1-based
        int gridX = pos.first * 970;
        int gridY = pos.second * 686;

        // Load and process detections
        QFile file(detectionPath);
        if (file.open(QIODevice::ReadOnly)) {
            QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
            QJsonArray detections = doc.object()["detections"].toArray();

            for (const QJsonValue &val : detections) {
                QJsonObject detection = val.toObject();
                QString defectType = detection["class_name"].toString();
                float confidence = detection["confidence"].toDouble();
                
                // Get original coordinates
                float orig_x = detection["center_x"].toDouble();
                float orig_y = detection["center_y"].toDouble();
                float orig_w = detection["width"].toDouble();
                float orig_h = detection["height"].toDouble();

                // Transform coordinates to canvas position
                float canvas_x = gridX + (orig_x - (1920 - 970) / 2);
                float canvas_y = gridY + (orig_y - (1080 - 686) / 2);

                // Set color based on defect type
                QColor color;
                if (defectType == "damage") color = QColor(255, 0, 0, 128);
                else if (defectType == "mark") color = QColor(0, 255, 0, 128);
                else if (defectType == "oil") color = QColor(0, 0, 255, 128);
                else if (defectType == "edge") color = QColor(255, 165, 0, 128);
                else color = QColor(128, 128, 128, 128);

                // Draw defect rectangle
                labeledPainter.setPen(QPen(color, 2));
                labeledPainter.setBrush(color);

                QRectF defectRect(
                    canvas_x - orig_w/2,
                    canvas_y - orig_h/2,
                    orig_w,
                    orig_h
                );
                labeledPainter.drawRect(defectRect);

                // Draw label
                labeledPainter.setPen(Qt::white);
                labeledPainter.setFont(QFont("Arial", 10));
                QString label = QString("%1\n%2%")
                    .arg(defectType)
                    .arg(qRound(confidence * 100));
                labeledPainter.drawText(defectRect, Qt::AlignCenter, label);

                // Save defect coordinates
                QString imageName = QFileInfo(imagePath).fileName();
                saveDefectCoordinates(imageName, imageNumber, defectType, confidence, 
                                   defectRect, stitchedImage);
            }
            file.close();
        }
    }

    labeledPainter.end();

    // Save all defect coordinates to JSON file
    saveAllDefectCoordinates();

    // Save labeled stitched image
    QString labeledPath = QString("%1/stitched_labeled.jpg").arg(surfacePath);
    if (!labeledImage.save(labeledPath)) {
        qDebug() << "Failed to save labeled stitched image";
    }
}

QMap<int, QPair<int, int>> ImageStitcher::createSequencePositionMap()
{
    QMap<int, QPair<int, int>> seqToPos;
    for (int i = 0; i < sequence.size(); ++i) {
        int row = i / imagesInX;
        int col = i % imagesInX;
        seqToPos[i + 1] = qMakePair(col, row);
    }
    return seqToPos;
}

void ImageStitcher::saveDefectCoordinates(const QString &imageName, int seqNum,
                                        const QString &defectType, float confidence,
                                        const QRectF &canvasRect, const QImage &stitchedImage)
{
    // Calculate physical dimensions
    float physicalX = canvasRect.center().x() * actualWidth / stitchedImage.width();
    float physicalY = canvasRect.center().y() * actualHeight / stitchedImage.height();
    float physicalW = canvasRect.width() * actualWidth / stitchedImage.width();
    float physicalH = canvasRect.height() * actualHeight / stitchedImage.height();

    // Create JSON object for this defect
    QJsonObject defect;
    defect["source_image"] = imageName;
    defect["sequence_number"] = seqNum;
    defect["type"] = defectType;
    defect["confidence"] = confidence;
    
    // Canvas coordinates (pixels)
    QJsonObject canvasPos;
    canvasPos["x"] = canvasRect.center().x();
    canvasPos["y"] = canvasRect.center().y();
    canvasPos["width"] = canvasRect.width();
    canvasPos["height"] = canvasRect.height();
    defect["canvas_position"] = canvasPos;
    
    // Physical coordinates (mm)
    QJsonObject physicalPos;
    physicalPos["x"] = physicalX;
    physicalPos["y"] = physicalY;
    physicalPos["width"] = physicalW;
    physicalPos["height"] = physicalH;
    defect["physical_position"] = physicalPos;

    // Add to array of defects
    defectCoordinates.append(defect);
}

void ImageStitcher::saveAllDefectCoordinates()
{
    // Create the main JSON object
    QJsonObject mainObject;
    mainObject["surface_width"] = actualWidth;
    mainObject["surface_height"] = actualHeight;
    mainObject["grid_x"] = imagesInX;
    mainObject["grid_y"] = imagesInY;
    
    // Convert sequence to QVariantList
    QVariantList variantList;
    for (int value : sequence) {
        variantList.append(QVariant(value));
    }
    mainObject["sequence"] = QJsonArray::fromVariantList(variantList);
    
    mainObject["defects"] = defectCoordinates;

    // Save to file
    QJsonDocument doc(mainObject);
    QString coordPath = QString("%1/defect_coordinates.json").arg(surfacePath);
    QFile coordFile(coordPath);
    
    if (coordFile.open(QIODevice::WriteOnly)) {
        coordFile.write(doc.toJson(QJsonDocument::Indented));
        coordFile.close();
        qDebug() << "Successfully saved defect coordinates to JSON file";
    } else {
        qDebug() << "Failed to save defect coordinates to JSON file";
    }
} 