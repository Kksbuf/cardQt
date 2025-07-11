#ifndef DIMENSIONSDIALOG_H
#define DIMENSIONSDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QPushButton>

class DimensionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DimensionsDialog(QWidget *parent = nullptr);
    
    // Getters for the dimensions
    double getActualWidth() const { return actualWidthSpinBox->value(); }
    double getActualHeight() const { return actualHeightSpinBox->value(); }
    double getCapturedWidth() const { return capturedWidthSpinBox->value(); }
    double getCapturedHeight() const { return capturedHeightSpinBox->value(); }

    // Setters for the dimensions
    void setDimensions(double actualWidth, double actualHeight, 
                      double capturedWidth, double capturedHeight);

private:
    void setupUI();
    
    QDoubleSpinBox *actualWidthSpinBox;
    QDoubleSpinBox *actualHeightSpinBox;
    QDoubleSpinBox *capturedWidthSpinBox;
    QDoubleSpinBox *capturedHeightSpinBox;
    QPushButton *confirmButton;
    QPushButton *cancelButton;
};

#endif // DIMENSIONSDIALOG_H 