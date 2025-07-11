#ifndef CUTTINGWINDOW_H
#define CUTTINGWINDOW_H

#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeWidget>
#include <QTableWidget>
#include <QGroupBox>
#include <QPushButton>
#include <QScrollArea>
#include <QMouseEvent>
#include "cuttinganalyzer.h"
#include <QFrame>
#include <QSpacerItem>
#include <QSizePolicy>

// Custom QLabel for handling mouse events
class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr) : QLabel(parent) {}

signals:
    void clicked(QPoint pos);

protected:
    void mousePressEvent(QMouseEvent *event) override {
        emit clicked(event->pos());
    }
};

// Custom widget for displaying a piece in the stack
class StackPieceWidget : public QFrame {
    Q_OBJECT
public:
    StackPieceWidget(int surfaceNum, const QString& pieceId, bool hasDefect, QWidget* parent = nullptr) : QFrame(parent) {
        setFixedSize(100, 20); // More compact size
        setFrameStyle(QFrame::Box | QFrame::Plain);
        setLineWidth(1);
        
        // Create layout
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(2, 0, 2, 0);
        layout->setSpacing(1);
        
        // Create label with compact text
        QString displayText = QString("s%1%2%3")
            .arg(surfaceNum)
            .arg(pieceId)
            .arg(hasDefect ? "D" : "");
        QLabel* label = new QLabel(displayText, this);
        label->setAlignment(Qt::AlignCenter);
        label->setStyleSheet("QLabel { font-size: 8pt; }");
        
        // Set style based on defect status
        QString styleSheet;
        if (hasDefect) {
            styleSheet = "QFrame { background-color: #ffcccc; border: 1px solid red; }";
            label->setStyleSheet("QLabel { color: red; font-size: 8pt; }");
        } else {
            styleSheet = "QFrame { background-color: #f5deb3; border: 1px solid black; }";
            label->setStyleSheet("QLabel { font-size: 8pt; }");
        }
        setStyleSheet(styleSheet);
        
        layout->addWidget(label);
    }
};

// Custom widget for displaying a stack
class StackWidget : public QFrame {
    Q_OBJECT
public:
    StackWidget(const QString& label, int capacity, QWidget* parent = nullptr) : QFrame(parent), maxCapacity(capacity) {
        setFrameStyle(QFrame::Box | QFrame::Raised);
        setLineWidth(1);
        setStyleSheet("StackWidget { background-color: white; }");
        setFixedWidth(110);
        
        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setSpacing(0);
        layout->setContentsMargins(2, 2, 2, 2);
        
        // Add stack label at top
        QLabel* titleLabel = new QLabel(label, this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("QLabel { font-weight: bold; font-size: 8pt; padding: 1px; background-color: #e0e0e0; border-radius: 2px; }");
        layout->addWidget(titleLabel);
        
        // Add piece counter below title
        pieceCount = new QLabel("0/50 pieces", this);
        pieceCount->setAlignment(Qt::AlignCenter);
        pieceCount->setStyleSheet("QLabel { font-size: 7pt; padding: 1px; background-color: #f0f0f0; border-radius: 2px; }");
        layout->addWidget(pieceCount);
        
        // Container for pieces with bottom alignment
        pieceContainer = new QWidget(this);
        pieceLayout = new QVBoxLayout(pieceContainer);
        pieceLayout->setSpacing(0);
        pieceLayout->setContentsMargins(1, 1, 1, 1);
        pieceLayout->setAlignment(Qt::AlignBottom);
        
        // Add spacer to push pieces to bottom
        QSpacerItem* spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        pieceLayout->addItem(spacer);
        
        layout->addWidget(pieceContainer);
    }
    
    void addPiece(int surfaceNum, const QString& pieceId, bool hasDefect) {
        if (pieces.size() >= maxCapacity) return;
        
        // Create new piece
        StackPieceWidget* piece = new StackPieceWidget(surfaceNum, pieceId, hasDefect, this);
        pieces.append(piece);
        
        // Always add after the spacer (which is at index 0)
        pieceLayout->insertWidget(1, piece);
        
        // Update count
        pieceCount->setText(QString("%1/%2 pieces").arg(pieces.size()).arg(maxCapacity));
    }
    
    void clear() {
        qDeleteAll(pieces);
        pieces.clear();
        pieceCount->setText(QString("0/%1 pieces").arg(maxCapacity));
    }
    
private:
    QWidget* pieceContainer;
    QVBoxLayout* pieceLayout;
    QLabel* pieceCount;
    QList<StackPieceWidget*> pieces;
    int maxCapacity;
};

class CuttingWindow : public QDialog
{
    Q_OBJECT

public:
    CuttingWindow(QWidget *parent, 
                 const QString &sessionPath,
                 int piecesInX,
                 int piecesInY,
                 bool useXAxisStacking);
    ~CuttingWindow();
    
    // Make this public so it can be called after configuration is confirmed
    void performCuttingAnalysis();

private slots:
    void onSurfaceSelectionChanged();
    void updateDefectPreview(const QString &surfacePath);
    void onPreviousSurface();
    void onNextSurface();
    void updateNavigationButtons();
    void onCuttingPreviewClicked(QPoint pos);
    void showPieceDefects(int pieceX, int pieceY);

private:
    void setupUI();
    void setupSurfaceColumn();
    void setupDefectColumn();
    void setupStackingColumn();
    void setupSummaryColumn();
    void loadSurfaces();
    void drawCuttingGrid(QLabel *label, const QPixmap &baseImage);
    void updateStackPreview();
    void updateSummaryText();

    QString sessionPath;
    int piecesInX;
    int piecesInY;
    bool useXAxisStacking;
    CuttingAnalyzer *analyzer;

    // UI Components
    QTreeWidget *surfaceList;
    QLabel *surfacePreview;
    QLabel *defectPreview;
    ClickableLabel *cuttingPreview;  // Changed to ClickableLabel
    QTableWidget *defectTable;
    QLabel *stackPreview;
    QLabel *summaryLabel;
    QPushButton *prevButton;
    QPushButton *nextButton;
    QLabel *currentSurfaceLabel;

    // Layout components
    QHBoxLayout *mainLayout;
    QVBoxLayout *surfaceColumn;
    QVBoxLayout *defectColumn;
    QVBoxLayout *stackColumn;
    QVBoxLayout *summaryColumn;

    QScrollArea* stackScroll;
    QWidget* stackContainer;
    QHBoxLayout* stackGrid;
};

#endif // CUTTINGWINDOW_H 