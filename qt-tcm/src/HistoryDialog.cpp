#include "HistoryDialog.h"

#include <QtWidgets>
#include <QtSql>

HistoryDialog::HistoryDialog(const QString& connectionName, QWidget* parent)
    : QDialog(parent), connectionName(connectionName) {
  setWindowTitle("历史处方");
  resize(520, 540);
  auto* layout = new QVBoxLayout(this);

  filterEdit = new QLineEdit();
  filterEdit->setPlaceholderText("按患者/诊断关键词过滤...");
  list = new QListWidget();
  list->setSelectionMode(QAbstractItemView::SingleSelection);

  auto* buttons = new QHBoxLayout();
  auto* ok = new QPushButton("打开");
  auto* cancel = new QPushButton("取消");
  buttons->addStretch();
  buttons->addWidget(ok);
  buttons->addWidget(cancel);

  layout->addWidget(filterEdit);
  layout->addWidget(list, 1);
  layout->addLayout(buttons);

  connect(filterEdit, &QLineEdit::textChanged, this, &HistoryDialog::handleFilterChanged);
  connect(ok, &QPushButton::clicked, this, &HistoryDialog::handleAccept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

  refresh("");
}

int HistoryDialog::selectedPrescriptionId() const {
  auto* item = list->currentItem();
  if (!item) return -1;
  return item->data(Qt::UserRole).toInt();
}

void HistoryDialog::handleFilterChanged(const QString& text) { refresh(text); }

void HistoryDialog::handleAccept() { if (selectedPrescriptionId() > 0) accept(); }

void HistoryDialog::refresh(const QString& filterText) {
  list->clear();
  QSqlDatabase db = QSqlDatabase::database(connectionName);
  if (!db.isValid() || !db.isOpen()) return;
  QSqlQuery q(db);
  if (filterText.trimmed().isEmpty()) {
    q.prepare("SELECT id, patient_name, COALESCE(diagnosis,''), created_at FROM prescriptions ORDER BY datetime(created_at) DESC LIMIT 300");
  } else {
    const QString like = "%" + filterText.trimmed() + "%";
    q.prepare("SELECT id, patient_name, COALESCE(diagnosis,''), created_at FROM prescriptions WHERE patient_name LIKE ? OR diagnosis LIKE ? ORDER BY datetime(created_at) DESC LIMIT 300");
    q.addBindValue(like);
    q.addBindValue(like);
  }
  if (!q.exec()) return;
  while (q.next()) {
    const int id = q.value(0).toInt();
    const QString name = q.value(1).toString();
    const QString diag = q.value(2).toString();
    const QString created = q.value(3).toString();
    auto* item = new QListWidgetItem(QString("[%1] %2\n%3").arg(created, name, diag));
    item->setData(Qt::UserRole, id);
    list->addItem(item);
  }
  if (list->count() > 0) list->setCurrentRow(0);
}