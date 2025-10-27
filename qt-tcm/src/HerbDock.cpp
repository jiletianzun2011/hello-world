#include "HerbDock.h"

#include <QtWidgets>
#include <QtSql>

HerbDock::HerbDock(const QString& connectionName, QWidget* parent)
    : QDockWidget(parent), connectionName(connectionName) {
  setWindowTitle("草药字典");
  container = new QWidget(this);
  auto* layout = new QVBoxLayout(container);

  searchEdit = new QLineEdit();
  searchEdit->setPlaceholderText("搜索：名称/拼音/别名 (回车或即时过滤)");

  resultsList = new QListWidget();
  resultsList->setSelectionMode(QAbstractItemView::SingleSelection);

  detailView = new QTextBrowser();
  detailView->setOpenLinks(false);

  layout->addWidget(searchEdit);
  layout->addWidget(resultsList, 1);
  layout->addWidget(detailView);
  container->setLayout(layout);
  setWidget(container);

  connect(searchEdit, &QLineEdit::textChanged, this, &HerbDock::handleSearchTextChanged);
  connect(resultsList, &QListWidget::itemActivated, this, &HerbDock::handleItemActivated);
  connect(resultsList, &QListWidget::currentRowChanged, this, &HerbDock::handleCurrentRowChanged);

  refreshResults("");
}

void HerbDock::setConnectionName(const QString& name) { connectionName = name; }

void HerbDock::handleSearchTextChanged(const QString& text) { refreshResults(text); }

void HerbDock::handleItemActivated() {
  auto* item = resultsList->currentItem();
  if (!item) return;
  emit herbChosen(item->data(Qt::UserRole).toString());
}

void HerbDock::handleCurrentRowChanged(int row) {
  if (row < 0) { detailView->clear(); return; }
  auto* item = resultsList->item(row);
  if (!item) { detailView->clear(); return; }
  const QString name = item->data(Qt::UserRole).toString();
  QSqlDatabase db = QSqlDatabase::database(connectionName);
  if (!db.isValid() || !db.isOpen()) { detailView->clear(); return; }
  QSqlQuery q(db);
  q.prepare("SELECT contraindications, properties, category, pinyin, alias FROM herbs WHERE name = ? LIMIT 1");
  q.addBindValue(name);
  if (!q.exec() || !q.next()) { detailView->clear(); return; }
  const QString contraind = q.value(0).toString();
  const QString props = q.value(1).toString();
  const QString category = q.value(2).toString();
  const QString pinyin = q.value(3).toString();
  const QString alias = q.value(4).toString();

  QString html;
  html += QString("<b>%1</b> (%2) — 分类：%3").arg(name, pinyin, category);
  if (!alias.trimmed().isEmpty()) html += QString("<br/>别名：%1").arg(alias);
  if (!props.trimmed().isEmpty()) html += QString("<br/>性味归经：%1").arg(props);
  if (!contraind.trimmed().isEmpty()) html += QString("<br/><b>禁忌：</b>%1").arg(contraind);
  detailView->setHtml(html);
}

void HerbDock::refreshResults(const QString& text) {
  resultsList->clear();
  QSqlDatabase db = QSqlDatabase::database(connectionName);
  if (!db.isValid() || !db.isOpen()) return;
  QSqlQuery q(db);
  const QString like = "%" + text.trimmed() + "%";
  if (text.trimmed().isEmpty()) {
    q.prepare("SELECT name, pinyin FROM herbs ORDER BY name LIMIT 100");
  } else {
    q.prepare("SELECT name, pinyin FROM herbs WHERE name LIKE ? OR pinyin LIKE ? OR alias LIKE ? ORDER BY name LIMIT 100");
    q.addBindValue(like);
    q.addBindValue(like);
    q.addBindValue(like);
  }
  if (!q.exec()) return;
  while (q.next()) {
    const QString name = q.value(0).toString();
    const QString pinyin = q.value(1).toString();
    auto* item = new QListWidgetItem(QString("%1 (%2)").arg(name, pinyin));
    item->setData(Qt::UserRole, name);
    resultsList->addItem(item);
  }
  if (resultsList->count() > 0) resultsList->setCurrentRow(0);
}