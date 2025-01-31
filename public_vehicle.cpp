#include "public_vehicle.h"
#include "ui_public_vehicle.h"
#include "BasePage.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <Qstring>
#include <QCryptographicHash>
#include <QByteArray>
#include <QMessageBox>
#include <QComboBox>

#include <logger.h>

public_velicle::public_velicle(BasePage *parent)
    : BasePage(parent)
    , ui(new Ui::public_velicle)
{
    ui->setupUi(this);
    connect(ui->refresh_button_3, &QPushButton::clicked, this, &public_velicle::loadDataFromDatabase);
    connect(ui->add_button_3, &QPushButton::clicked, this, &public_velicle::addrow);

    ui->tableWidget_3->setColumnCount(4);  // 4 列
    // 設置表格標題
    ui->tableWidget_3->setHorizontalHeaderLabels({
        "車牌號碼", "乘坐數", "車輛狀態","刪除按鈕"
    });
    ui->tableWidget_3->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed);

    connect(ui->tableWidget_3, &QTableWidget::cellChanged, this, &public_velicle::onCellChanged);
    // 從資料庫載入資料
    loadDataFromDatabase();
}
void public_velicle::closeEvent(QCloseEvent *event)
{
    // 當窗口被關閉時，使用 deleteLater() 推遲銷毀
    this->deleteLater();  // 確保在事件循環後銷毀視窗

    // 可以添加額外的處理邏輯（例如發送信號等）
    QWidget::closeEvent(event);  // 呼叫基類的 closeEvent，執行正常的關閉操作
}
public_velicle::~public_velicle()
{
    delete ui;
}

void public_velicle::loadDataFromDatabase()
{
    disconnect(ui->tableWidget_3, &QTableWidget::cellChanged, this, &public_velicle::onCellChanged);

    // 执行数据库查询，选取所有角色数据
    QSqlQuery query(
        "SELECT "
        "car_number, can_passenger, car_situation_id, id "
        "FROM "
        "car "
        "WHERE "
        "is_del = 0"
        );
    // 清除表格中的现有行
    ui->tableWidget_3->setRowCount(0);

    int row = 0;  // 用于记录当前插入的行索引

    // 遍历查询结果并填充表格
    while (query.next()) {
        // 在 TableWidget 插入一行
        ui->tableWidget_3->insertRow(row);

        // 遍历数据库列并将值填入每个单元格
        for (int col = 0; col < ui->tableWidget_3->columnCount() - 1; ++col) {
            QString cellText = query.value(col).toString();
            QTableWidgetItem *item = new QTableWidgetItem(cellText);
            item->setTextAlignment(Qt::AlignCenter);

            // 如果需要，可以附加隐藏数据，例如 ID（仅对第一列进行此操作）
            if (col == 0) {
                item->setData(Qt::UserRole, query.value("id"));
            }
            // 将表格项目添加到表格中
            ui->tableWidget_3->setItem(row, col, item);
        }
        // 创建下拉框 (QComboBox) 并填充角色名称
        QComboBox *comboBox = new QComboBox();
        comboBox->addItem("可以開", 1);  // 角色名作为显示值，id作为隐藏数据
        comboBox->addItem("不能開", 0);  // 角色名作为显示值，id作为隐藏数据

        // 设置下拉框默认选中的值为当前用户的角色
        int currentRoleId = query.value("car_situation_id").toInt();
        int index = comboBox->findData(currentRoleId);
        if (index >= 0) {
            comboBox->setCurrentIndex(index);
        }

        // 将下拉框放入第二列
        ui->tableWidget_3->setCellWidget(row, 2, comboBox);
        // // 连接 QComboBox 的 currentIndexChanged 信号
        connect(comboBox, &QComboBox::currentIndexChanged, this, [this, row](int index) {
            qDebug() << "Row" << row << ", Column 2 (Role) index changed!";
            this->onCellChanged(row); // 呼叫行改變槽函式
        });
        // // 创建下拉框 (QComboBox) 并填充角色名称
        // QComboBox *comboBox = new QComboBox();
        // // 将角色列表添加到下拉框
        // for (auto it = rolesMap.begin(); it != rolesMap.end(); ++it) {
        //     comboBox->addItem(it.value(), it.key());  // 角色名作为显示值，id作为隐藏数据
        // }

        // // 设置下拉框默认选中的值为当前用户的角色
        // int currentRoleId = query.value("role_id").toInt();
        // int index = comboBox->findData(currentRoleId);
        // if (index >= 0) {
        //     comboBox->setCurrentIndex(index);
        // }

        // // 将下拉框放入第二列
        // ui->tableWidget_3->setCellWidget(row, 1, comboBox);
        // // // 连接 QComboBox 的 currentIndexChanged 信号
        // connect(comboBox, &QComboBox::currentIndexChanged, this, [this, row](int index) {
        //     qDebug() << "Row" << row << ", Column 1 (Role) index changed!";
        //     this->onCellChanged(row); // 呼叫行改變槽函式
        // });
        // 插入删除按钮到最后一列
        QPushButton *deleteButton = new QPushButton("删除");
        deleteButton->setStyleSheet(
            "font-size: 12px; "
            "height: 30px; "
            "width: 20px; "
            "min-height:30px; "
            "min-width:40px; "
            "max-height:30px; "
            "max-width:40px; "
            "margin-left: 25%; "
            "margin-right: 25%;"
            );
        deleteButton->setProperty("rowId", query.value("id"));

        // 连接按钮的点击信号到 deleteRow 槽函数
        connect(deleteButton, &QPushButton::clicked, this, [this, deleteButton]() {
            int id = deleteButton->property("rowId").toInt();  // 获取隐藏的 id
            this->deleteRow(id);
        });

        // 将删除按钮添加到表格
        ui->tableWidget_3->setCellWidget(row, ui->tableWidget_3->columnCount() - 1, deleteButton);
        qDebug() << row << " ------------------------------------ \n";
        row++;  // 增加行索引
    }

    // 设置列和行的自动调整大小
    ui->tableWidget_3->resizeColumnsToContents();
    ui->tableWidget_3->resizeRowsToContents();

    // 计算某一行的宽度
    int rowWidth = 0;
    int rowIndex = 0;  // 假设要计算第 0 行的宽度
    for (int col = 0; col < ui->tableWidget_3->columnCount(); ++col) {
        rowWidth += ui->tableWidget_3->columnWidth(col);  // 获取该列的宽度
    }

    // 增加额外的 20 宽度来调整窗口
    int currentWidth = this->width();  // 获取当前窗口宽度
    this->resize(rowWidth + 45, this->height());  // 设置新宽度并保持高度不


    connect(ui->tableWidget_3, &QTableWidget::cellChanged, this, &public_velicle::onCellChanged);
}

void public_velicle::onCellChanged(int row) {
    int id = ui->tableWidget_3->item(row, 0)->data(Qt::UserRole).toInt();
    // 檢查行號是否有效
    if (row < 0 || row >= ui->tableWidget_3->rowCount()) {
        qDebug() << "Invalid row number:" << row;
        return;
    }

    qDebug() << "Row changed:" << row;

    // 遍歷該行的所有列，輸出狀態
    for (int col = 0; col < ui->tableWidget_3->columnCount(); ++col) {
        QWidget *widget = ui->tableWidget_3->cellWidget(row, col);

        if (widget && qobject_cast<QComboBox *>(widget)) {
            // 檢查是否是 QComboBox
            QComboBox *comboBox = qobject_cast<QComboBox *>(widget);
            int selectedRoleId = comboBox->currentData().toInt();
            qDebug() << "Column" << col << ": QComboBox selected Role ID:" << selectedRoleId;
        } else if (ui->tableWidget_3->item(row, col)) {
            // 如果是普通的 QTableWidgetItem，直接輸出文本和隱藏數據
            QTableWidgetItem *item = ui->tableWidget_3->item(row, col);
            qDebug() << "Column" << col
                     << ": Text:" << item->text()
                     << ": data:" << item->data(Qt::UserRole).toString();
        } else {
            qDebug() << "Column" << col << ": Empty";
        }
    }

    // 從第一列取得資料庫中記錄的 ID
    if (ui->tableWidget_3->item(row, 0)) {
        int id = ui->tableWidget_3->item(row, 0)->data(Qt::UserRole).toInt();
        // 保存數據到資料庫
        saveDataToDatabase(row, id);
        loadDataFromDatabase();  // 重新加載數據，更新顯示
    } else {
        qDebug() << "Row" << row << "does not have a valid ID in column 0.";
    }
}

void public_velicle::deleteRow(int id) {
    // 使用 sender() 獲取信號的發送者
    QPushButton *button = qobject_cast<QPushButton *>(sender());
    if (!button) {
        qDebug() << "Error: Sender is not a QPushButton.";
        return;
    }
    // 獲取按鈕所在的行
    int row = -1;
    for (int i = 0; i < ui->tableWidget_3->rowCount(); ++i) {
        if (ui->tableWidget_3->cellWidget(i, ui->tableWidget_3->columnCount() - 1) == button) {
            row = i;
            break;
        }
    }
    // 檢查行號是否有效
    if (row < 0 || row >= ui->tableWidget_3->rowCount()) {
        qDebug() << "Invalid row number:" << row;
        return;
    }
    // 彈出確認對話框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(
        this,
        "確認刪除",
        QString("您確定要刪除第 %1 行嗎？").arg(row + 1),
        QMessageBox::Yes | QMessageBox::No
        );
    // 如果用戶點擊 "Yes"，刪除行
    if (reply == QMessageBox::Yes) {
        QSqlQuery deleteQuery;
        deleteQuery.prepare("UPDATE car SET is_del = 1 WHERE id = :id");
        deleteQuery.bindValue(":id", id);
        if (deleteQuery.exec()) {
            qDebug() << "Row deleted successfully!";
        }
        logger log_ins;
        QString logMessage = QString("%1 刪除了一台車輛，車輛編號: %2")
                                 .arg(log_ins.return_username())
                                 .arg(id);
        log_ins.save_logger(logMessage);

        loadDataFromDatabase();
        qDebug() << "Row" << row << "deleted successfully!";
    } else {
        qDebug() << "Row deletion canceled.";
    }
}

// 儲存特定行資料到資料庫
void public_velicle::saveDataToDatabase(int row, int id)
{
    if (row < 0 || row >= ui->tableWidget_3->rowCount()) {
        qDebug() << "ERROR: Invalid row:" << row;
        return;
    }
    int currentUserId = 0; // 初始化当前用户 ID
    // 检查第 0 列的 QTableWidgetItem 是否存在
    if (ui->tableWidget_3->item(row, 0) != nullptr) {
        QVariant idData = ui->tableWidget_3->item(row, 0)->data(Qt::UserRole);
        if (idData.isValid()) {
            currentUserId = idData.toInt();
        } else {
            qDebug() << "Error: ID data not found in Qt::UserRole.";
        }
    }
    // 取得該行的用戶名，並檢查是否為空
    QString car_number = "";
    if (ui->tableWidget_3->item(row, 0) != nullptr) {
        car_number = ui->tableWidget_3->item(row, 0)->text();
    }
    QString can_passenger = "";
    if (ui->tableWidget_3->item(row, 1) != nullptr) {
        can_passenger = ui->tableWidget_3->item(row, 1)->text();
    }
    QString car_situation_id = "";
    // 确保单元格内确实有一个小部件（假设在第 2 列）
    if (ui->tableWidget_3->cellWidget(row, 2) != nullptr) {
        QComboBox* comboBox = qobject_cast<QComboBox*>(ui->tableWidget_3->cellWidget(row, 2));
        if (comboBox != nullptr) {
            car_situation_id = comboBox->currentData().toString(); // 获取与当前选项关联的Data
            // 或者获取显示的文本：
            // car_situation_id = comboBox->currentText();
        } else {
            qDebug() << "Error: The widget in the cell is not a QComboBox.";
        }
    } else {
        qDebug() << "Error: No widget in the specified cell.";
    }

    // Query the database to check for duplicate usernames
    QSqlQuery roleQuery;
    roleQuery.prepare("SELECT COUNT(*) FROM car WHERE car_number = :car_number AND is_del = 0 AND id != :id");
    roleQuery.bindValue(":car_number", car_number);
    roleQuery.bindValue(":id",currentUserId);
    if (roleQuery.exec() && roleQuery.next()) {
        int count = roleQuery.value(0).toInt();
        if (count > 0) {
            QMessageBox::warning(this, "警告", "車牌不能重複！");
            loadDataFromDatabase();
            return;
        }
    } else {
        QMessageBox::critical(this, "錯誤", "無法檢查車牌是否重複：" + roleQuery.lastError().text());
        return;
    }
    // 先初始化 SQL 查询
    QString sql = "UPDATE car SET "
                  "car_number = :car_number,"
                  "can_passenger = :can_passenger,"
                  "car_situation_id = :car_situation";


    sql += " WHERE id = :id";  // 添加 WHERE 子句

    // 创建 SQL 查询并准备执行
    QSqlQuery query;
    query.prepare(sql);

    // 绑定查询参数
    query.bindValue(":id", id);
    query.bindValue(":car_number", car_number);
    query.bindValue(":can_passenger", can_passenger);
    query.bindValue(":car_situation", car_situation_id);
    // 执行查询
    if (query.exec()) {
        qDebug() << "User updated successfully!";
    } else {
        qDebug() << "Failed to update user:" << query.lastError();
    }
    logger log_ins;
    QString logMessage = QString("%1 修改了車輛資料，車輛編號: %2 資料: car_number = %3 can_passenger = %4 car_situation_id = %5")
                             .arg(log_ins.return_username())
                             .arg(id).arg(car_number).arg(can_passenger).arg(car_situation_id);
    log_ins.save_logger(logMessage);
    qDebug() << "Executing update query for User ID:" << id;
}

#include <QLabel>
#include <QLineEdit>
// addrow 函式實現
void public_velicle::addrow() {
    // 創建對話框
    QDialog dialog(this);
    dialog.setWindowTitle("新增車輛");

    // 創建控件
    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    QHBoxLayout *label_text = new QHBoxLayout(&dialog);
    // 名稱輸入框
    QLabel *car_number_Label = new QLabel("車牌號碼:", &dialog);
    QLineEdit *car_number_Edit = new QLineEdit(&dialog);
    QLabel *can_passenger_Label = new QLabel("乘坐數量:", &dialog);
    QLineEdit *can_passenger_Edit = new QLineEdit(&dialog);
    // 設置垂直布局
    label_text->addWidget(car_number_Label);
    label_text->addWidget(car_number_Edit);
    mainLayout->addLayout(label_text);
    label_text = new QHBoxLayout(&dialog);

    label_text->addWidget(can_passenger_Label);
    label_text->addWidget(can_passenger_Edit);
    mainLayout->addLayout(label_text);
    label_text = new QHBoxLayout(&dialog);

    // 角色選擇框
    label_text = new QHBoxLayout(&dialog);
    QLabel *roleLabel = new QLabel("角色:", &dialog);
    QComboBox *roleComboBox = new QComboBox(&dialog);


    roleComboBox->addItem("不能開",0); // 添加角色名稱，並將角色 ID 設為隱藏值
    roleComboBox->addItem("能開",1); // 添加角色名稱，並將角色 ID 設為隱藏值

    label_text->addWidget(roleLabel);
    label_text->addWidget(roleComboBox);
    mainLayout->addLayout(label_text);
    // 設置對話框按鈕
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    mainLayout->addWidget(buttonBox);


    // 連接按鈕信號
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // 顯示對話框
    if (dialog.exec() == QDialog::Accepted) {
        // 獲取名稱
        QString car_number = car_number_Edit->text();
        QString can_passenger = can_passenger_Edit->text();
        int selectedRoleId = roleComboBox->currentData().toInt();

        if (!car_number.isEmpty() && !can_passenger.isEmpty()) {
            // 首先檢查資料庫中是否已存在相同的用戶名
            QSqlQuery checkQuery;
            checkQuery.prepare("SELECT COUNT(*) FROM car WHERE car_number = :car_number AND is_del = 0");
            checkQuery.bindValue(":car_number", car_number);

            if (checkQuery.exec() && checkQuery.next()) {
                int count = checkQuery.value(0).toInt();
                if (count > 0) {
                    QMessageBox::warning(this, "警告", "該車牌已存在，請使用其他車牌！");
                    return; // 中止插入操作
                }
            } else {
                qDebug() << "檢查用戶名失敗：" << checkQuery.lastError().text();
                QMessageBox::critical(this, "錯誤", "無法檢查車牌是否已存在！");
                return;
            }

            // 開始插入資料到資料庫
            QSqlQuery query;
            // 使用 SQL 插入語句
            query.prepare("INSERT INTO car (car_number, can_passenger, car_situation_id,is_del) "
                          "VALUES (:car_number, :can_passenger, :car_situation_id,0)");
            // 設定綁定參數
            query.bindValue(":car_number", car_number);
            query.bindValue(":can_passenger", can_passenger);
            query.bindValue(":car_situation_id", selectedRoleId);

            // 執行查詢並檢查是否成功
            if (query.exec()) {
                // 重新加載資料庫資料並更新 UI
                loadDataFromDatabase();
            } else {
                qDebug() << "新增人員失敗：" << query.lastError().text();
                QMessageBox::warning(this, "錯誤", "無法新增人員至資料庫！");
            }
            // 獲取返回的訂單 ID
            int orderId = -1;
            if (query.next()) {
                orderId = query.value(0).toInt();
            }
            logger log_ins;
            QString logMessage = QString("%1 新增了一台車輛，規則編號: %2 資料 car_number = %3 can_passenger = %4 car_situation_id = %5")
                                     .arg(log_ins.return_username())
                                     .arg(orderId).arg(car_number).arg(can_passenger).arg(selectedRoleId);
            log_ins.save_logger(logMessage);
        } else {
            // 如果名稱、帳號或密碼為空，顯示錯誤提示
            QMessageBox::warning(this, "警告", "車牌或乘客量不能為空！");
        }
    }
}

