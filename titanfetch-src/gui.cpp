#include "gui.h"
#include "sysinfo.h"

#include <QApplication>
#include <QScreen>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFrame>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QGraphicsDropShadowEffect>
#include <QFontDatabase>
#include <cmath>

// ── Custom animated progress bar ──────────────────────────────────────────────

class TitanBar : public QWidget {
    Q_OBJECT
public:
    TitanBar(long used, long total, const QColor &fill, QWidget *parent = nullptr)
        : QWidget(parent), m_pct(total > 0 ? static_cast<double>(used)/total : 0.0)
        , m_fill(fill)
    {
        setFixedHeight(10);
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QRectF bg(0, 0, width(), height());
        // track
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0x31, 0x32, 0x44));
        p.drawRoundedRect(bg, 5, 5);
        // fill
        if (m_pct > 0.0) {
            const double fw = std::max(10.0, width() * m_pct);
            QLinearGradient grad(0, 0, fw, 0);
            grad.setColorAt(0.0, m_fill.lighter(130));
            grad.setColorAt(1.0, m_fill);
            p.setBrush(grad);
            p.drawRoundedRect(QRectF(0, 0, fw, height()), 5, 5);
        }
    }
private:
    double m_pct;
    QColor m_fill;
};

#include "gui.moc"

// ── Palette ───────────────────────────────────────────────────────────────────
namespace C {
    static const QColor bg     {0x0f, 0x0f, 0x17};
    static const QColor card   {0x1e, 0x1e, 0x2e};
    static const QColor border {0x31, 0x32, 0x44};
    static const QColor text   {0xcd, 0xd6, 0xf4};
    static const QColor muted  {0x6c, 0x70, 0x86};
    static const QColor cyan   {0x89, 0xb4, 0xfa};
    static const QColor purple {0xcb, 0xa6, 0xf7};
    static const QColor green  {0xa6, 0xe3, 0xa1};
    static const QColor yellow {0xf9, 0xe2, 0xaf};
    static const QColor red    {0xf3, 0x8b, 0xa8};
}

// ── Helper: create a styled section card ─────────────────────────────────────

static QWidget* makeCard(const QString &title, QWidget *parent)
{
    auto *card = new QWidget(parent);
    card->setObjectName(QStringLiteral("card"));
    card->setStyleSheet(
        QStringLiteral("QWidget#card { background: #1e1e2e; border-radius: 10px;"
                       " border: 1px solid #313244; }"));

    auto *vl = new QVBoxLayout(card);
    vl->setContentsMargins(16, 12, 16, 14);
    vl->setSpacing(8);

    if (!title.isEmpty()) {
        auto *hdr = new QLabel(title, card);
        hdr->setStyleSheet(
            QStringLiteral("color:#89b4fa; font-size:10px; font-weight:bold;"
                           " letter-spacing:1.5px; border:none; background:transparent;"));
        vl->addWidget(hdr);

        auto *div = new QFrame(card);
        div->setFrameShape(QFrame::HLine);
        div->setStyleSheet(QStringLiteral("background:#313244; max-height:1px; border:none;"));
        vl->addWidget(div);
    }

    return card;
}

// ── Helper: info row ──────────────────────────────────────────────────────────

static void addRow(QVBoxLayout *vl, const QString &icon,
                   const QString &key, const QString &val,
                   const QColor &valColor = C::text)
{
    auto *row = new QHBoxLayout();
    row->setSpacing(8);
    row->setContentsMargins(0,0,0,0);

    auto *ic = new QLabel(icon);
    ic->setFixedWidth(18);
    ic->setStyleSheet(QStringLiteral("color:#89b4fa; font-size:13px;"
                                     " background:transparent; border:none;"));
    row->addWidget(ic);

    auto *k = new QLabel(key.leftJustified(11));
    k->setStyleSheet(QStringLiteral("color:#a6adc8; font-size:11px;"
                                    " background:transparent; border:none;"));
    row->addWidget(k);

    auto *v = new QLabel(val);
    v->setStyleSheet(
        QStringLiteral("color:%1; font-size:11px; background:transparent; border:none;")
            .arg(valColor.name()));
    v->setWordWrap(false);
    v->setTextInteractionFlags(Qt::TextSelectableByMouse);
    row->addWidget(v, 1);

    vl->addLayout(row);
}

// ── Helper: progress row ──────────────────────────────────────────────────────

static void addProgressRow(QVBoxLayout *vl, const QString &icon,
                           const QString &key, const QString &valStr,
                           long used, long total, const QColor &fill)
{
    // label row
    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(8);
    topRow->setContentsMargins(0,0,0,0);

    auto *ic = new QLabel(icon);
    ic->setFixedWidth(18);
    ic->setStyleSheet(QStringLiteral("color:#89b4fa; font-size:13px;"
                                     " background:transparent; border:none;"));
    topRow->addWidget(ic);

    auto *k = new QLabel(key.leftJustified(11));
    k->setStyleSheet(QStringLiteral("color:#a6adc8; font-size:11px;"
                                    " background:transparent; border:none;"));
    topRow->addWidget(k);

    auto *v = new QLabel(valStr);
    v->setStyleSheet(
        QStringLiteral("color:#cdd6f4; font-size:11px; background:transparent; border:none;"));
    topRow->addWidget(v, 1);

    // pct label
    const int pct = total > 0 ? static_cast<int>(used * 100 / total) : 0;
    auto *pctLbl = new QLabel(QStringLiteral("%1%").arg(pct));
    pctLbl->setStyleSheet(
        QStringLiteral("color:%1; font-size:10px; font-weight:bold;"
                       " background:transparent; border:none;").arg(fill.name()));
    topRow->addWidget(pctLbl);

    vl->addLayout(topRow);

    // bar
    auto *bar = new TitanBar(used, total, fill);
    vl->addWidget(bar);
}

// ── Gui ──────────────────────────────────────────────────────────────────────

Gui::Gui(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(QStringLiteral("TitanFetch"));
    setFixedSize(780, 520);

    const SysData d = SysInfo::fetch();

    // ── root container ────────────────────────────────────────────────────────
    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("root"));
    root->setGeometry(0, 0, 780, 520);
    root->setStyleSheet(
        QStringLiteral("QWidget#root { background: #0f0f17; border-radius: 16px;"
                       " border: 1px solid #313244; }"
                       "QLabel { font-family: 'JetBrains Mono', 'Monospace', monospace; }"));

    auto *rootVL = new QVBoxLayout(root);
    rootVL->setContentsMargins(0, 0, 0, 0);
    rootVL->setSpacing(0);

    // ═══════════════════════════════════════════════════════════════════════════
    // HEADER
    // ═══════════════════════════════════════════════════════════════════════════
    auto *header = new QWidget(root);
    header->setFixedHeight(110);
    header->setObjectName(QStringLiteral("header"));
    header->setStyleSheet(
        QStringLiteral("QWidget#header { background: qlineargradient("
                       "x1:0,y1:0,x2:1,y2:1,"
                       "stop:0 #1a1a2e, stop:0.5 #16213e, stop:1 #0f3460);"
                       " border-radius: 0px; border-top-left-radius: 16px;"
                       " border-top-right-radius: 16px;"
                       " border-bottom: 1px solid #313244; }"));

    auto *headerHL = new QHBoxLayout(header);
    headerHL->setContentsMargins(24, 16, 20, 16);
    headerHL->setSpacing(16);

    // Logo text block
    auto *logoLbl = new QLabel(
        QStringLiteral(
            "<pre style='color:#00d4ff; font-size:9px; line-height:1.3;"
            " font-weight:bold; margin:0;'>"
            " /T\\  <br>"
            "/ITI\\ <br>"
            " TAN  "
            "</pre>"),
        header);
    logoLbl->setTextFormat(Qt::RichText);
    logoLbl->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    headerHL->addWidget(logoLbl, 0, Qt::AlignVCenter);

    // vertical sep
    auto *vs1 = new QFrame(header);
    vs1->setFrameShape(QFrame::VLine);
    vs1->setFixedWidth(1);
    vs1->setStyleSheet(QStringLiteral("background:#2a2a4a; border:none;"));
    headerHL->addWidget(vs1);

    // user@host + tagline
    auto *userBlock = new QVBoxLayout();
    userBlock->setSpacing(4);

    auto *userLbl = new QLabel(
        QStringLiteral("<span style='color:#00d4ff; font-size:20px; font-weight:bold;'>%1</span>"
                       "<span style='color:#585b70; font-size:20px;'>@</span>"
                       "<span style='color:#cba6f7; font-size:20px; font-weight:bold;'>%2</span>")
            .arg(d.user, d.host),
        header);
    userLbl->setTextFormat(Qt::RichText);
    userLbl->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    userBlock->addWidget(userLbl);

    // sub-info chips
    auto *chipsHL = new QHBoxLayout();
    chipsHL->setSpacing(8);
    auto makeChip = [&](const QString &txt, const QColor &c) {
        auto *chip = new QLabel(txt, header);
        chip->setStyleSheet(
            QStringLiteral("color:%1; background:rgba(255,255,255,0.05);"
                           " border:1px solid %1; border-radius:4px;"
                           " padding:1px 7px; font-size:10px;").arg(c.name()));
        return chip;
    };
    // find kernel and uptime from fields
    QString kernelVal, uptimeVal, pkgVal;
    for (const auto &[k, v] : d.fields) {
        if (k == QLatin1String("Kernel"))   kernelVal = v;
        if (k == QLatin1String("Uptime"))   uptimeVal = v;
        if (k == QLatin1String("Packages")) pkgVal    = v;
    }
    if (!kernelVal.isEmpty())
        chipsHL->addWidget(makeChip(QStringLiteral(" ") + kernelVal, C::cyan));
    if (!uptimeVal.isEmpty())
        chipsHL->addWidget(makeChip(QStringLiteral(" ") + uptimeVal, C::green));
    if (!pkgVal.isEmpty())
        chipsHL->addWidget(makeChip(QStringLiteral("󰏗 ") + pkgVal, C::yellow));
    chipsHL->addStretch();
    userBlock->addLayout(chipsHL);

    headerHL->addLayout(userBlock, 1);

    // close button
    auto *closeBtn = new QLabel(QStringLiteral("✕"), header);
    closeBtn->setObjectName(QStringLiteral("closeBtn"));
    closeBtn->setFixedSize(28, 28);
    closeBtn->setAlignment(Qt::AlignCenter);
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setStyleSheet(
        QStringLiteral("color:#6c7086; background:rgba(255,255,255,0.04);"
                       " border-radius:14px; font-size:14px; border:1px solid #313244;"));
    closeBtn->installEventFilter(this);
    headerHL->addWidget(closeBtn, 0, Qt::AlignTop);

    rootVL->addWidget(header);

    // ═══════════════════════════════════════════════════════════════════════════
    // BODY — two columns
    // ═══════════════════════════════════════════════════════════════════════════
    auto *body = new QHBoxLayout();
    body->setContentsMargins(16, 14, 16, 14);
    body->setSpacing(12);

    // ── LEFT column: System info ──────────────────────────────────────────────
    auto *leftVL = new QVBoxLayout();
    leftVL->setSpacing(10);

    {
        auto *card = makeCard(QStringLiteral("SYSTEM"), root);
        auto *vl = qobject_cast<QVBoxLayout*>(card->layout());

        for (const auto &[key, val] : d.fields) {
            if (key == QLatin1String("OS"))         addRow(vl, "󰣇", key, val, C::cyan);
            else if (key == QLatin1String("Host"))  addRow(vl, "󰌢", key, val);
            else if (key == QLatin1String("Shell")) addRow(vl, "", key, val, C::green);
            else if (key == QLatin1String("DE"))    addRow(vl, "", key, val, C::purple);
            else if (key == QLatin1String("Terminal")) addRow(vl, "", key, val);
            else if (key == QLatin1String("Resolution")) addRow(vl, "󰍹", key, val);
        }
        leftVL->addWidget(card);
    }

    {
        auto *card = makeCard(QStringLiteral("NETWORK"), root);
        auto *vl = qobject_cast<QVBoxLayout*>(card->layout());
        for (const auto &[key, val] : d.fields) {
            if (key == QLatin1String("Local IP"))
                addRow(vl, "󰩟", key, val, C::cyan);
        }
        leftVL->addWidget(card);
    }

    // colour swatches card
    {
        auto *card = makeCard(QString(), root);
        auto *vl = qobject_cast<QVBoxLayout*>(card->layout());
        auto *swHL = new QHBoxLayout();
        swHL->setSpacing(6);
        const QStringList cols = {
            QStringLiteral("#45475a"), QStringLiteral("#f38ba8"),
            QStringLiteral("#a6e3a1"), QStringLiteral("#f9e2af"),
            QStringLiteral("#89b4fa"), QStringLiteral("#f5c2e7"),
            QStringLiteral("#94e2d5"), QStringLiteral("#cdd6f4"),
        };
        for (const QString &c : cols) {
            auto *sw = new QWidget();
            sw->setFixedSize(26, 16);
            sw->setStyleSheet(
                QStringLiteral("background:%1; border-radius:4px;").arg(c));
            swHL->addWidget(sw);
        }
        swHL->addStretch();
        vl->addLayout(swHL);
        leftVL->addWidget(card);
    }

    leftVL->addStretch();
    body->addLayout(leftVL, 45);

    // ── RIGHT column: Hardware ────────────────────────────────────────────────
    auto *rightVL = new QVBoxLayout();
    rightVL->setSpacing(10);

    {
        auto *card = makeCard(QStringLiteral("HARDWARE"), root);
        auto *vl = qobject_cast<QVBoxLayout*>(card->layout());

        for (const auto &[key, val] : d.fields) {
            if (key == QLatin1String("CPU"))
                addRow(vl, "", key, val, C::yellow);
            else if (key == QLatin1String("GPU"))
                addRow(vl, "󰢮", key, val, C::green);
        }

        vl->addSpacing(6);

        // Memory progress bar
        addProgressRow(vl, "", "Memory",
                       QStringLiteral("%1/%2 MiB").arg(d.memUsedMiB).arg(d.memTotalMiB),
                       d.memUsedMiB, d.memTotalMiB, C::cyan);

        vl->addSpacing(4);

        // Disk progress bar
        addProgressRow(vl, "󰋊", "Disk (/)",
                       QStringLiteral("%1/%2 GiB").arg(d.diskUsedGiB).arg(d.diskTotalGiB),
                       d.diskUsedGiB, d.diskTotalGiB, C::purple);

        vl->addSpacing(4);

        // Battery progress bar
        if (d.batteryPct >= 0) {
            const QColor battColor = d.batteryPct > 50 ? C::green
                                   : d.batteryPct > 20 ? C::yellow
                                   : C::red;
            QString battStatus;
            for (const auto &[k, v] : d.fields)
                if (k == QLatin1String("Battery")) battStatus = v;

            addProgressRow(vl, "", "Battery",
                           battStatus,
                           d.batteryPct, 100, battColor);
        }

        rightVL->addWidget(card);
    }

    // Version footer card
    {
        auto *card = makeCard(QString(), root);
        auto *vl = qobject_cast<QVBoxLayout*>(card->layout());
        auto *fHL = new QHBoxLayout();
        fHL->setSpacing(0);
        auto *versionLbl = new QLabel(
            QStringLiteral("<span style='color:#45475a; font-size:10px;'>"
                           "TitanFetch v1.0.0  •  ArchTitan OS</span>"),
            card);
        versionLbl->setTextFormat(Qt::RichText);
        versionLbl->setStyleSheet(
            QStringLiteral("background:transparent; border:none;"));
        fHL->addStretch();
        fHL->addWidget(versionLbl);
        vl->addLayout(fHL);
        rightVL->addWidget(card);
    }

    rightVL->addStretch();
    body->addLayout(rightVL, 55);

    rootVL->addLayout(body, 1);
}

// ── Painting: gradient background with rounded clip ───────────────────────────

void Gui::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(rect(), 16, 16);
    p.setClipPath(path);
    p.fillRect(rect(), QColor(0x0f, 0x0f, 0x17));
}

// ── Drag & close ──────────────────────────────────────────────────────────────

bool Gui::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj->objectName() == QLatin1String("closeBtn")) {
        if (ev->type() == QEvent::MouseButtonRelease) { close(); return true; }
        if (ev->type() == QEvent::Enter)
            static_cast<QLabel*>(obj)->setStyleSheet(
                QStringLiteral("color:#f38ba8; background:rgba(243,139,168,0.1);"
                               " border-radius:14px; font-size:14px; border:1px solid #f38ba8;"));
        if (ev->type() == QEvent::Leave)
            static_cast<QLabel*>(obj)->setStyleSheet(
                QStringLiteral("color:#6c7086; background:rgba(255,255,255,0.04);"
                               " border-radius:14px; font-size:14px; border:1px solid #313244;"));
    }
    return QWidget::eventFilter(obj, ev);
}

void Gui::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        m_dragPos = ev->globalPosition().toPoint() - frameGeometry().topLeft();
}

void Gui::mouseMoveEvent(QMouseEvent *ev)
{
    if (ev->buttons() & Qt::LeftButton)
        move(ev->globalPosition().toPoint() - m_dragPos);
}
