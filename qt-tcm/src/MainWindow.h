#pragma once

#include <QMainWindow>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QTextEdit;
class QTableWidget;
class QPushButton;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);

private slots:
  void handleAddRow();
  void handleRemoveRow();
  void handleSave();
  void showAbout();

private:
  void initializeUi();
  void connectSignals();
  bool ensureDatabase();
  bool saveCurrentPrescription();
  void showError(const QString& message);

  QLineEdit* patientNameEdit;
  QSpinBox* patientAgeSpin;
  QComboBox* patientGenderCombo;
  QTextEdit* diagnosisEdit;
  QTableWidget* prescriptionTable;
  QPushButton* addRowButton;
  QPushButton* removeRowButton;
  QPushButton* saveButton;
};