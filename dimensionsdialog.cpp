#include "dimensionsdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>

DimensionsDialog::DimensionsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    setWindowTitle("Surface Dimensions");
    setModal(true);
}

void DimensionsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Actual Surface Dimensions group
    QGroupBox *actualGroup = new QGroupBox("Actual Surface Dimensions");
    QGridLayout *actualLayout = new QGridLayout;
    
    actualWidthSpinBox = new QDoubleSpinBox;
    actualHeightSpinBox = new QDoubleSpinBox;
    
    // Configure spin boxes
    actualWidthSpinBox->setRange(0, 10000);
    actualHeightSpinBox->setRange(0, 10000);
    actualWidthSpinBox->setValue(420.0);
    actualHeightSpinBox->setValue(297.0);
    actualWidthSpinBox->setSuffix(" mm");
    actualHeightSpinBox->setSuffix(" mm");
    
    actualLayout->addWidget(new QLabel("Width:"), 0, 0);
    actualLayout->addWidget(actualWidthSpinBox, 0, 1);
    actualLayout->addWidget(new QLabel("Height:"), 1, 0);
    actualLayout->addWidget(actualHeightSpinBox, 1, 1);
    actualGroup->setLayout(actualLayout);
    
    // Captured Image Area group
    QGroupBox *capturedGroup = new QGroupBox("Captured Image Area");
    QGridLayout *capturedLayout = new QGridLayout;
    
    capturedWidthSpinBox = new QDoubleSpinBox;
    capturedHeightSpinBox = new QDoubleSpinBox;
    
    // Configure spin boxes
    capturedWidthSpinBox->setRange(0, 10000);
    capturedHeightSpinBox->setRange(0, 10000);
    capturedWidthSpinBox->setValue(140.0);
    capturedHeightSpinBox->setValue(99.0);
    capturedWidthSpinBox->setSuffix(" mm");
    capturedHeightSpinBox->setSuffix(" mm");
    
    capturedLayout->addWidget(new QLabel("Width:"), 0, 0);
    capturedLayout->addWidget(capturedWidthSpinBox, 0, 1);
    capturedLayout->addWidget(new QLabel("Height:"), 1, 0);
    capturedLayout->addWidget(capturedHeightSpinBox, 1, 1);
    capturedGroup->setLayout(capturedLayout);
    
    // Add groups to main layout
    mainLayout->addWidget(actualGroup);
    mainLayout->addWidget(capturedGroup);
    
    // Add buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void DimensionsDialog::setDimensions(double actualWidth, double actualHeight, 
                                   double capturedWidth, double capturedHeight)
{
    actualWidthSpinBox->setValue(actualWidth);
    actualHeightSpinBox->setValue(actualHeight);
    capturedWidthSpinBox->setValue(capturedWidth);
    capturedHeightSpinBox->setValue(capturedHeight);
} 