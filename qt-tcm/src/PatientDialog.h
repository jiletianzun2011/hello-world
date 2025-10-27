#pragma once

#include <QDialog>

class QLineEdit;
class QListWidget;

class PatientDialog : public QDialog {
  Q_OBJECT
public:
  explicit PatientDialog(const QString& connectionName, QWidget* parent = nullptr);
  int selectedPatientId() const;

private slots:
  void handleFilterChanged(const QString& text);
  void handleAccept();

private:
  void refresh(const QString& filterText);

  QString connectionName;
  QLineEdit* filterEdit;
  QListWidget* list;
};