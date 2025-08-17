#include <QApplication>
#include <QMainWindow>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHeaderView>
#include <QDir>
#include <QMessageBox>

static const QString kLogPath = QStringLiteral("..\\data\\opslog.ndjson"); // relative to qtui\build exe

class OpsWindow : public QMainWindow {
public:
    OpsWindow(QWidget* parent=nullptr) : QMainWindow(parent) {
        auto* central = new QWidget(this);
        auto* vbox = new QVBoxLayout(central);

        auto* title = new QLabel("<b>SENA FAMILY FOUNDATION, LLC — VULTURE LABS OPS CONSOLE (UI)</b>");
        vbox->addWidget(title);

        // Form
        auto* form = new QHBoxLayout();
        cat = new QLineEdit();  cat->setPlaceholderText("Category (Boot, Note, Action, Security, ...)");
        det = new QLineEdit();  det->setPlaceholderText("Detail");
        auto* addBtn = new QPushButton("Add Log");
        form->addWidget(new QLabel("Category:"));
        form->addWidget(cat, 1);
        form->addWidget(new QLabel("Detail:"));
        form->addWidget(det, 2);
        form->addWidget(addBtn);
        vbox->addLayout(form);

        // Table
        table = new QTableWidget(0, 3);
        table->setHorizontalHeaderLabels({"Timestamp","Category","Detail"});
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        vbox->addWidget(table, 1);

        // Buttons
        auto* btnRow = new QHBoxLayout();
        auto* refreshBtn = new QPushButton("Refresh");
        auto* exitBtn = new QPushButton("Exit");
        btnRow->addStretch();
        btnRow->addWidget(refreshBtn);
        btnRow->addWidget(exitBtn);
        vbox->addLayout(btnRow);

        setCentralWidget(central);
        resize(900, 520);

        // Styling: monochrome green vibe
        qApp->setStyleSheet(
            "QWidget { color:#00ff66; background:black; }"
            "QHeaderView::section { background:#003300; color:#00ff66; }"
            "QLineEdit { background:#001a00; border:1px solid #00a050; }"
            "QPushButton { background:#002200; border:1px solid #00a050; padding:4px; }"
            "QTableWidget { gridline-color:#006633; }"
        );

        // Connects (lambdas)
        connect(addBtn, &QPushButton::clicked, this, [this]{ addLog(); });
        connect(refreshBtn, &QPushButton::clicked, this, [this]{ loadLogs(); });
        connect(exitBtn, &QPushButton::clicked, this, [this]{ close(); });

        loadLogs();
    }

private:
    void addLog() {
        QString category = cat->text().trimmed();
        QString detail   = det->text().trimmed();
        if (category.isEmpty()) category = "Note";
        if (detail.isEmpty())   detail = "-";

        QDir().mkpath("..\\data"); // ensure dir exists

        QFile f(kLogPath);
        if (!f.open(QIODevice::Append | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Cannot open log file:\n" + kLogPath);
            return;
        }
        QTextStream out(&f);
        const QString ts = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

        QJsonObject obj{ {"ts", ts}, {"category", category}, {"detail", detail} };
        out << QJsonDocument(obj).toJson(QJsonDocument::Compact) << "\n";
        f.close();

        cat->clear(); det->clear();
        loadLogs();
    }

    void loadLogs() {
        table->setRowCount(0);
        QFile f(kLogPath);
        if (!f.exists()) return;
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

        QList<QByteArray> lines;
        while (!f.atEnd()) {
            QByteArray line = f.readLine().trimmed();
            if (!line.isEmpty()) lines.append(line);
        }
        f.close();

        const int keep = 200;
        int start = qMax(0, lines.size() - keep);
        for (int i = start; i < lines.size(); ++i) {
            QJsonParseError err{};
            QJsonDocument doc = QJsonDocument::fromJson(lines[i], &err);
            if (err.error != QJsonParseError::NoError || !doc.isObject()) continue;
            QJsonObject o = doc.object();
            int row = table->rowCount();
            table->insertRow(row);
            table->setItem(row, 0, new QTableWidgetItem(o.value("ts").toString()));
            table->setItem(row, 1, new QTableWidgetItem(o.value("category").toString()));
            table->setItem(row, 2, new QTableWidgetItem(o.value("detail").toString()));
        }
        table->scrollToBottom();
    }

    QLineEdit* cat{};
    QLineEdit* det{};
    QTableWidget* table{};
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    OpsWindow win;
    win.setWindowTitle("Vulture Labs — Ops Console (Qt UI)");
    win.show();
    return app.exec();
}
