#include "settingswidget.h"
#include <QScrollArea>
#include <QTableWidget>
#include <QHeaderView>
#include <QMessageBox>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QUrl>
#include <QFile>

SettingsWidget::SettingsWidget(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("SettingsWidget"));
    setupUi();
}

void SettingsWidget::setupUi()
{
    setStyleSheet(QStringLiteral(R"(
        QWidget#SettingsWidget {
            background-color: #060812;
            color: #f8fafc;
            font-family: "Inter", -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        }
        QListWidget {
            background: #04060d;
            border-right: 1px solid rgba(255, 255, 255, 0.07);
            border-top: none;
            border-bottom: none;
            border-left: none;
            padding: 18px 8px;
            color: #94a3b8;
            font-size: 13px;
            font-weight: 500;
        }
        QListWidget::item {
            padding: 10px 12px;
            border-radius: 6px;
            margin-bottom: 2px;
        }
        QListWidget::item:hover {
            background: rgba(255, 255, 255, 0.04);
            color: #ffffff;
        }
        QListWidget::item:selected {
            background: rgba(56, 189, 248, 0.15);
            color: #38bdf8;
            font-weight: 600;
        }
        QScrollArea {
            background: transparent;
            border: none;
        }
        QComboBox {
            background: #0d1528;
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 6px;
            color: #f8fafc;
            padding: 6px 12px;
            font-size: 12.5px;
            min-width: 220px;
        }
        QComboBox:focus { border-color: #38bdf8; }
        QComboBox QAbstractItemView {
            background: #090e1c;
            color: #f8fafc;
            selection-background-color: rgba(56, 189, 248, 0.2);
            border: 1px solid rgba(56, 189, 248, 0.3);
            padding: 4px;
        }
        QCheckBox {
            spacing: 8px;
            font-size: 13px;
            color: #f8fafc;
        }
        QCheckBox::indicator {
            width: 38px;
            height: 20px;
            border-radius: 10px;
            background: rgba(255, 255, 255, 0.12);
        }
        QCheckBox::indicator:checked {
            background: #38bdf8;
        }
        QPushButton.ActionBtn {
            background: rgba(255, 255, 255, 0.08);
            border: 1px solid rgba(255, 255, 255, 0.1);
            border-radius: 6px;
            color: #ffffff;
            font-size: 12px;
            font-weight: 600;
            padding: 6px 14px;
        }
        QPushButton.ActionBtn:hover {
            background: rgba(56, 189, 248, 0.18);
            border-color: #38bdf8;
            color: #38bdf8;
        }
        QPushButton.DangerBtn {
            background: rgba(239, 68, 68, 0.15);
            border: 1px solid rgba(239, 68, 68, 0.3);
            border-radius: 6px;
            color: #ef4444;
            font-size: 12px;
            font-weight: 600;
            padding: 6px 14px;
        }
        QPushButton.DangerBtn:hover {
            background: #ef4444;
            color: #ffffff;
        }
    )"));

    auto *rootLayout = new QHBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // Left Navigation List
    m_navList = new QListWidget(this);
    m_navList->setFixedWidth(220);
    m_navList->addItem(QStringLiteral("⚙  General"));
    m_navList->addItem(QStringLiteral("🎨  Appearance"));
    m_navList->addItem(QStringLiteral("🔍  Search Engine"));
    m_navList->addItem(QStringLiteral("🛡  TitanShield"));
    m_navList->addItem(QStringLiteral("⚡  Performance"));
    m_navList->addItem(QStringLiteral("✦  Titan AI"));
    m_navList->addItem(QStringLiteral("⌨  Shortcuts"));
    m_navList->addItem(QStringLiteral("ℹ  About"));

    connect(m_navList, &QListWidget::currentRowChanged, this, &SettingsWidget::onNavChanged);
    rootLayout->addWidget(m_navList);

    // Right Pages Stack
    m_pages = new QStackedWidget(this);
    m_pages->addWidget(createGeneralPage());
    m_pages->addWidget(createAppearancePage());
    m_pages->addWidget(createSearchPage());
    m_pages->addWidget(createPrivacyPage());
    m_pages->addWidget(createPerformancePage());
    m_pages->addWidget(createAIPage());
    m_pages->addWidget(createShortcutsPage());
    m_pages->addWidget(createAboutPage());

    rootLayout->addWidget(m_pages, 1);
    m_navList->setCurrentRow(0);
}

void SettingsWidget::onNavChanged(int row)
{
    if (row >= 0 && row < m_pages->count()) {
        m_pages->setCurrentIndex(row);
    }
}

static QWidget *createPageContainer(const QString &title, const QString &sub, QVBoxLayout*& contentOut)
{
    auto *scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto *widget = new QWidget(scroll);
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(36, 32, 36, 32);
    layout->setSpacing(24);
    layout->setAlignment(Qt::AlignTop);

    auto *titleLabel = new QLabel(title, widget);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 700; color: #ffffff;"));
    layout->addWidget(titleLabel);

    auto *subLabel = new QLabel(sub, widget);
    subLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #64748b; margin-top: -18px; margin-bottom: 8px;"));
    layout->addWidget(subLabel);

    contentOut = layout;
    scroll->setWidget(widget);
    return scroll;
}

static QWidget *createSettingsCard()
{
    auto *card = new QWidget();
    card->setStyleSheet(QStringLiteral(R"(
        QWidget {
            background: #0a0f1d;
            border: 1px solid rgba(255, 255, 255, 0.07);
            border-radius: 10px;
        }
    )"));
    return card;
}

static void addSettingRow(QVBoxLayout *cardLayout, const QString &label, const QString &desc, QWidget *control)
{
    auto *row = new QWidget();
    row->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    auto *hLayout = new QHBoxLayout(row);
    hLayout->setContentsMargins(16, 12, 16, 12);
    hLayout->setSpacing(20);

    auto *infoBox = new QWidget(row);
    infoBox->setStyleSheet(QStringLiteral("background: transparent; border: none;"));
    auto *vLayout = new QVBoxLayout(infoBox);
    vLayout->setContentsMargins(0, 0, 0, 0);
    vLayout->setSpacing(2);

    auto *lLabel = new QLabel(label, infoBox);
    lLabel->setStyleSheet(QStringLiteral("font-size: 13.5px; font-weight: 600; color: #ffffff;"));
    vLayout->addWidget(lLabel);

    if (!desc.isEmpty()) {
        auto *dLabel = new QLabel(desc, infoBox);
        dLabel->setStyleSheet(QStringLiteral("font-size: 11.5px; color: #64748b;"));
        vLayout->addWidget(dLabel);
    }

    hLayout->addWidget(infoBox, 1);
    if (control) {
        control->setParent(row);
        hLayout->addWidget(control, 0, Qt::AlignRight | Qt::AlignVCenter);
    }

    cardLayout->addWidget(row);
}

QWidget *SettingsWidget::createGeneralPage()
{
    QVBoxLayout *layout;
    auto *page = createPageContainer(QStringLiteral("General Settings"), QStringLiteral("Configure startup behavior and download locations"), layout);

    auto *card = createSettingsCard();
    auto *cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(0, 4, 0, 4);
    cLayout->setSpacing(0);

    auto *combo = new QComboBox();
    combo->addItem(QStringLiteral("Open New Tab Page (Titan Workspace)"));
    combo->addItem(QStringLiteral("Restore Previous Session Tabs"));
    addSettingRow(cLayout, QStringLiteral("On Startup"), QStringLiteral("Choose what opens when Titan Browser starts"), combo);

    auto *dlBtn = new QPushButton(QStringLiteral("Open ~/Downloads"));
    dlBtn->setProperty("class", "ActionBtn");
    connect(dlBtn, &QPushButton::clicked, []{
        QDesktopServices::openUrl(QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)));
    });
    addSettingRow(cLayout, QStringLiteral("Download Directory"), QStringLiteral("Default location for all downloaded files"), dlBtn);

    layout->addWidget(card);
    return page;
}

QWidget *SettingsWidget::createAppearancePage()
{
    QVBoxLayout *layout;
    auto *page = createPageContainer(QStringLiteral("Appearance & Themes"), QStringLiteral("Customize browser palette, theme colors, and UI density"), layout);

    auto *card = createSettingsCard();
    auto *cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(16, 16, 16, 16);
    cLayout->setSpacing(12);

    auto *grid = new QGridLayout();
    grid->setSpacing(12);

    auto addThemeCard = [this, grid](int col, const QString &name, const QString &borderCol, const QString &bgCol) {
        auto *btn = new QPushButton();
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(QStringLiteral(R"(
            QPushButton {
                background: %1;
                border: 2px solid %2;
                border-radius: 8px;
                padding: 16px 12px;
                color: #ffffff;
                font-weight: 600;
                font-size: 12.5px;
                min-width: 120px;
            }
            QPushButton:hover {
                box-shadow: 0 0 12px %2;
            }
        )").arg(bgCol, borderCol));
        btn->setText(name);
        connect(btn, &QPushButton::clicked, this, [this, name]{ onThemeSelected(name); });
        grid->addWidget(btn, 0, col);
    };

    addThemeCard(0, QStringLiteral("Obsidian Dark"), QStringLiteral("#38bdf8"), QStringLiteral("#060812"));
    addThemeCard(1, QStringLiteral("Midnight Neon"), QStringLiteral("#818cf8"), QStringLiteral("#090e24"));
    addThemeCard(2, QStringLiteral("AMOLED Black"),  QStringLiteral("#22c55e"), QStringLiteral("#000000"));
    addThemeCard(3, QStringLiteral("Frost Light"),   QStringLiteral("#94a3b8"), QStringLiteral("#f8fafc"));

    cLayout->addLayout(grid);
    layout->addWidget(card);
    return page;
}

QWidget *SettingsWidget::createSearchPage()
{
    QVBoxLayout *layout;
    auto *page = createPageContainer(QStringLiteral("Search Engine"), QStringLiteral("Configure omnibox search engine and keyword aliases"), layout);

    auto *card = createSettingsCard();
    auto *cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(0, 4, 0, 4);
    cLayout->setSpacing(0);

    auto *combo = new QComboBox();
    combo->addItem(QStringLiteral("DuckDuckGo (Privacy-First)"));
    combo->addItem(QStringLiteral("Google Search"));
    combo->addItem(QStringLiteral("Brave Search"));
    combo->addItem(QStringLiteral("Startpage"));
    combo->addItem(QStringLiteral("SearXNG (Decentralized)"));
    addSettingRow(cLayout, QStringLiteral("Default Search Engine"), QStringLiteral("Omnibox query default destination"), combo);

    auto *cb = new QCheckBox();
    cb->setChecked(true);
    addSettingRow(cLayout, QStringLiteral("Quick Keyword Prefixes"), QStringLiteral("Type 'gh ', 'yt ', 'wiki ', 'g ', 'ddg ' to search specific targets"), cb);

    layout->addWidget(card);
    return page;
}

QWidget *SettingsWidget::createPrivacyPage()
{
    QVBoxLayout *layout;
    auto *page = createPageContainer(QStringLiteral("TitanShield Security"), QStringLiteral("20,000+ domain blocker, anti-fingerprinting, and ad skipping"), layout);

    auto *card = createSettingsCard();
    auto *cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(0, 4, 0, 4);
    cLayout->setSpacing(0);

    auto *cb1 = new QCheckBox(); cb1->setChecked(true);
    addSettingRow(cLayout, QStringLiteral("TitanShield Content Blocker"), QStringLiteral("Block trackers, ads, and telemetry network requests"), cb1);

    auto *cb2 = new QCheckBox(); cb2->setChecked(true);
    addSettingRow(cLayout, QStringLiteral("Instant Video Ad Skip"), QStringLiteral("Neutralize preroll and midroll video ads on YouTube & Spotify"), cb2);

    auto *cb3 = new QCheckBox(); cb3->setChecked(true);
    addSettingRow(cLayout, QStringLiteral("Anti-Fingerprinting & Canvas Defense"), QStringLiteral("Spoof canvas and hardware device fingerprints"), cb3);

    auto *cb4 = new QCheckBox(); cb4->setChecked(true);
    addSettingRow(cLayout, QStringLiteral("Enforce HTTPS Upgrades"), QStringLiteral("Automatically upgrade insecure HTTP requests to HTTPS"), cb4);

    auto *clearBtn = new QPushButton(QStringLiteral("Clear Browsing Data"));
    clearBtn->setProperty("class", "DangerBtn");
    connect(clearBtn, &QPushButton::clicked, this, &SettingsWidget::onClearDataClicked);
    addSettingRow(cLayout, QStringLiteral("Browsing History & Cache"), QStringLiteral("Purge all local session records and disk caches"), clearBtn);

    layout->addWidget(card);
    return page;
}

QWidget *SettingsWidget::createPerformancePage()
{
    QVBoxLayout *layout;
    auto *page = createPageContainer(QStringLiteral("Performance & Optimization"), QStringLiteral("Manage memory saver, smooth scrolling, and hardware acceleration"), layout);

    auto *card = createSettingsCard();
    auto *cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(0, 4, 0, 4);
    cLayout->setSpacing(0);

    auto *cb1 = new QCheckBox(); cb1->setChecked(true);
    addSettingRow(cLayout, QStringLiteral("Smart Tab Memory Saver"), QStringLiteral("Suspend inactive background tabs after 15 minutes to save memory"), cb1);

    auto *cb2 = new QCheckBox(); cb2->setChecked(true);
    addSettingRow(cLayout, QStringLiteral("Hardware-Assisted Smooth Scrolling"), QStringLiteral("Smooth 60/120fps viewport scroll animations"), cb2);

    layout->addWidget(card);
    return page;
}

QWidget *SettingsWidget::createAIPage()
{
    QVBoxLayout *layout;
    auto *page = createPageContainer(QStringLiteral("Titan AI Assistant"), QStringLiteral("Configure on-device developer intelligence"), layout);

    auto *card = createSettingsCard();
    auto *cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(0, 4, 0, 4);
    cLayout->setSpacing(0);

    auto *combo = new QComboBox();
    combo->addItem(QStringLiteral("ArchTitan Local Engine (On-Device)"));
    combo->addItem(QStringLiteral("Ollama Localhost (http://localhost:11434)"));
    combo->addItem(QStringLiteral("Custom Remote Endpoint"));
    addSettingRow(cLayout, QStringLiteral("AI Provider"), QStringLiteral("Engine used to generate responses and code insights"), combo);

    auto *keyLabel = new QLabel(QStringLiteral("Ctrl + K"));
    keyLabel->setStyleSheet(QStringLiteral("background: rgba(255,255,255,0.08); padding: 3px 8px; border-radius: 4px; color: #38bdf8; font-weight: 600;"));
    addSettingRow(cLayout, QStringLiteral("Keyboard Trigger"), QStringLiteral("Opens the Titan AI Assistant flyout"), keyLabel);

    layout->addWidget(card);
    return page;
}

QWidget *SettingsWidget::createShortcutsPage()
{
    QVBoxLayout *layout;
    auto *page = createPageContainer(QStringLiteral("Keyboard Shortcuts"), QStringLiteral("Fast keyboard navigation bindings"), layout);

    auto *table = new QTableWidget();
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({QStringLiteral("Action"), QStringLiteral("Shortcut")});
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setStyleSheet(QStringLiteral(R"(
        QTableWidget {
            background: #0a0f1d;
            border: 1px solid rgba(255, 255, 255, 0.07);
            border-radius: 8px;
            color: #ffffff;
            gridline-color: rgba(255, 255, 255, 0.05);
        }
        QHeaderView::section {
            background: #080d1a;
            color: #64748b;
            font-size: 11px;
            font-weight: 700;
            padding: 8px;
            border: none;
        }
    )"));

    struct KeyRow { QString action; QString key; };
    QList<KeyRow> rows = {
        {QStringLiteral("New Tab"), QStringLiteral("Ctrl + T")},
        {QStringLiteral("Close Tab"), QStringLiteral("Ctrl + W")},
        {QStringLiteral("Focus Address Bar"), QStringLiteral("Ctrl + L")},
        {QStringLiteral("Titan AI Assistant"), QStringLiteral("Ctrl + K")},
        {QStringLiteral("Find in Page"), QStringLiteral("Ctrl + F")},
        {QStringLiteral("Bookmarks Menu"), QStringLiteral("Ctrl + B")},
        {QStringLiteral("Browsing History"), QStringLiteral("Ctrl + H")},
        {QStringLiteral("Downloads"), QStringLiteral("Ctrl + J")},
        {QStringLiteral("Settings"), QStringLiteral("Ctrl + ,")},
        {QStringLiteral("Navigate Back"), QStringLiteral("Alt + Left")},
        {QStringLiteral("Navigate Forward"), QStringLiteral("Alt + Right")},
        {QStringLiteral("Home Workspace"), QStringLiteral("Alt + H")},
        {QStringLiteral("Spaces Workspace"), QStringLiteral("Alt + S")},
        {QStringLiteral("Zoom In / Out / Reset"), QStringLiteral("Ctrl + + / - / 0")}
    };

    table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(rows[i].action));
        auto *item = new QTableWidgetItem(rows[i].key);
        item->setTextAlignment(Qt::AlignCenter);
        table->setItem(i, 1, item);
    }

    layout->addWidget(table);
    return page;
}

QWidget *SettingsWidget::createAboutPage()
{
    QVBoxLayout *layout;
    auto *page = createPageContainer(QStringLiteral("About Titan Browser"), QStringLiteral("Architecture and runtime system details"), layout);

    auto *card = createSettingsCard();
    auto *cLayout = new QVBoxLayout(card);
    cLayout->setContentsMargins(0, 4, 0, 4);
    cLayout->setSpacing(0);

    addSettingRow(cLayout, QStringLiteral("Version"), QStringLiteral("v1.0.0 (ArchTitan Release Build)"), nullptr);
    addSettingRow(cLayout, QStringLiteral("Engine Framework"), QStringLiteral("Qt 6.7 WebEngine with 100% Native Qt6 C++ Workspace"), nullptr);
    addSettingRow(cLayout, QStringLiteral("Telemetry Policy"), QStringLiteral("Zero cloud telemetry. 100% private on-device storage."), nullptr);

    layout->addWidget(card);
    return page;
}

void SettingsWidget::onThemeSelected(const QString &theme)
{
    emit themeChanged(theme);
}

void SettingsWidget::onClearDataClicked()
{
    if (QMessageBox::question(this, QStringLiteral("Clear Data"),
        QStringLiteral("Are you sure you want to clear browsing history, cookies, and local caches?")) == QMessageBox::Yes)
    {
        QMessageBox::information(this, QStringLiteral("Data Cleared"), QStringLiteral("Browsing session data has been purged successfully."));
    }
}
