#pragma once

#include <QMainWindow>

class QLineEdit;
class QSpinBox;
class QComboBox;
class QTextEdit;
class QTableWidget;
class QPushButton;
class QAction;
class HerbDock;

class MainWindow : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(QWidget* parent = nullptr);

private slots:
  void handleAddRow();
  void handleRemoveRow();
  void handleSave();
  void showAbout();
  void openTemplates();
  void openHistory();
  void openPatients();
  void saveAsTemplate();
  void onHerbChosen(const QString& herbName);

private:
  void initializeUi();
  void connectSignals();
  bool ensureDatabase();
  bool saveCurrentPrescription();
  void showError(const QString& message);
  bool applyTemplateById(int formulaId);
  bool loadPrescriptionById(int prescriptionId);

  QLineEdit* patientNameEdit;
  QSpinBox* patientAgeSpin;
  QComboBox* patientGenderCombo;
  QTextEdit* diagnosisEdit;
  QTableWidget* prescriptionTable;
  QPushButton* addRowButton;
  QPushButton* removeRowButton;
  QPushButton* saveButton;

  HerbDock* herbDock;

  QAction* actionTemplates;
  QAction* actionHistory;
  QAction* actionPatients;
  QAction* actionSaveTemplate;
};