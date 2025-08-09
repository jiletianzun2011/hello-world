# 中医处方软件（Qt 6 + C++）

简洁的桌面端示例：患者信息录入 + 处方药材表格 + SQLite 本地持久化。

## 依赖
- Qt 6 (Widgets, Sql)
- CMake >= 3.21
- C++ 编译器（g++/clang++）

## 构建
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## 运行
```bash
./build/qt_tcm
```

数据默认保存在 `QStandardPaths::AppLocalDataLocation` 下的 `tcm_prescriptions.sqlite`。

## 功能
- 患者信息：姓名、年龄、性别、证候/诊断
- 处方表：药材/剂量/单位/用法；支持添加、删除、保存
- 本地 SQLite 存储：`prescriptions` 与 `prescription_items` 两张表

## 后续可扩展
- 检索/历史记录、模板方、常用药材联想
- 处方打印/导出 PDF、签名与版本管理
- 药典/相互作用提示、医保/费用估算