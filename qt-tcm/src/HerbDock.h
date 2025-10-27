#pragma once

#include <QDockWidget>

class QLineEdit;
class QListWidget;
class QTextBrowser;

class HerbDock : public QDockWidget {
  Q_OBJECT
public:
  explicit HerbDock(const QString& connectionName, QWidget* parent = nullptr);
  void setConnectionName(const QString& name);

signals:
  void herbChosen(const QString& herbName);

private slots:
  void handleSearchTextChanged(const QString& text);
  void handleItemActivated();
  void handleCurrentRowChanged(int row);

private:
  void refreshResults(const QString& text);

  QString connectionName;
  QWidget* container;
  QLineEdit* searchEdit;
  QListWidget* resultsList;
  QTextBrowser* detailView;
};