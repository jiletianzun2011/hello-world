#include "MainWindow.h"

#include <QtWidgets>
#include <QtSql>

namespace {
QString databaseFilePath() {
  const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  QDir d(dir);
  if (!d.exists()) {
    d.mkpath(".");
  }
  return d.filePath("tcm_prescriptions.sqlite");
}
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      patientNameEdit(nullptr),
      patientAgeSpin(nullptr),
      patientGenderCombo(nullptr),
      diagnosisEdit(nullptr),
      prescriptionTable(nullptr),
      addRowButton(nullptr),
      removeRowButton(nullptr),
      saveButton(nullptr) {
  initializeUi();
  connectSignals();
  ensureDatabase();
}

void MainWindow::initializeUi() {
  setWindowTitle("中医处方 - Qt");
  resize(1000, 700);

  auto* central = new QWidget(this);
  auto* mainLayout = new QVBoxLayout(central);

  auto* formLayout = new QFormLayout();
  patientNameEdit = new QLineEdit();
  patientAgeSpin = new QSpinBox();
  patientAgeSpin->setRange(0, 120);
  patientGenderCombo = new QComboBox();
  patientGenderCombo->addItems({"男", "女", "其他"});
  diagnosisEdit = new QTextEdit();

  formLayout->addRow("姓名:", patientNameEdit);
  formLayout->addRow("年龄:", patientAgeSpin);
  formLayout->addRow("性别:", patientGenderCombo);
  formLayout->addRow("证候/诊断:", diagnosisEdit);

  mainLayout->addLayout(formLayout);

  prescriptionTable = new QTableWidget(0, 4);
  QStringList headers = {"药材", "剂量", "单位", "用法"};
  prescriptionTable->setHorizontalHeaderLabels(headers);
  prescriptionTable->horizontalHeader()->setStretchLastSection(true);
  prescriptionTable->verticalHeader()->setVisible(false);
  prescriptionTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  prescriptionTable->setSelectionMode(QAbstractItemView::SingleSelection);
  prescriptionTable->setEditTriggers(QAbstractItemView::DoubleClicked |
                                     QAbstractItemView::SelectedClicked |
                                     QAbstractItemView::EditKeyPressed);

  mainLayout->addWidget(prescriptionTable, 1);

  auto* buttonRow = new QHBoxLayout();
  addRowButton = new QPushButton("添加药材");
  removeRowButton = new QPushButton("删除选中");
  saveButton = new QPushButton("保存处方");
  buttonRow->addStretch();
  buttonRow->addWidget(addRowButton);
  buttonRow->addWidget(removeRowButton);
  buttonRow->addWidget(saveButton);

  mainLayout->addLayout(buttonRow);

  setCentralWidget(central);

  auto* fileMenu = menuBar()->addMenu("文件");
  auto* exitAction = fileMenu->addAction("退出");
  connect(exitAction, &QAction::triggered, this, &QWidget::close);

  auto* helpMenu = menuBar()->addMenu("帮助");
  auto* aboutAction = helpMenu->addAction("关于");
  connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
}

void MainWindow::connectSignals() {
  connect(addRowButton, &QPushButton::clicked, this, &MainWindow::handleAddRow);
  connect(removeRowButton, &QPushButton::clicked, this, &MainWindow::handleRemoveRow);
  connect(saveButton, &QPushButton::clicked, this, &MainWindow::handleSave);
}

void MainWindow::handleAddRow() {
  int row = prescriptionTable->rowCount();
  prescriptionTable->insertRow(row);
  for (int col = 0; col < prescriptionTable->columnCount(); ++col) {
    prescriptionTable->setItem(row, col, new QTableWidgetItem());
  }
}

void MainWindow::handleRemoveRow() {
  int row = prescriptionTable->currentRow();
  if (row >= 0) {
    prescriptionTable->removeRow(row);
  }
}

void MainWindow::handleSave() {
  if (patientNameEdit->text().trimmed().isEmpty()) {
    showError("请填写患者姓名");
    return;
  }
  if (!saveCurrentPrescription()) {
    return;
  }
  QMessageBox::information(this, "完成", "处方已保存");
}

void MainWindow::showAbout() {
  QMessageBox::about(this, "关于", "中医处方软件（示例）\n基于 Qt 6 + C++");
}

void MainWindow::showError(const QString& message) {
  QMessageBox::warning(this, "提示", message);
}

bool MainWindow::ensureDatabase() {
  if (QSqlDatabase::contains("tcm")) {
    return true;
  }
  QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "tcm");
  db.setDatabaseName(databaseFilePath());
  if (!db.open()) {
    showError("无法打开数据库: " + db.lastError().text());
    return false;
  }
  QSqlQuery q(db);
  const char* createPrescriptions =
      "CREATE TABLE IF NOT EXISTS prescriptions ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "patient_name TEXT NOT NULL,"
      "age INTEGER,"
      "gender TEXT,"
      "diagnosis TEXT,"
      "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
      ");";
  if (!q.exec(createPrescriptions)) {
    showError("创建表失败: " + q.lastError().text());
    return false;
  }
  const char* createItems =
      "CREATE TABLE IF NOT EXISTS prescription_items ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "prescription_id INTEGER NOT NULL,"
      "herb TEXT,"
      "dosage REAL,"
      "unit TEXT,"
      "usage TEXT,"
      "FOREIGN KEY(prescription_id) REFERENCES prescriptions(id) ON DELETE CASCADE"
      ");";
  if (!q.exec(createItems)) {
    showError("创建表失败: " + q.lastError().text());
    return false;
  }
  return true;
}

bool MainWindow::saveCurrentPrescription() {
  QSqlDatabase db = QSqlDatabase::database("tcm");
  if (!db.isValid() || !db.isOpen()) {
    showError("数据库未打开");
    return false;
  }

  if (!db.transaction()) {
    showError("启动事务失败: " + db.lastError().text());
    return false;
  }

  QSqlQuery insertPrescription(db);
  insertPrescription.prepare(
      "INSERT INTO prescriptions(patient_name, age, gender, diagnosis) VALUES(?, ?, ?, ?)"
  );
  insertPrescription.addBindValue(patientNameEdit->text().trimmed());
  insertPrescription.addBindValue(patientAgeSpin->value());
  insertPrescription.addBindValue(patientGenderCombo->currentText());
  insertPrescription.addBindValue(diagnosisEdit->toPlainText().trimmed());

  if (!insertPrescription.exec()) {
    db.rollback();
    showError("保存处方失败: " + insertPrescription.lastError().text());
    return false;
  }

  QSqlQuery lastIdQuery(db);
  if (!lastIdQuery.exec("SELECT last_insert_rowid()")) {
    db.rollback();
    showError("获取ID失败: " + lastIdQuery.lastError().text());
    return false;
  }
  int prescriptionId = -1;
  if (lastIdQuery.next()) {
    prescriptionId = lastIdQuery.value(0).toInt();
  }

  QSqlQuery insertItem(db);
  insertItem.prepare(
      "INSERT INTO prescription_items(prescription_id, herb, dosage, unit, usage) VALUES(?, ?, ?, ?, ?)"
  );

  for (int row = 0; row < prescriptionTable->rowCount(); ++row) {
    const QString herb = prescriptionTable->item(row, 0) ? prescriptionTable->item(row, 0)->text().trimmed() : QString();
    const QString dosageStr = prescriptionTable->item(row, 1) ? prescriptionTable->item(row, 1)->text().trimmed() : QString();
    const QString unit = prescriptionTable->item(row, 2) ? prescriptionTable->item(row, 2)->text().trimmed() : QString();
    const QString usage = prescriptionTable->item(row, 3) ? prescriptionTable->item(row, 3)->text().trimmed() : QString();

    if (herb.isEmpty()) {
      continue; // skip empty rows
    }

    bool ok = false;
    double dosage = dosageStr.toDouble(&ok);
    if (!ok && !dosageStr.isEmpty()) {
      db.rollback();
      showError(QString("剂量无效(行 %1)").arg(row + 1));
      return false;
    }

    insertItem.addBindValue(prescriptionId);
    insertItem.addBindValue(herb);
    insertItem.addBindValue(ok ? QVariant(dosage) : QVariant(QMetaType(QMetaType::Double)));
    insertItem.addBindValue(unit);
    insertItem.addBindValue(usage);

    if (!insertItem.exec()) {
      db.rollback();
      showError(QString("保存药材失败(行 %1): ").arg(row + 1) + insertItem.lastError().text());
      return false;
    }
  }

  if (!db.commit()) {
    showError("提交事务失败: " + db.lastError().text());
    return false;
  }

  return true;
}