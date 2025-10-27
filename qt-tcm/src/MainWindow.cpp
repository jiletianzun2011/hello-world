#include "MainWindow.h"

#include <QtWidgets>
#include <QtSql>
#include "HerbDock.h"
#include "TemplatesDialog.h"
#include "HistoryDialog.h"
#include "PatientDialog.h"

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
      saveButton(nullptr),
      herbDock(nullptr),
      actionTemplates(nullptr),
      actionHistory(nullptr),
      actionPatients(nullptr),
      actionSaveTemplate(nullptr) {
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
  actionSaveTemplate = fileMenu->addAction("保存为模板");
  auto* exitAction = fileMenu->addAction("退出");
  connect(exitAction, &QAction::triggered, this, &QWidget::close);

  auto* toolsMenu = menuBar()->addMenu("工具");
  actionTemplates = toolsMenu->addAction("模板方...");
  actionHistory = toolsMenu->addAction("历史处方...");
  actionPatients = toolsMenu->addAction("患者档案...");

  auto* helpMenu = menuBar()->addMenu("帮助");
  auto* aboutAction = helpMenu->addAction("关于");
  connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);

  herbDock = new HerbDock("tcm", this);
  addDockWidget(Qt::LeftDockWidgetArea, herbDock);
}

void MainWindow::connectSignals() {
  connect(addRowButton, &QPushButton::clicked, this, &MainWindow::handleAddRow);
  connect(removeRowButton, &QPushButton::clicked, this, &MainWindow::handleRemoveRow);
  connect(saveButton, &QPushButton::clicked, this, &MainWindow::handleSave);

  connect(actionTemplates, &QAction::triggered, this, &MainWindow::openTemplates);
  connect(actionHistory, &QAction::triggered, this, &MainWindow::openHistory);
  connect(actionPatients, &QAction::triggered, this, &MainWindow::openPatients);
  connect(actionSaveTemplate, &QAction::triggered, this, &MainWindow::saveAsTemplate);

  connect(herbDock, &HerbDock::herbChosen, this, &MainWindow::onHerbChosen);
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

void MainWindow::openTemplates() {
  TemplatesDialog dlg("tcm", this);
  if (dlg.exec() == QDialog::Accepted) {
    applyTemplateById(dlg.selectedFormulaId());
  }
}

void MainWindow::openHistory() {
  HistoryDialog dlg("tcm", this);
  if (dlg.exec() == QDialog::Accepted) {
    loadPrescriptionById(dlg.selectedPrescriptionId());
  }
}

void MainWindow::openPatients() {
  PatientDialog dlg("tcm", this);
  if (dlg.exec() == QDialog::Accepted) {
    const int id = dlg.selectedPatientId();
    if (id > 0) {
      QSqlDatabase db = QSqlDatabase::database("tcm");
      QSqlQuery q(db);
      q.prepare("SELECT name, age, gender FROM patients WHERE id = ?");
      q.addBindValue(id);
      if (q.exec() && q.next()) {
        patientNameEdit->setText(q.value(0).toString());
        patientAgeSpin->setValue(q.value(1).toInt());
        const QString g = q.value(2).toString();
        int idx = patientGenderCombo->findText(g);
        if (idx >= 0) patientGenderCombo->setCurrentIndex(idx);
      }
    }
  }
}

void MainWindow::saveAsTemplate() {
  bool ok = false;
  const QString name = QInputDialog::getText(this, "保存为模板", "模板名:", QLineEdit::Normal, "", &ok);
  if (!ok || name.trimmed().isEmpty()) return;
  QSqlDatabase db = QSqlDatabase::database("tcm");
  if (!db.transaction()) { showError("启动事务失败: " + db.lastError().text()); return; }
  QSqlQuery q(db);
  q.prepare("INSERT INTO formulas(name, source, description) VALUES(?, '', '')");
  q.addBindValue(name.trimmed());
  if (!q.exec()) { db.rollback(); showError(q.lastError().text()); return; }
  QSqlQuery last(db);
  last.exec("SELECT last_insert_rowid()");
  int fid = -1; if (last.next()) fid = last.value(0).toInt();

  QSqlQuery qi(db);
  qi.prepare("INSERT INTO formula_items(formula_id, herb, dosage, unit, usage) VALUES(?, ?, ?, ?, ?)");
  for (int row = 0; row < prescriptionTable->rowCount(); ++row) {
    const QString herb = prescriptionTable->item(row, 0) ? prescriptionTable->item(row, 0)->text().trimmed() : QString();
    const QString dosageStr = prescriptionTable->item(row, 1) ? prescriptionTable->item(row, 1)->text().trimmed() : QString();
    const QString unit = prescriptionTable->item(row, 2) ? prescriptionTable->item(row, 2)->text().trimmed() : QString();
    const QString usage = prescriptionTable->item(row, 3) ? prescriptionTable->item(row, 3)->text().trimmed() : QString();
    if (herb.isEmpty()) continue;
    bool okd = false; double d = dosageStr.toDouble(&okd);
    qi.addBindValue(fid);
    qi.addBindValue(herb);
    qi.addBindValue(okd ? d : QVariant(QMetaType(QMetaType::Double)));
    qi.addBindValue(unit);
    qi.addBindValue(usage);
    if (!qi.exec()) { db.rollback(); showError(qi.lastError().text()); return; }
  }
  if (!db.commit()) { showError(db.lastError().text()); return; }
  QMessageBox::information(this, "完成", "已保存为模板");
}

void MainWindow::onHerbChosen(const QString& herbName) {
  int row = prescriptionTable->currentRow();
  if (row < 0) { row = prescriptionTable->rowCount(); prescriptionTable->insertRow(row); }
  if (!prescriptionTable->item(row, 0)) prescriptionTable->setItem(row, 0, new QTableWidgetItem());
  prescriptionTable->item(row, 0)->setText(herbName);
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
      "patient_id INTEGER,"
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

  const char* createPatients =
      "CREATE TABLE IF NOT EXISTS patients ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "name TEXT NOT NULL,"
      "gender TEXT,"
      "age INTEGER,"
      "phone TEXT,"
      "id_no TEXT,"
      "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
      ");";
  if (!q.exec(createPatients)) { showError("创建表失败: " + q.lastError().text()); return false; }

  const char* createFormulas =
      "CREATE TABLE IF NOT EXISTS formulas ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "name TEXT NOT NULL UNIQUE,"
      "source TEXT,"
      "description TEXT,"
      "created_at TEXT DEFAULT CURRENT_TIMESTAMP"
      ");";
  if (!q.exec(createFormulas)) { showError("创建表失败: " + q.lastError().text()); return false; }

  const char* createFormulaItems =
      "CREATE TABLE IF NOT EXISTS formula_items ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "formula_id INTEGER NOT NULL,"
      "herb TEXT,"
      "dosage REAL,"
      "unit TEXT,"
      "usage TEXT,"
      "FOREIGN KEY(formula_id) REFERENCES formulas(id) ON DELETE CASCADE"
      ");";
  if (!q.exec(createFormulaItems)) { showError("创建表失败: " + q.lastError().text()); return false; }

  const char* createHerbs =
      "CREATE TABLE IF NOT EXISTS herbs ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "name TEXT NOT NULL UNIQUE,"
      "pinyin TEXT,"
      "alias TEXT,"
      "category TEXT,"
      "properties TEXT,"
      "contraindications TEXT"
      ");";
  if (!q.exec(createHerbs)) { showError("创建表失败: " + q.lastError().text()); return false; }

  // Seed minimal data if empty
  if (!q.exec("SELECT COUNT(1) FROM herbs") || !q.next()) return true;
  if (q.value(0).toInt() == 0) {
    QSqlQuery ins(db);
    ins.prepare("INSERT INTO herbs(name, pinyin, alias, category, properties, contraindications) VALUES(?, ?, ?, ?, ?, ?)");
    struct Item { const char* n; const char* py; const char* al; const char* cat; const char* prop; const char* contra; };
    const Item items[] = {
      {"人参","renshen","参","补气","甘、微苦，平；归脾肺心经","实证、热证慎用；不宜与藜芦同用"},
      {"黄芪","huangqi","","补气","甘，微温；归脾肺经","表实邪盛者慎用"},
      {"甘草","gancao","国老","补气/调和","甘，平；归心肺脾胃经","高血压、水肿慎用；不宜与甘遂、大戟等同用"},
      {"茯苓","fuling","","利水渗湿","甘淡，平；归心脾肾经","小便不利属肾虚者慎用"}
    };
    for (const auto& it : items) {
      ins.addBindValue(QString::fromUtf8(it.n));
      ins.addBindValue(QString::fromUtf8(it.py));
      ins.addBindValue(QString::fromUtf8(it.al));
      ins.addBindValue(QString::fromUtf8(it.cat));
      ins.addBindValue(QString::fromUtf8(it.prop));
      ins.addBindValue(QString::fromUtf8(it.contra));
      if (!ins.exec()) { /* ignore seed errors */ }
    }
  }

  if (!q.exec("SELECT COUNT(1) FROM formulas") || !q.next()) return true;
  if (q.value(0).toInt() == 0) {
    if (db.transaction()) {
      QSqlQuery f(db); f.prepare("INSERT INTO formulas(name, source, description) VALUES(?,?,?)");
      f.addBindValue("四君子汤"); f.addBindValue("太平惠民和剂局方"); f.addBindValue("益气健脾");
      if (f.exec()) {
        QSqlQuery last(db); last.exec("SELECT last_insert_rowid()"); int fid = -1; if (last.next()) fid = last.value(0).toInt();
        QSqlQuery fi(db); fi.prepare("INSERT INTO formula_items(formula_id, herb, dosage, unit, usage) VALUES(?,?,?,?,?)");
        struct FI { const char* h; double d; const char* u; const char* use; };
        const FI fis[] = {{"人参",9.0,"g","煎服"},{"茯苓",9.0,"g","煎服"},{"白术",9.0,"g","煎服"},{"甘草",6.0,"g","煎服"}};
        for (const auto& it : fis) {
          fi.addBindValue(fid);
          fi.addBindValue(QString::fromUtf8(it.h));
          fi.addBindValue(it.d);
          fi.addBindValue(QString::fromUtf8(it.u));
          fi.addBindValue(QString::fromUtf8(it.use));
          if (!fi.exec()) { /* ignore seed errors */ }
        }
        db.commit();
      } else { db.rollback(); }
    }
  }

  return true;
}

bool MainWindow::applyTemplateById(int formulaId) {
  if (formulaId <= 0) return false;
  QSqlDatabase db = QSqlDatabase::database("tcm");
  QSqlQuery q(db);
  q.prepare("SELECT herb, dosage, unit, usage FROM formula_items WHERE formula_id = ? ORDER BY id");
  q.addBindValue(formulaId);
  if (!q.exec()) { showError(q.lastError().text()); return false; }
  prescriptionTable->setRowCount(0);
  int row = 0;
  while (q.next()) {
    prescriptionTable->insertRow(row);
    prescriptionTable->setItem(row, 0, new QTableWidgetItem(q.value(0).toString()));
    prescriptionTable->setItem(row, 1, new QTableWidgetItem(q.value(1).isNull() ? QString() : QString::number(q.value(1).toDouble())));
    prescriptionTable->setItem(row, 2, new QTableWidgetItem(q.value(2).toString()));
    prescriptionTable->setItem(row, 3, new QTableWidgetItem(q.value(3).toString()));
    ++row;
  }
  return true;
}

bool MainWindow::loadPrescriptionById(int prescriptionId) {
  if (prescriptionId <= 0) return false;
  QSqlDatabase db = QSqlDatabase::database("tcm");
  QSqlQuery q(db);
  q.prepare("SELECT patient_name, age, gender, diagnosis FROM prescriptions WHERE id = ?");
  q.addBindValue(prescriptionId);
  if (!q.exec() || !q.next()) return false;
  patientNameEdit->setText(q.value(0).toString());
  patientAgeSpin->setValue(q.value(1).toInt());
  int idx = patientGenderCombo->findText(q.value(2).toString());
  if (idx >= 0) patientGenderCombo->setCurrentIndex(idx);
  diagnosisEdit->setPlainText(q.value(3).toString());

  QSqlQuery qi(db);
  qi.prepare("SELECT herb, dosage, unit, usage FROM prescription_items WHERE prescription_id = ? ORDER BY id");
  qi.addBindValue(prescriptionId);
  if (!qi.exec()) return false;
  prescriptionTable->setRowCount(0);
  int row = 0;
  while (qi.next()) {
    prescriptionTable->insertRow(row);
    prescriptionTable->setItem(row, 0, new QTableWidgetItem(qi.value(0).toString()));
    prescriptionTable->setItem(row, 1, new QTableWidgetItem(qi.value(1).isNull() ? QString() : QString::number(qi.value(1).toDouble())));
    prescriptionTable->setItem(row, 2, new QTableWidgetItem(qi.value(2).toString()));
    prescriptionTable->setItem(row, 3, new QTableWidgetItem(qi.value(3).toString()));
    ++row;
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

  // Upsert patient
  int patientId = -1;
  {
    QSqlQuery psel(db);
    psel.prepare("SELECT id FROM patients WHERE name = ? AND COALESCE(age,0) = ? AND COALESCE(gender,'') = ? LIMIT 1");
    psel.addBindValue(patientNameEdit->text().trimmed());
    psel.addBindValue(patientAgeSpin->value());
    psel.addBindValue(patientGenderCombo->currentText());
    if (psel.exec() && psel.next()) {
      patientId = psel.value(0).toInt();
    } else {
      QSqlQuery pins(db);
      pins.prepare("INSERT INTO patients(name, gender, age) VALUES(?, ?, ?)");
      pins.addBindValue(patientNameEdit->text().trimmed());
      pins.addBindValue(patientGenderCombo->currentText());
      pins.addBindValue(patientAgeSpin->value());
      if (!pins.exec()) { db.rollback(); showError("保存患者失败: " + pins.lastError().text()); return false; }
      QSqlQuery last(db); last.exec("SELECT last_insert_rowid()"); if (last.next()) patientId = last.value(0).toInt();
    }
  }

  QSqlQuery insertPrescription(db);
  insertPrescription.prepare(
      "INSERT INTO prescriptions(patient_id, patient_name, age, gender, diagnosis) VALUES(?, ?, ?, ?, ?)"
  );
  insertPrescription.addBindValue(patientId);
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