#include "TemplatesDialog.h"

#include <QtWidgets>
#include <QtSql>

TemplatesDialog::TemplatesDialog(const QString& connectionName, QWidget* parent)
    : QDialog(parent), connectionName(connectionName) {
  setWindowTitle("模板方");
  resize(480, 520);
  auto* layout = new QVBoxLayout(this);

  filterEdit = new QLineEdit();
  filterEdit->setPlaceholderText("过滤模板名称...");
  list = new QListWidget();
  list->setSelectionMode(QAbstractItemView::SingleSelection);

  auto* buttonsLayout = new QHBoxLayout();
  deleteButton = new QPushButton("删除");
  okButton = new QPushButton("应用");
  auto* cancelBtn = new QPushButton("取消");
  buttonsLayout->addWidget(deleteButton);
  buttonsLayout->addStretch();
  buttonsLayout->addWidget(okButton);
  buttonsLayout->addWidget(cancelBtn);

  layout->addWidget(filterEdit);
  layout->addWidget(list, 1);
  layout->addLayout(buttonsLayout);

  connect(filterEdit, &QLineEdit::textChanged, this, &TemplatesDialog::handleFilterChanged);
  connect(okButton, &QPushButton::clicked, this, &TemplatesDialog::handleAccept);
  connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
  connect(deleteButton, &QPushButton::clicked, this, &TemplatesDialog::handleDelete);

  refresh("");
}

int TemplatesDialog::selectedFormulaId() const {
  auto* item = list->currentItem();
  if (!item) return -1;
  return item->data(Qt::UserRole).toInt();
}

void TemplatesDialog::handleFilterChanged(const QString& text) { refresh(text); }

void TemplatesDialog::handleAccept() { if (selectedFormulaId() > 0) accept(); }

void TemplatesDialog::handleDelete() {
  const int id = selectedFormulaId();
  if (id <= 0) return;
  if (QMessageBox::question(this, "确认", "确定删除该模板？") != QMessageBox::Yes) return;
  QSqlDatabase db = QSqlDatabase::database(connectionName);
  if (!db.isValid() || !db.isOpen()) return;
  QSqlQuery q(db);
  q.prepare("DELETE FROM formulas WHERE id = ?");
  q.addBindValue(id);
  if (!q.exec()) {
    QMessageBox::warning(this, "错误", q.lastError().text());
    return;
  }
  refresh(filterEdit->text());
}

void TemplatesDialog::refresh(const QString& filterText) {
  list->clear();
  QSqlDatabase db = QSqlDatabase::database(connectionName);
  if (!db.isValid() || !db.isOpen()) return;
  QSqlQuery q(db);
  if (filterText.trimmed().isEmpty()) {
    q.prepare("SELECT id, name, COALESCE(source,''), COALESCE(description,'') FROM formulas ORDER BY name LIMIT 200");
  } else {
    const QString like = "%" + filterText.trimmed() + "%";
    q.prepare("SELECT id, name, COALESCE(source,''), COALESCE(description,'') FROM formulas WHERE name LIKE ? OR source LIKE ? OR description LIKE ? ORDER BY name LIMIT 200");
    q.addBindValue(like);
    q.addBindValue(like);
    q.addBindValue(like);
  }
  if (!q.exec()) return;
  while (q.next()) {
    const int id = q.value(0).toInt();
    const QString name = q.value(1).toString();
    const QString source = q.value(2).toString();
    const QString desc = q.value(3).toString();
    auto* item = new QListWidgetItem(QString("%1  —  %2\n%3").arg(name, source, desc));
    item->setData(Qt::UserRole, id);
    item->setToolTip(desc);
    list->addItem(item);
  }
  if (list->count() > 0) list->setCurrentRow(0);
}