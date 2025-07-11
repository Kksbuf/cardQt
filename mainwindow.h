#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QTreeWidget>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QMessageBox>
#include <QTextEdit>
#include <QFileSystemWatcher>
#include "capturesettingsdialog.h"
#include "capturewindow.h"
#include "defectdetector.h"
#include "imagestitcher.h"

struct Dimensions {
    double actualWidth;
    double actualHeight;
    double capturedWidth;
    double capturedHeight;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr, const QString &sessionPath = QString());
    ~MainWindow();

private slots:
    void onAddNewSurface();
    void onDeleteSurface();
    void onCutSurface();
    void onItemSelectionChanged();
    void onItemExpanded(QTreeWidgetItem* item);
    void onItemCollapsed(QTreeWidgetItem* item);
    void onModelStatusMessage(const QString &message);
    void onModelInitComplete();
    void onModelInitFailed(const QString &error);
    void onDetectionComplete(const QStringList &results);

private:
    void setupUI();
    void setupTopBar();
    void setupMainArea();
    void setupDebugArea();
    void loadDimensions();
    void saveDimensions();
    void loadSurfaces();
    void loadSurfaceImages(QTreeWidgetItem* surfaceItem);
    void updatePreviewImage(const QString& imagePath);
    bool isSurfaceItem(QTreeWidgetItem* item) const;
    void initializeDefectDetector();
    void updateImageStatus(const QString &imagePath, const QString &status, int defectCount = -1);
    QTreeWidgetItem* findImageItem(const QString &imagePath);
    
    QString sessionPath;
    Dimensions dimensions;
    
    // UI Components
    QWidget *centralWidget;
    QTreeWidget *surfaceTree;
    QLabel *originalImageLabel;
    QLabel *defectImageLabel;
    QTableWidget *defectTable;
    QLabel *dimensionsLabel;
    QTextEdit *debugOutput;
    
    // Buttons
    QPushButton *addSurfaceButton;
    QPushButton *deleteSurfaceButton;
    QPushButton *cutSurfaceButton;

    // Defect detector
    DefectDetector *defectDetector;
    ImageStitcher *currentStitcher;

    // Capture settings
    struct CaptureSettings {
        int imagesInX;
        int imagesInY;
        QVector<int> sequence;
    };
    CaptureSettings currentCaptureSettings;

    QFileSystemWatcher *defectWatcher = nullptr;
};

#endif // MAINWINDOW_H