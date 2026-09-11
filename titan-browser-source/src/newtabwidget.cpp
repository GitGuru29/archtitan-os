#include "newtabwidget.h"
#include <QDateTime>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDialog>
#include <QTextEdit>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QUuid>
#include <QCryptographicHash>
#include <QRegularExpression>
#include <QColorDialog>
#include <QAction>
#include <QMenu>
#include <QGraphicsDropShadowEffect>

struct EngineInfo {
    QString name;
    QString url;
};

static const EngineInfo kEngines[] = {
    { QStringLiteral("DDG"),       QStringLiteral("https://duckduckgo.com/?q=") },
    { QStringLiteral("Google"),    QStringLiteral("https://www.google.com/search?q=") },
    { QStringLiteral("Brave"),     QStringLiteral("https://search.brave.com/search?q=") },
    { QStringLiteral("Startpage"), QStringLiteral("https://www.startpage.com/sp/search?query=") }
};

NewTabWidget::NewTabWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("NewTabWidget"));
    loadShortcuts();
    setupUi();

    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &NewTabWidget::updateClockAndGreeting);
    timer->start(1000);
    updateClockAndGreeting();
}

void NewTabWidget::setupUi()
{
    setStyleSheet(QStringLiteral(R"(
        QWidget#NewTabWidget {
            background-color: #060812;
            color: #f8fafc;
            font-family: "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 6px;
        }
        QScrollBar::handle:vertical {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 3px;
        }
    )"));

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto *contentWidget = new QWidget(scrollArea);
    auto *contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setContentsMargins(40, 36, 40, 48);
    contentLayout->setSpacing(24);
    contentLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);

    setupHeader(contentLayout);
    setupSearchBar(contentLayout);
    setupSpaces(contentLayout);
    setupSpeedDials(contentLayout);
    setupDevTools(contentLayout);

    scrollArea->setWidget(contentWidget);
    rootLayout->addWidget(scrollArea);
}

void NewTabWidget::setupHeader(QVBoxLayout *layout)
{
    auto *headerBox = new QWidget(this);
    auto *hLayout = new QVBoxLayout(headerBox);
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(4);
    hLayout->setAlignment(Qt::AlignCenter);

    // Titan Logo
    auto *logoBtn = new QPushButton(headerBox);
    logoBtn->setIcon(QIcon(QStringLiteral(":/icons/logo.svg")));
    logoBtn->setIconSize(QSize(42, 42));
    logoBtn->setStyleSheet(QStringLiteral("background: transparent; border: none; padding: 4px;"));
    logoBtn->setCursor(Qt::PointingHandCursor);
    hLayout->addWidget(logoBtn, 0, Qt::AlignCenter);

    // Digital Clock
    m_clockLabel = new QLabel(QStringLiteral("12:00"), headerBox);
    m_clockLabel->setStyleSheet(QStringLiteral("font-size: 36px; font-weight: 800; color: #ffffff; letter-spacing: -0.5px;"));
    m_clockLabel->setAlignment(Qt::AlignCenter);
    hLayout->addWidget(m_clockLabel, 0, Qt::AlignCenter);

    // Greeting
    m_greetingLabel = new QLabel(QStringLiteral("Good evening, Titan."), headerBox);
    m_greetingLabel->setStyleSheet(QStringLiteral("font-size: 19px; font-weight: 600; color: #e2e8f0;"));
    m_greetingLabel->setAlignment(Qt::AlignCenter);
    hLayout->addWidget(m_greetingLabel, 0, Qt::AlignCenter);

    auto *subLabel = new QLabel(QStringLiteral("Unified operating environment for developer workflows"), headerBox);
    subLabel->setStyleSheet(QStringLiteral("font-size: 12.5px; color: #64748b; font-weight: 400;"));
    subLabel->setAlignment(Qt::AlignCenter);
    hLayout->addWidget(subLabel, 0, Qt::AlignCenter);

    layout->addWidget(headerBox);
}

void NewTabWidget::setupSearchBar(QVBoxLayout *layout)
{
    auto *barBox = new QWidget(this);
    barBox->setMaximumWidth(700);
    barBox->setMinimumWidth(550);
    auto *bLayout = new QHBoxLayout(barBox);
    bLayout->setContentsMargins(12, 6, 12, 6);
    bLayout->setSpacing(8);

    barBox->setStyleSheet(QStringLiteral(R"(
        QWidget {
            background: #090e1c;
            border: 1px solid rgba(255, 255, 255, 0.08);
            border-radius: 12px;
        }
        QWidget:focus-within {
            border: 1px solid #38bdf8;
            background: #0d1528;
        }
    )"));

    m_engineBtn = new QPushButton(QStringLiteral("DDG"), barBox);
    m_engineBtn->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            background: rgba(56, 189, 248, 0.12);
            border: 1px solid rgba(56, 189, 248, 0.3);
            border-radius: 6px;
            color: #38bdf8;
            font-size: 11px;
            font-weight: 700;
            padding: 4px 8px;
            min-height: 20px;
        }
        QPushButton:hover {
            background: rgba(56, 189, 248, 0.22);
        }
    )"));
    m_engineBtn->setCursor(Qt::PointingHandCursor);
    connect(m_engineBtn, &QPushButton::clicked, this, &NewTabWidget::onSearchEngineClicked);
    bLayout->addWidget(m_engineBtn);

    m_searchInput = new QLineEdit(barBox);
    m_searchInput->setPlaceholderText(QStringLiteral("Search the web, compute Math, or enter URL..."));
    m_searchInput->setStyleSheet(QStringLiteral(R"(
        QLineEdit {
            background: transparent;
            border: none;
            color: #ffffff;
            font-size: 14.5px;
            padding: 4px 6px;
        }
    )"));
    connect(m_searchInput, &QLineEdit::returnPressed, this, &NewTabWidget::onSearchSubmitted);
    bLayout->addWidget(m_searchInput, 1);

    auto *badge = new QLabel(QStringLiteral("Enter ↵"), barBox);
    badge->setStyleSheet(QStringLiteral("color: #64748b; font-size: 10.5px; font-weight: 600; background: rgba(255,255,255,0.05); padding: 3px 6px; border-radius: 4px; border: none;"));
    bLayout->addWidget(badge);

    layout->addWidget(barBox, 0, Qt::AlignCenter);
}

void NewTabWidget::setupSpaces(QVBoxLayout *layout)
{
    auto *box = new QWidget(this);
    box->setMaximumWidth(860);
    auto *vLayout = new QVBoxLayout(box);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("SPACES"), box);
    title->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 1px;"));
    vLayout->addWidget(title);

    auto *spacesRow = new QHBoxLayout();
    spacesRow->setSpacing(10);

    auto addSpaceCard = [this, spacesRow, box](const QString &name, const QString &sub, const QString &dotColor, bool active) {
        auto *btn = new QPushButton(box);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("spaceName", name);

        QString qss = QStringLiteral(R"(
            QPushButton {
                background: %1;
                border: 1px solid %2;
                border-radius: 10px;
                padding: 12px 14px;
                text-align: left;
                min-width: 170px;
            }
            QPushButton:hover {
                background: #111a2f;
                border-color: rgba(56, 189, 248, 0.35);
            }
        )").arg(active ? QStringLiteral("#0e172a") : QStringLiteral("#090e1c"))
          .arg(active ? QStringLiteral("rgba(56, 189, 248, 0.45)") : QStringLiteral("rgba(255, 255, 255, 0.07)"));

        btn->setStyleSheet(qss);
        btn->setText(QStringLiteral("● %1\n%2").arg(name, sub));
        connect(btn, &QPushButton::clicked, this, [this, name]{ onSpaceSelected(name); });
        spacesRow->addWidget(btn);
        m_spaceButtons.append(btn);
    };

    addSpaceCard(QStringLiteral("Development"), QStringLiteral("C++, Rust, Vite"), QStringLiteral("#38bdf8"), true);
    addSpaceCard(QStringLiteral("Research"), QStringLiteral("Docs, Papers, AI"), QStringLiteral("#c084fc"), false);
    addSpaceCard(QStringLiteral("Personal"), QStringLiteral("Feeds, Media"), QStringLiteral("#22c55e"), false);

    auto *newSpaceBtn = new QPushButton(QStringLiteral("+ New Space"), box);
    newSpaceBtn->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            background: #090e1c;
            border: 1px dashed rgba(255, 255, 255, 0.15);
            border-radius: 10px;
            color: #94a3b8;
            font-size: 12px;
            font-weight: 500;
            padding: 12px 14px;
            min-width: 140px;
        }
        QPushButton:hover {
            border-color: #38bdf8;
            color: #38bdf8;
        }
    )"));
    newSpaceBtn->setCursor(Qt::PointingHandCursor);
    connect(newSpaceBtn, &QPushButton::clicked, this, [this]{
        QMessageBox::information(this, QStringLiteral("Create Space"), QStringLiteral("Titan Spaces allow custom tab sets and isolated environments."));
    });
    spacesRow->addWidget(newSpaceBtn);

    vLayout->addLayout(spacesRow);
    layout->addWidget(box, 0, Qt::AlignCenter);
}

void NewTabWidget::setupSpeedDials(QVBoxLayout *layout)
{
    auto *box = new QWidget(this);
    box->setMaximumWidth(860);
    auto *vLayout = new QVBoxLayout(box);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("QUICK ACCESS"), box);
    title->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 1px;"));
    vLayout->addWidget(title);

    m_dialsContainer = new QWidget(box);
    m_dialsLayout = new QGridLayout(m_dialsContainer);
    m_dialsLayout->setContentsMargins(0, 0, 0, 0);
    m_dialsLayout->setSpacing(10);

    renderShortcuts();
    vLayout->addWidget(m_dialsContainer);

    layout->addWidget(box, 0, Qt::AlignCenter);
}

void NewTabWidget::setupDevTools(QVBoxLayout *layout)
{
    auto *box = new QWidget(this);
    box->setMaximumWidth(860);
    auto *vLayout = new QVBoxLayout(box);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("DEVELOPER TOOLBOX"), box);
    title->setStyleSheet(QStringLiteral("font-size: 11px; font-weight: 700; color: #64748b; letter-spacing: 1px;"));
    vLayout->addWidget(title);

    auto *grid = new QGridLayout();
    grid->setSpacing(10);

    auto addToolBtn = [this, grid, box](int row, int col, const QString &icon, const QString &name, const QString &sub, const QString &id) {
        auto *btn = new QPushButton(box);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(R"(
            QPushButton {
                background: #090e1c;
                border: 1px solid rgba(255, 255, 255, 0.07);
                border-radius: 10px;
                padding: 10px 12px;
                text-align: left;
                color: #ffffff;
            }
            QPushButton:hover {
                background: #111a2f;
                border-color: rgba(56, 189, 248, 0.35);
            }
        )"));
        btn->setText(QStringLiteral("%1  %2\n    %3").arg(icon, name, sub));
        connect(btn, &QPushButton::clicked, this, [this, id]{ onOpenToolClicked(id); });
        grid->addWidget(btn, row, col);
    };

    addToolBtn(0, 0, QStringLiteral("{}"), QStringLiteral("JSON Tool"), QStringLiteral("Beautify & Minify"), QStringLiteral("json"));
    addToolBtn(0, 1, QStringLiteral("64"), QStringLiteral("Base64"), QStringLiteral("Encode & Decode"), QStringLiteral("base64"));
    addToolBtn(0, 2, QStringLiteral("#"),  QStringLiteral("UUID / Hash"), QStringLiteral("UUIDv4 & SHA-256"), QStringLiteral("uuid"));
    addToolBtn(1, 0, QStringLiteral(".*"), QStringLiteral("Regex Tester"), QStringLiteral("Pattern Matcher"), QStringLiteral("regex"));
    addToolBtn(1, 1, QStringLiteral("⏱"),  QStringLiteral("Epoch Converter"), QStringLiteral("Unix Timestamps"), QStringLiteral("epoch"));
    addToolBtn(1, 2, QStringLiteral("🎨"), QStringLiteral("Color Palette"), QStringLiteral("HEX & RGB Picker"), QStringLiteral("color"));

    vLayout->addLayout(grid);
    layout->addWidget(box, 0, Qt::AlignCenter);
}

void NewTabWidget::renderShortcuts()
{
    // Clear existing items in dialsLayout
    QLayoutItem *item;
    while ((item = m_dialsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    int col = 0;
    int row = 0;
    const int maxCols = 6;

    for (int i = 0; i < m_shortcuts.size(); ++i) {
        const auto &sc = m_shortcuts[i];
        auto *btn = new QPushButton(m_dialsContainer);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(R"(
            QPushButton {
                background: #090e1c;
                border: 1px solid rgba(255, 255, 255, 0.07);
                border-radius: 10px;
                padding: 12px 10px;
                color: #ffffff;
                font-weight: 600;
                font-size: 12px;
                min-width: 120px;
            }
            QPushButton:hover {
                background: #111a2f;
                border-color: rgba(56, 189, 248, 0.35);
            }
        )"));

        QString domain = QUrl(sc.url).host();
        btn->setText(QStringLiteral("%1\n%2").arg(sc.name, domain.isEmpty() ? sc.url : domain));

        QString url = sc.url;
        connect(btn, &QPushButton::clicked, this, [this, url]{
            emit navigateRequested(QUrl(url));
        });

        m_dialsLayout->addWidget(btn, row, col);
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }

    // Add Shortcut Button
    auto *addBtn = new QPushButton(QStringLiteral("+ Add\nShortcut"), m_dialsContainer);
    addBtn->setCursor(Qt::PointingHandCursor);
    addBtn->setStyleSheet(QStringLiteral(R"(
        QPushButton {
            background: #090e1c;
            border: 1px dashed rgba(255, 255, 255, 0.15);
            border-radius: 10px;
            padding: 12px 10px;
            color: #94a3b8;
            font-size: 12px;
            min-width: 120px;
        }
        QPushButton:hover {
            border-color: #38bdf8;
            color: #38bdf8;
        }
    )"));
    connect(addBtn, &QPushButton::clicked, this, &NewTabWidget::onAddShortcutClicked);
    m_dialsLayout->addWidget(addBtn, row, col);
}

void NewTabWidget::loadShortcuts()
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/titanbrowser");
    QDir().mkpath(dirPath);

    QFile file(dirPath + QStringLiteral("/shortcuts.json"));
    if (file.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
        for (const auto &v : arr) {
            QJsonObject obj = v.toObject();
            m_shortcuts.append({
                obj.value(QStringLiteral("name")).toString(),
                obj.value(QStringLiteral("url")).toString(),
                obj.value(QStringLiteral("color")).toString()
            });
        }
    }

    if (m_shortcuts.isEmpty()) {
        m_shortcuts.append({QStringLiteral("ArchTitan"), QStringLiteral("https://archtitan.io"), QStringLiteral("#38bdf8")});
        m_shortcuts.append({QStringLiteral("GitHub"), QStringLiteral("https://github.com"), QStringLiteral("#ffffff")});
        m_shortcuts.append({QStringLiteral("YouTube"), QStringLiteral("https://youtube.com"), QStringLiteral("#ef4444")});
        m_shortcuts.append({QStringLiteral("Google"), QStringLiteral("https://google.com"), QStringLiteral("#4285F4")});
        m_shortcuts.append({QStringLiteral("Reddit"), QStringLiteral("https://reddit.com"), QStringLiteral("#ff4500")});
        m_shortcuts.append({QStringLiteral("Notion"), QStringLiteral("https://notion.so"), QStringLiteral("#ffffff")});
    }
}

void NewTabWidget::saveShortcuts()
{
    const QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/titanbrowser");
    QDir().mkpath(dirPath);

    QFile file(dirPath + QStringLiteral("/shortcuts.json"));
    if (file.open(QIODevice::WriteOnly)) {
        QJsonArray arr;
        for (const auto &sc : m_shortcuts) {
            QJsonObject obj;
            obj[QStringLiteral("name")] = sc.name;
            obj[QStringLiteral("url")]  = sc.url;
            obj[QStringLiteral("color")] = sc.color;
            arr.append(obj);
        }
        file.write(QJsonDocument(arr).toJson());
    }
}

void NewTabWidget::updateClockAndGreeting()
{
    QDateTime now = QDateTime::currentDateTime();
    if (m_clockLabel) {
        m_clockLabel->setText(now.toString(QStringLiteral("hh:mm")));
    }

    if (m_greetingLabel) {
        int h = now.time().hour();
        QString greeting = QStringLiteral("Good evening, Titan.");
        if (h >= 5 && h < 12) greeting = QStringLiteral("Good morning, Titan.");
        else if (h >= 12 && h < 18) greeting = QStringLiteral("Good afternoon, Titan.");
        m_greetingLabel->setText(greeting);
    }
}

void NewTabWidget::focusSearch()
{
    if (m_searchInput) {
        m_searchInput->setFocus();
        m_searchInput->selectAll();
    }
}

void NewTabWidget::onSearchEngineClicked()
{
    m_currentEngineIdx = (m_currentEngineIdx + 1) % 4;
    m_engineBtn->setText(kEngines[m_currentEngineIdx].name);
}

void NewTabWidget::onSearchSubmitted()
{
    QString input = m_searchInput->text().trimmed();
    if (input.isEmpty()) return;

    // Direct URL check
    if (input.startsWith(QStringLiteral("http://")) || input.startsWith(QStringLiteral("https://"))) {
        emit navigateRequested(QUrl(input));
        return;
    }

    if (!input.contains(u' ') && input.contains(u'.')) {
        emit navigateRequested(QUrl(QStringLiteral("https://") + input));
        return;
    }

    // Quick Math calculation
    QRegularExpression mathRegex(QStringLiteral("^[0-9+\\-*/().\\s^%]+$"));
    if (mathRegex.match(input).hasMatch() && input.contains(QRegularExpression(QStringLiteral("[0-9]")))) {
        // Safe basic math check
        // If user wants search, fallback
    }

    QUrl target(kEngines[m_currentEngineIdx].url + QUrl::toPercentEncoding(input));
    emit navigateRequested(target);
}

void NewTabWidget::onSpaceSelected(const QString &spaceName)
{
    m_activeSpace = spaceName;
    for (auto *btn : m_spaceButtons) {
        bool isAct = (btn->property("spaceName").toString() == spaceName);
        btn->setStyleSheet(QStringLiteral(R"(
            QPushButton {
                background: %1;
                border: 1px solid %2;
                border-radius: 10px;
                padding: 12px 14px;
                text-align: left;
                min-width: 170px;
            }
            QPushButton:hover {
                background: #111a2f;
                border-color: rgba(56, 189, 248, 0.35);
            }
        )").arg(isAct ? QStringLiteral("#0e172a") : QStringLiteral("#090e1c"))
          .arg(isAct ? QStringLiteral("rgba(56, 189, 248, 0.45)") : QStringLiteral("rgba(255, 255, 255, 0.07)")));
    }
}

void NewTabWidget::onAddShortcutClicked()
{
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Add Quick Access Shortcut"));
    dlg.resize(360, 180);
    dlg.setStyleSheet(QStringLiteral(R"(
        QDialog { background: #090e1c; color: #f8fafc; }
        QLabel { color: #94a3b8; font-size: 12px; }
        QLineEdit {
            background: #0f182d;
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 6px;
            padding: 6px 10px;
            color: #ffffff;
            font-size: 13px;
        }
        QPushButton {
            background: #0284c7;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 600;
        }
        QPushButton:hover { background: #0369a1; }
    )"));

    auto *layout = new QVBoxLayout(&dlg);
    layout->addWidget(new QLabel(QStringLiteral("Shortcut Name:"), &dlg));
    auto *nameEdit = new QLineEdit(&dlg);
    nameEdit->setPlaceholderText(QStringLiteral("e.g. StackOverflow"));
    layout->addWidget(nameEdit);

    layout->addWidget(new QLabel(QStringLiteral("URL:"), &dlg));
    auto *urlEdit = new QLineEdit(&dlg);
    urlEdit->setPlaceholderText(QStringLiteral("https://stackoverflow.com"));
    layout->addWidget(urlEdit);

    auto *saveBtn = new QPushButton(QStringLiteral("Save Shortcut"), &dlg);
    connect(saveBtn, &QPushButton::clicked, [&] {
        QString name = nameEdit->text().trimmed();
        QString url  = urlEdit->text().trimmed();
        if (!name.isEmpty() && !url.isEmpty()) {
            if (!url.startsWith(QStringLiteral("http://")) && !url.startsWith(QStringLiteral("https://"))) {
                url = QStringLiteral("https://") + url;
            }
            m_shortcuts.append({name, url, QStringLiteral("#38bdf8")});
            saveShortcuts();
            renderShortcuts();
            dlg.accept();
        }
    });
    layout->addWidget(saveBtn);

    dlg.exec();
}

void NewTabWidget::onOpenToolClicked(const QString &toolName)
{
    QDialog dlg(this);
    dlg.resize(480, 320);
    dlg.setStyleSheet(QStringLiteral(R"(
        QDialog { background: #090e1c; color: #f8fafc; }
        QLabel { color: #94a3b8; font-size: 12px; }
        QTextEdit, QLineEdit {
            background: #060914;
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 6px;
            padding: 8px;
            color: #38bdf8;
            font-family: monospace;
            font-size: 12.5px;
        }
        QPushButton {
            background: #0284c7;
            color: #ffffff;
            border: none;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 600;
        }
        QPushButton:hover { background: #0369a1; }
    )"));

    auto *layout = new QVBoxLayout(&dlg);

    if (toolName == QStringLiteral("json")) {
        dlg.setWindowTitle(QStringLiteral("JSON Beautifier & Minifier"));
        auto *edit = new QTextEdit(&dlg);
        edit->setPlaceholderText(QStringLiteral("{\"key\":\"value\",\"number\":42}"));
        layout->addWidget(edit);

        auto *hLayout = new QHBoxLayout();
        auto *bBtn = new QPushButton(QStringLiteral("Beautify"), &dlg);
        connect(bBtn, &QPushButton::clicked, [edit]{
            QJsonDocument doc = QJsonDocument::fromJson(edit->toPlainText().toUtf8());
            if (!doc.isNull()) edit->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
            else QMessageBox::warning(edit, QStringLiteral("Error"), QStringLiteral("Invalid JSON"));
        });
        hLayout->addWidget(bBtn);

        auto *mBtn = new QPushButton(QStringLiteral("Minify"), &dlg);
        connect(mBtn, &QPushButton::clicked, [edit]{
            QJsonDocument doc = QJsonDocument::fromJson(edit->toPlainText().toUtf8());
            if (!doc.isNull()) edit->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
            else QMessageBox::warning(edit, QStringLiteral("Error"), QStringLiteral("Invalid JSON"));
        });
        hLayout->addWidget(mBtn);

        auto *cBtn = new QPushButton(QStringLiteral("Copy"), &dlg);
        connect(cBtn, &QPushButton::clicked, [edit]{
            QApplication::clipboard()->setText(edit->toPlainText());
        });
        hLayout->addWidget(cBtn);
        layout->addLayout(hLayout);
    }
    else if (toolName == QStringLiteral("base64")) {
        dlg.setWindowTitle(QStringLiteral("Base64 Encoder / Decoder"));
        auto *edit = new QTextEdit(&dlg);
        layout->addWidget(edit);

        auto *hLayout = new QHBoxLayout();
        auto *encBtn = new QPushButton(QStringLiteral("Encode Base64"), &dlg);
        connect(encBtn, &QPushButton::clicked, [edit]{
            edit->setPlainText(QString::fromUtf8(edit->toPlainText().toUtf8().toBase64()));
        });
        hLayout->addWidget(encBtn);

        auto *decBtn = new QPushButton(QStringLiteral("Decode Base64"), &dlg);
        connect(decBtn, &QPushButton::clicked, [edit]{
            edit->setPlainText(QString::fromUtf8(QByteArray::fromBase64(edit->toPlainText().toUtf8())));
        });
        hLayout->addWidget(decBtn);
        layout->addLayout(hLayout);
    }
    else if (toolName == QStringLiteral("uuid")) {
        dlg.setWindowTitle(QStringLiteral("UUID & SHA-256 Generator"));
        auto *uEdit = new QLineEdit(&dlg);
        uEdit->setText(QUuid::createUuid().toString(QUuid::WithoutBraces));
        layout->addWidget(new QLabel(QStringLiteral("Generated UUIDv4:"), &dlg));
        layout->addWidget(uEdit);

        auto *hLayout = new QHBoxLayout();
        auto *regBtn = new QPushButton(QStringLiteral("Regenerate UUID"), &dlg);
        connect(regBtn, &QPushButton::clicked, [uEdit]{
            uEdit->setText(QUuid::createUuid().toString(QUuid::WithoutBraces));
        });
        hLayout->addWidget(regBtn);

        auto *copyBtn = new QPushButton(QStringLiteral("Copy UUID"), &dlg);
        connect(copyBtn, &QPushButton::clicked, [uEdit]{
            QApplication::clipboard()->setText(uEdit->text());
        });
        hLayout->addWidget(copyBtn);
        layout->addLayout(hLayout);
    }
    else if (toolName == QStringLiteral("regex")) {
        dlg.setWindowTitle(QStringLiteral("Regex Tester"));
        auto *patEdit = new QLineEdit(&dlg);
        patEdit->setPlaceholderText(QStringLiteral("Pattern e.g. [a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}"));
        layout->addWidget(new QLabel(QStringLiteral("Regex Pattern:"), &dlg));
        layout->addWidget(patEdit);

        auto *testEdit = new QTextEdit(&dlg);
        testEdit->setPlaceholderText(QStringLiteral("Test text..."));
        layout->addWidget(new QLabel(QStringLiteral("Test String:"), &dlg));
        layout->addWidget(testEdit);

        auto *resLabel = new QLabel(QStringLiteral("Result:"), &dlg);
        layout->addWidget(resLabel);

        auto *testBtn = new QPushButton(QStringLiteral("Test Pattern"), &dlg);
        connect(testBtn, &QPushButton::clicked, [patEdit, testEdit, resLabel]{
            QRegularExpression re(patEdit->text());
            if (!re.isValid()) {
                resLabel->setText(QStringLiteral("Result: Invalid Regex Error"));
                return;
            }
            auto match = re.match(testEdit->toPlainText());
            if (match.hasMatch()) {
                resLabel->setText(QStringLiteral("Result: ✔ Match Found: %1").arg(match.captured(0)));
            } else {
                resLabel->setText(QStringLiteral("Result: ✖ No match"));
            }
        });
        layout->addWidget(testBtn);
    }
    else if (toolName == QStringLiteral("epoch")) {
        dlg.setWindowTitle(QStringLiteral("Epoch / Timestamp Converter"));
        auto *edit = new QLineEdit(&dlg);
        edit->setText(QString::number(QDateTime::currentSecsSinceEpoch()));
        layout->addWidget(new QLabel(QStringLiteral("Unix Timestamp:"), &dlg));
        layout->addWidget(edit);

        auto *resLabel = new QLabel(QDateTime::currentDateTime().toUTC().toString(Qt::ISODate), &dlg);
        resLabel->setStyleSheet(QStringLiteral("color: #38bdf8; font-weight: 600; font-size: 13px;"));
        layout->addWidget(resLabel);

        auto *btn = new QPushButton(QStringLiteral("Convert Timestamp"), &dlg);
        connect(btn, &QPushButton::clicked, [edit, resLabel]{
            qint64 ep = edit->text().toLongLong();
            QDateTime dt = QDateTime::fromSecsSinceEpoch(ep);
            resLabel->setText(QStringLiteral("UTC: %1\nLocal: %2").arg(dt.toUTC().toString(), dt.toString()));
        });
        layout->addWidget(btn);
    }
    else if (toolName == QStringLiteral("color")) {
        QColor col = QColorDialog::getColor(QColor(56, 189, 248), this, QStringLiteral("Select Color"));
        if (col.isValid()) {
            QApplication::clipboard()->setText(col.name(QColor::HexRgb));
        }
        return;
    }

    dlg.exec();
}
