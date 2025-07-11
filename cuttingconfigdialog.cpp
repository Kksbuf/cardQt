#include "cuttingconfigdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>

CuttingConfigDialog::CuttingConfigDialog(QWidget *parent,
                                       double surfaceWidth,
                                       double surfaceHeight,
                                       double captureWidth,
                                       double captureHeight,
                                       int surfaceCount)
    : QDialog(parent),
      surfaceWidth(surfaceWidth),
      surfaceHeight(surfaceHeight),
      captureWidth(captureWidth),
      captureHeight(captureHeight),
      surfaceCount(surfaceCount)
{
    setWindowTitle("Cutting Configuration");
    setupUI();
}

void CuttingConfigDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create info group
    QGroupBox *infoGroup = new QGroupBox(this);
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    // Surface size
    surfaceSizeLabel = new QLabel(QString("Surface size: %1 x %2 mm")
        .arg(surfaceWidth, 0, 'f', 1)
        .arg(surfaceHeight, 0, 'f', 1));
    infoLayout->addWidget(surfaceSizeLabel);
    
    // Capture area
    captureAreaLabel = new QLabel(QString("Capture area: %1 x %2 mm")
        .arg(captureWidth, 0, 'f', 1)
        .arg(captureHeight, 0, 'f', 1));
    infoLayout->addWidget(captureAreaLabel);
    
    // Number of surfaces
    surfaceCountLabel = new QLabel(QString("Number of surfaces: %1")
        .arg(surfaceCount));
    infoLayout->addWidget(surfaceCountLabel);
    
    mainLayout->addWidget(infoGroup);
    
    // Create cutting configuration group
    QGroupBox *configGroup = new QGroupBox(this);
    QVBoxLayout *configLayout = new QVBoxLayout(configGroup);
    
    // Pieces in X and Y
    QHBoxLayout *piecesLayout = new QHBoxLayout();
    
    QLabel *piecesXLabel = new QLabel("Pieces in X:", this);
    piecesInXSpinBox = new QSpinBox(this);
    piecesInXSpinBox->setMinimum(1);
    piecesInXSpinBox->setMaximum(10);
    piecesInXSpinBox->setValue(4);
    piecesLayout->addWidget(piecesXLabel);
    piecesLayout->addWidget(piecesInXSpinBox);
    
    piecesLayout->addSpacing(20);
    
    QLabel *piecesYLabel = new QLabel("Pieces in Y:", this);
    piecesInYSpinBox = new QSpinBox(this);
    piecesInYSpinBox->setMinimum(1);
    piecesInYSpinBox->setMaximum(10);
    piecesInYSpinBox->setValue(2);
    piecesLayout->addWidget(piecesYLabel);
    piecesLayout->addWidget(piecesInYSpinBox);
    
    configLayout->addLayout(piecesLayout);
    
    // Total pieces info
    totalPiecesLabel = new QLabel(this);
    totalPiecesAllLabel = new QLabel(this);
    configLayout->addWidget(totalPiecesLabel);
    configLayout->addWidget(totalPiecesAllLabel);
    
    mainLayout->addWidget(configGroup);
    
    // Create stacking method group
    QGroupBox *stackingGroup = new QGroupBox("Stacking Method", this);
    QVBoxLayout *stackingLayout = new QVBoxLayout(stackingGroup);
    
    xAxisStackingRadio = new QRadioButton("X-axis Stacking", this);
    singleStackRadio = new QRadioButton("Single Stack", this);
    xAxisStackingRadio->setChecked(true);
    
    stackingLayout->addWidget(xAxisStackingRadio);
    stackingLayout->addWidget(singleStackRadio);
    
    mainLayout->addWidget(stackingGroup);
    
    // Create button box
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        Qt::Horizontal,
        this);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
    
    // Connect signals
    connect(piecesInXSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CuttingConfigDialog::onPiecesChanged);
    connect(piecesInYSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CuttingConfigDialog::onPiecesChanged);
    
    // Initial update
    onPiecesChanged();
}

void CuttingConfigDialog::onPiecesChanged()
{
    int piecesPerSurface = piecesInXSpinBox->value() * piecesInYSpinBox->value();
    int totalPieces = piecesPerSurface * surfaceCount;
    
    totalPiecesLabel->setText(QString("Total pieces per surface: %1")
        .arg(piecesPerSurface));
    totalPiecesAllLabel->setText(QString("Total pieces across all surfaces: %1")
        .arg(totalPieces));
} 