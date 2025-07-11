#ifndef CUTTINGANALYZER_H
#define CUTTINGANALYZER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVector>
#include <QPair>

struct CutPiece {
    int x;  // x-index of the piece (1-based)
    int y;  // y-index of the piece (1-based)
    QVector<QJsonObject> defects;  // defects located in this piece
};

class CuttingAnalyzer {
public:
    CuttingAnalyzer(const QString &sessionPath,
                    int piecesInX,
                    int piecesInY,
                    double surfaceWidth,
                    double surfaceHeight);

    bool analyzeSurfaces();
    bool saveAnalysis(const QString &outputPath, const QString &surfaceName);

private:
    bool analyzeDefectsInSurface(const QString &surfacePath, int surfaceIndex);
    bool isDefectInPiece(const QJsonObject &defect, int pieceX, int pieceY);
    bool isPointInPiece(double x, double y, int pieceX, int pieceY);
    void addDefectToPieces(const QJsonObject &defect, const QVector<QPair<int, int>> &affectedPieces);

    QString sessionPath;
    int piecesInX;
    int piecesInY;
    double surfaceWidth;
    double surfaceHeight;
    double pieceWidth;
    double pieceHeight;
    QVector<QVector<CutPiece>> pieces;  // 2D vector of pieces [x][y]
    QJsonArray outsideDefects;  // defects that fall outside the surface
};

#endif // CUTTINGANALYZER_H 