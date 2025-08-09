#pragma once

#include <QDialog>

class QLineEdit;
class QListWidget;
class QPushButton;

class TemplatesDialog : public QDialog {
  Q_OBJECT
public:
  explicit TemplatesDialog(const QString& connectionName, QWidget* parent = nullptr);
  int selectedFormulaId() const;

signals:
  void requestDelete(int formulaId);

private slots:
  void handleFilterChanged(const QString& text);
  void handleAccept();
  void handleDelete();

private:
  void refresh(const QString& filterText);

  QString connectionName;
  QLineEdit* filterEdit;
  QListWidget* list;
  QPushButton* okButton;
  QPushButton* deleteButton;
};