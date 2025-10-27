#include "PatientDialog.h"

#include <QtWidgets>
#include <QtSql>

PatientDialog::PatientDialog(const QString& connectionName, QWidget* parent)
    : QDialog(parent), connectionName(connectionName) {
  setWindowTitle("患者档案");
  resize(480, 520);
  auto* layout = new QVBoxLayout(this);

  filterEdit = new QLineEdit();
  filterEdit->setPlaceholderText("按姓名/手机号/证件号过滤...");
  list = new QListWidget();
  list->setSelectionMode(QAbstractItemView::SingleSelection);

  auto* buttons = new QHBoxLayout();
  auto* ok = new QPushButton("选择");
  auto* cancel = new QPushButton("取消");
  buttons->addStretch();
  buttons->addWidget(ok);
  buttons->addWidget(cancel);

  layout->addWidget(filterEdit);
  layout->addWidget(list, 1);
  layout->addLayout(buttons);

  connect(filterEdit, &QLineEdit::textChanged, this, &PatientDialog::handleFilterChanged);
  connect(ok, &QPushButton::clicked, this, &PatientDialog::handleAccept);
  connect(cancel, &QPushButton::clicked, this, &QDialog::reject);

  refresh("");
}

int PatientDialog::selectedPatientId() const {
  auto* item = list->currentItem();
  if (!item) return -1;
  return item->data(Qt::UserRole).toInt();
}

void PatientDialog::handleFilterChanged(const QString& text) { refresh(text); }

void PatientDialog::handleAccept() { if (selectedPatientId() > 0) accept(); }

void PatientDialog::refresh(const QString& filterText) {
  list->clear();
  QSqlDatabase db = QSqlDatabase::database(connectionName);
  if (!db.isValid() || !db.isOpen()) return;
  QSqlQuery q(db);
  if (filterText.trimmed().isEmpty()) {
    q.prepare("SELECT id, name, COALESCE(gender,''), COALESCE(phone,''), COALESCE(id_no,''), COALESCE(age,0) FROM patients ORDER BY datetime(created_at) DESC, name LIMIT 300");
  } else {
    const QString like = "%" + filterText.trimmed() + "%";
    q.prepare("SELECT id, name, COALESCE(gender,''), COALESCE(phone,''), COALESCE(id_no,''), COALESCE(age,0) FROM patients WHERE name LIKE ? OR phone LIKE ? OR id_no LIKE ? ORDER BY datetime(created_at) DESC, name LIMIT 300");
    q.addBindValue(like);
    q.addBindValue(like);
    q.addBindValue(like);
  }
  if (!q.exec()) return;
  while (q.next()) {
    const int id = q.value(0).toInt();
    const QString name = q.value(1).toString();
    const QString gender = q.value(2).toString();
    const QString phone = q.value(3).toString();
    const QString idNo = q.value(4).toString();
    const int age = q.value(5).toInt();
    auto* item = new QListWidgetItem(QString("%1（%2/%3岁）\n%4  %5").arg(name, gender, QString::number(age), phone, idNo));
    item->setData(Qt::UserRole, id);
    list->addItem(item);
  }
  if (list->count() > 0) list->setCurrentRow(0);
}