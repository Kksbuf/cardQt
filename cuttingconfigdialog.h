#ifndef CUTTINGCONFIGDIALOG_H
#define CUTTINGCONFIGDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QSpinBox>
#include <QRadioButton>
#include <QPushButton>

class CuttingConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CuttingConfigDialog(QWidget *parent = nullptr,
                               double surfaceWidth = 420.0,
                               double surfaceHeight = 297.0,
                               double captureWidth = 140.0,
                               double captureHeight = 99.0,
                               int surfaceCount = 1);
    
    int getPiecesInX() const { return piecesInXSpinBox->value(); }
    int getPiecesInY() const { return piecesInYSpinBox->value(); }
    bool isXAxisStacking() const { return xAxisStackingRadio->isChecked(); }

private:
    void setupUI();
    void updateTotalPieces();

    QLabel *surfaceSizeLabel;
    QLabel *captureAreaLabel;
    QLabel *surfaceCountLabel;
    QSpinBox *piecesInXSpinBox;
    QSpinBox *piecesInYSpinBox;
    QLabel *totalPiecesLabel;
    QLabel *totalPiecesAllLabel;
    QRadioButton *xAxisStackingRadio;
    QRadioButton *singleStackRadio;
    QPushButton *okButton;
    QPushButton *cancelButton;

    double surfaceWidth;
    double surfaceHeight;
    double captureWidth;
    double captureHeight;
    int surfaceCount;

private slots:
    void onPiecesChanged();
};

#endif // CUTTINGCONFIGDIALOG_H 