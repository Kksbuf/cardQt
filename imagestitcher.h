#ifndef IMAGESTITCHER_H
#define IMAGESTITCHER_H

#include <QObject>
#include <QString>
#include <QVector>
#include <QImage>
#include <QMap>
#include <QPair>
#include <QJsonArray>
#include <QJsonObject>

class ImageStitcher : public QObject
{
    Q_OBJECT

public:
    ImageStitcher(const QString &surfacePath,
                 int imagesInX, int imagesInY,
                 const QVector<int> &sequence,
                 double actualWidth, double actualHeight);
    
    bool stitchImages();
    void labelDefects();

signals:
    void finished();

private:
    QString surfacePath;
    int imagesInX;
    int imagesInY;
    QVector<int> sequence;
    double actualWidth;
    double actualHeight;
    QJsonArray defectCoordinates;  // Store all defect coordinates
    
    QImage cropCenterRegion(const QImage &source);
    QPoint getImagePosition(int sequenceIndex, int canvasWidth, int canvasHeight);
    QMap<int, QPair<int, int>> createSequencePositionMap();
    void saveDefectCoordinates(const QString &imageName, int seqNum, 
                             const QString &defectType, float confidence,
                             const QRectF &canvasRect, const QImage &stitchedImage);
    void saveAllDefectCoordinates();  // New method to save all coordinates at once
};

#endif // IMAGESTITCHER_H 