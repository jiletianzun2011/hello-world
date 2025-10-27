#pragma once

#include <QDialog>

class QLineEdit;
class QListWidget;

class HistoryDialog : public QDialog {
  Q_OBJECT
public:
  explicit HistoryDialog(const QString& connectionName, QWidget* parent = nullptr);
  int selectedPrescriptionId() const;

private slots:
  void handleFilterChanged(const QString& text);
  void handleAccept();

private:
  void refresh(const QString& filterText);

  QString connectionName;
  QLineEdit* filterEdit;
  QListWidget* list;
};