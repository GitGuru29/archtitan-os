#include "gui.h"
#include "sysinfo.h"

#include <QApplication>
#include <QScreen>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPixmap>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QTimer>
#include <cmath>
#include <random>
#include <vector>

// ── Color palette ─────────────────────────────────────────────────────────────
namespace C {
    static const QColor bg      {0x0d, 0x0d, 0x14};
    static const QColor surface {0x13, 0x13, 0x1f};
    static const QColor card    {0x1a, 0x1a, 0x2e};
    static const QColor border  {0x2a, 0x2a, 0x3e};
    static const QColor text    {0xcd, 0xd6, 0xf4};
    static const QColor muted   {0x45, 0x47, 0x5a};
    static const QColor subtle  {0x58, 0x5b, 0x70};
    static const QColor subtext {0xa6, 0xad, 0xc8};
    static const QColor cyan    {0x89, 0xdc, 0xeb};
    static const QColor blue    {0x89, 0xb4, 0xfa};
    static const QColor green   {0xa6, 0xe3, 0xa1};
    static const QColor yellow  {0xf9, 0xe2, 0xaf};
    static const QColor red     {0xf3, 0x8b, 0xa8};
    static const QColor purple  {0xcb, 0xa6, 0xf7};
    static const QColor teal    {0x94, 0xe2, 0xd5};
}

// ═════════════════════════════════════════════════════════════════════════════
// VisualizerWidget — animated audio-bar chart
// ═════════════════════════════════════════════════════════════════════════════
class VisualizerWidget : public QWidget {
    Q_OBJECT

public:
    explicit VisualizerWidget(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_rng(std::random_device{}())
    {
        setFixedSize(175, 55);
        setAttribute(Qt::WA_NoSystemBackground);

        const int N = 24;
        m_bars.resize(N);
        m_targets.resize(N);
        std::uniform_int_distribution<int> d(10, 50);
        for (int i = 0; i < N; ++i) {
            m_bars[i]    = d(m_rng);
            m_targets[i] = d(m_rng);
        }

        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &VisualizerWidget::tick);
        timer->start(75);
    }

private slots:
    void tick()
    {
        std::uniform_int_distribution<int> d(8, 52);
        for (int i = 0; i < static_cast<int>(m_bars.size()); ++i) {
            // Smooth easing toward target
            m_bars[i] += (m_targets[i] - m_bars[i]) / 4;
            if (std::abs(m_bars[i] - m_targets[i]) < 3)
                m_targets[i] = d(m_rng);
        }
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const int n      = static_cast<int>(m_bars.size());
        const int barW   = 4;
        const int gap    = 4;
        const int totalW = n * (barW + gap) - gap;
        const int startX = (width() - totalW) / 2;
        const int H      = height();

        for (int i = 0; i < n; ++i) {
            const int h = std::max(4, m_bars[i]);
            const int x = startX + i * (barW + gap);
            const int y = H - h;

            // gradient: cyan top → blue bottom
            QLinearGradient grad(x, y, x, H);
            grad.setColorAt(0.0, QColor(0x89, 0xdc, 0xeb, 240));
            grad.setColorAt(0.6, QColor(0x89, 0xb4, 0xfa, 200));
            grad.setColorAt(1.0, QColor(0x74, 0xc7, 0xec, 120));

            p.setPen(Qt::NoPen);
            p.setBrush(grad);
            p.drawRoundedRect(x, y, barW, h, 2, 2);
        }
    }

private:
    std::vector<int> m_bars;
    std::vector<int> m_targets;
    std::mt19937     m_rng;
};

// ═════════════════════════════════════════════════════════════════════════════
// TitanBar — slim gradient progress bar
// ═════════════════════════════════════════════════════════════════════════════
class TitanBar : public QWidget {
public:
    TitanBar(long used, long total, const QColor &fill, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_pct(total > 0 ? static_cast<double>(used) / static_cast<double>(total) : 0.0)
        , m_fill(fill)
    {
        setFixedHeight(6);
        setAttribute(Qt::WA_NoSystemBackground);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);

        // Track
        p.setBrush(QColor(0x31, 0x32, 0x44));
        p.drawRoundedRect(rect(), 3, 3);

        // Fill
        if (m_pct > 0.0) {
            const double fw = std::max(6.0, width() * m_pct);
            QLinearGradient grad(0, 0, fw, 0);
            grad.setColorAt(0.0, m_fill.lighter(140));
            grad.setColorAt(1.0, m_fill);
            p.setBrush(grad);
            p.drawRoundedRect(QRectF(0, 0, fw, height()), 3, 3);
        }
    }

private:
    double m_pct;
    QColor m_fill;
};

// Must include moc after all Q_OBJECT classes defined in this .cpp
#include "gui.moc"

// ═════════════════════════════════════════════════════════════════════════════
// Gui constructor
// ═════════════════════════════════════════════════════════════════════════════
Gui::Gui(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowTitle(QStringLiteral("TitanFetch"));
    setFixedSize(900, 560);

    const SysData d = SysInfo::fetch();

    // ── Extract header fields ──────────────────────────────────────────────────
    QString kernelVal, uptimeVal, pkgNum, pkgSuffix;
    for (const auto &[k, v] : d.fields) {
        if (k == QLatin1String("Kernel"))   kernelVal = v;
        if (k == QLatin1String("Uptime"))   uptimeVal = v;
        if (k == QLatin1String("Packages")) {
            const int paren = v.indexOf(QLatin1Char('('));
            if (paren > 0) {
                pkgNum    = v.left(paren).trimmed();
                pkgSuffix = v.mid(paren + 1).remove(QLatin1Char(')'));
            } else {
                pkgNum = v;
            }
        }
    }

    // ── Global stylesheet ─────────────────────────────────────────────────────
    const QString monoFont =
        QStringLiteral("'JetBrains Mono', 'Noto Mono', 'DejaVu Sans Mono', monospace");

    // ── Root container ────────────────────────────────────────────────────────
    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("root"));
    root->setGeometry(0, 0, 900, 560);
    root->setStyleSheet(
        QStringLiteral("QWidget#root { background: #0d0d14; border-radius: 16px;"
                       " border: 1px solid #2a2a3e; }"
                       "QLabel { font-family: %1; }").arg(monoFont));

    auto *rootVL = new QVBoxLayout(root);
    rootVL->setContentsMargins(0, 0, 0, 0);
    rootVL->setSpacing(0);

    // ═════════════════════════════════════════════════════════════════════════
    // HEADER
    // ═════════════════════════════════════════════════════════════════════════
    auto *header = new QWidget(root);
    header->setFixedHeight(88);
    header->setObjectName(QStringLiteral("hdr"));
    header->setStyleSheet(
        QStringLiteral("QWidget#hdr {"
                       " background: #151523;"
                       " border-top-left-radius: 16px; border-top-right-radius: 16px;"
                       " border-bottom: 1px solid #2a2a3e;"
                       "}"));

    auto *hdrHL = new QHBoxLayout(header);
    hdrHL->setContentsMargins(22, 0, 22, 0);
    hdrHL->setSpacing(16);

    // ── ArchTitan logo badge ───────────────────────────────────────────────────
    auto *badge = new QLabel(header);
    badge->setFixedSize(54, 54);
    {
        QPixmap px(QStringLiteral("/home/msfvenom/custom-os-build/assets/logo.png"));
        if (!px.isNull())
            badge->setPixmap(px.scaled(54, 54, Qt::KeepAspectRatio,
                                        Qt::SmoothTransformation));
        else
            badge->setText(QStringLiteral("AT"));
    }
    badge->setAlignment(Qt::AlignCenter);
    badge->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    hdrHL->addWidget(badge, 0, Qt::AlignVCenter);

    // ── Title block (wrapped in QWidget so AlignVCenter works) ───────────────
    auto *titleW = new QWidget(header);
    titleW->setStyleSheet(QStringLiteral("background:transparent;"));
    auto *titleBlock = new QVBoxLayout(titleW);
    titleBlock->setSpacing(4);
    titleBlock->setContentsMargins(0, 0, 0, 0);

    auto *titleLbl = new QLabel(titleW);
    titleLbl->setTextFormat(Qt::RichText);
    titleLbl->setText(
        QStringLiteral("<span style='color:#cdd6f4; font-size:23px;"
                       " font-weight:bold;'>Titan</span>"
                       "<span style='color:#89dceb; font-size:23px;"
                       " font-weight:bold;'>Fetch</span>"));
    titleLbl->setStyleSheet(QStringLiteral("background:transparent; border:none;"));
    titleBlock->addWidget(titleLbl);

    auto *subtitleLbl = new QLabel(
        QStringLiteral("%1@%2").arg(d.user, d.host), titleW);
    subtitleLbl->setStyleSheet(
        QStringLiteral("color:#585b70; font-size:12px; background:transparent; border:none;"));
    titleBlock->addWidget(subtitleLbl);

    hdrHL->addWidget(titleW, 1, Qt::AlignVCenter);

    // ── Visualizer ────────────────────────────────────────────────────────────
    auto *viz = new VisualizerWidget(header);
    hdrHL->addWidget(viz, 0, Qt::AlignVCenter);

    rootVL->addWidget(header);

    // ═════════════════════════════════════════════════════════════════════════
    // STATS BAR  (KERNEL | UPTIME | PACKAGES)
    // ═════════════════════════════════════════════════════════════════════════
    auto *statsBar = new QWidget(root);
    statsBar->setFixedHeight(70);
    statsBar->setObjectName(QStringLiteral("stats"));
    statsBar->setStyleSheet(
        QStringLiteral("QWidget#stats { background: #111120;"
                       " border-bottom: 1px solid #2a2a3e; }"));

    auto *statsHL = new QHBoxLayout(statsBar);
    statsHL->setContentsMargins(0, 0, 0, 0);
    statsHL->setSpacing(0);

    // Lambda: build one stat cell
    auto makeStatCell = [&](QWidget *parent, const QString &label, QWidget *valueW) {
        auto *cell = new QWidget(parent);
        auto *vl = new QVBoxLayout(cell);
        vl->setContentsMargins(26, 10, 26, 10);
        vl->setSpacing(4);

        auto *lbl = new QLabel(label, cell);
        lbl->setStyleSheet(
            QStringLiteral("color:#45475a; font-size:9px; font-weight:bold;"
                           " letter-spacing:1.8px; background:transparent; border:none;"));
        vl->addWidget(lbl);

        valueW->setParent(cell);
        vl->addWidget(valueW);
        return cell;
    };

    // Lambda: vertical divider
    auto makeDivider = [&](QWidget *parent) {
        auto *f = new QFrame(parent);
        f->setFrameShape(QFrame::VLine);
        f->setFixedWidth(1);
        f->setStyleSheet(QStringLiteral("background:#2a2a3e; border:none;"));
        return f;
    };

    // KERNEL
    {
        auto *v = new QLabel(kernelVal.isEmpty() ? QStringLiteral("Unknown") : kernelVal, statsBar);
        v->setStyleSheet(QStringLiteral("color:#cdd6f4; font-size:15px; font-weight:600;"
                                         " background:transparent; border:none;"));
        statsHL->addWidget(makeStatCell(statsBar, QStringLiteral("KERNEL"), v), 1);
        statsHL->addWidget(makeDivider(statsBar));
    }

    // UPTIME
    {
        auto *v = new QLabel(uptimeVal.isEmpty() ? QStringLiteral("0m") : uptimeVal, statsBar);
        v->setStyleSheet(QStringLiteral("color:#a6e3a1; font-size:15px; font-weight:600;"
                                         " background:transparent; border:none;"));
        statsHL->addWidget(makeStatCell(statsBar, QStringLiteral("UPTIME"), v), 1);
        statsHL->addWidget(makeDivider(statsBar));
    }

    // PACKAGES
    {
        // Number (white bold) + suffix (muted) side by side
        auto *pkgRow = new QWidget(statsBar);
        auto *pkgHL  = new QHBoxLayout(pkgRow);
        pkgHL->setContentsMargins(0, 0, 0, 0);
        pkgHL->setSpacing(6);

        auto *numLbl = new QLabel(pkgNum.isEmpty() ? QStringLiteral("0") : pkgNum, pkgRow);
        numLbl->setStyleSheet(QStringLiteral("color:#cdd6f4; font-size:15px; font-weight:600;"
                                              " background:transparent; border:none;"));
        pkgHL->addWidget(numLbl);

        if (!pkgSuffix.isEmpty()) {
            auto *sufLbl = new QLabel(pkgSuffix, pkgRow);
            sufLbl->setStyleSheet(QStringLiteral("color:#45475a; font-size:13px;"
                                                  " background:transparent; border:none;"));
            pkgHL->addWidget(sufLbl, 0, Qt::AlignBottom);
        }
        pkgHL->addStretch();

        statsHL->addWidget(makeStatCell(statsBar, QStringLiteral("PACKAGES"), pkgRow), 1);
    }

    rootVL->addWidget(statsBar);

    // ═════════════════════════════════════════════════════════════════════════
    // BODY  (left: SYSTEM | right: HARDWARE)
    // ═════════════════════════════════════════════════════════════════════════
    auto *body = new QHBoxLayout();
    body->setContentsMargins(16, 14, 16, 10);
    body->setSpacing(12);

    // ── Helper: make a section card with a ● TITLE header ─────────────────────
    auto makeSection = [&](const QString &title) -> QPair<QWidget *, QVBoxLayout *> {
        auto *card = new QWidget(root);
        card->setObjectName(QStringLiteral("card"));
        card->setStyleSheet(
            QStringLiteral("QWidget#card { background: #1a1a2e; border-radius: 10px;"
                           " border: 1px solid #2a2a3e; }"));

        auto *vl = new QVBoxLayout(card);
        vl->setContentsMargins(20, 18, 20, 20);
        vl->setSpacing(0);

        // ● TITLE header row
        if (!title.isEmpty()) {
            auto *hRow = new QHBoxLayout();
            hRow->setSpacing(7);
            hRow->setContentsMargins(0, 0, 0, 0);

            auto *dot = new QLabel(QStringLiteral("●"), card);
            dot->setStyleSheet(
                QStringLiteral("color:#89dceb; font-size:9px;"
                               " background:transparent; border:none;"));
            hRow->addWidget(dot, 0, Qt::AlignVCenter);

            auto *hdrTxt = new QLabel(title, card);
            hdrTxt->setStyleSheet(
                QStringLiteral("color:#cdd6f4; font-size:11px; font-weight:bold;"
                               " letter-spacing:2px; background:transparent; border:none;"));
            hRow->addWidget(hdrTxt);
            hRow->addStretch();
            vl->addLayout(hRow);
            vl->addSpacing(14);
        }

        return {card, vl};
    };

    // ── Helper: info row (key left, value right) ───────────────────────────────
    auto addInfoRow = [&](QVBoxLayout *vl, const QString &key,
                           const QString &val, const QColor &valColor = C::text) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 10, 0, 10);
        row->setSpacing(8);

        auto *k = new QLabel(key);
        k->setStyleSheet(QStringLiteral("color:#6c7086; font-size:13px;"
                                         " background:transparent; border:none;"));
        row->addWidget(k);
        row->addStretch();

        auto *v = new QLabel(val);
        v->setStyleSheet(
            QStringLiteral("color:%1; font-size:13px;"
                           " background:transparent; border:none;").arg(valColor.name()));
        v->setTextInteractionFlags(Qt::TextSelectableByMouse);
        row->addWidget(v);

        vl->addLayout(row);
    };

    // ── Helper: progress row (key left, value right, bar below) ───────────────
    auto addProgressRow = [&](QVBoxLayout *vl, const QString &key,
                               const QString &valStr,
                               long used, long total, const QColor &fill) {
        auto *row = new QHBoxLayout();
        row->setContentsMargins(0, 12, 0, 4);
        row->setSpacing(8);

        auto *k = new QLabel(key);
        k->setStyleSheet(QStringLiteral("color:#6c7086; font-size:13px;"
                                         " background:transparent; border:none;"));
        row->addWidget(k);
        row->addStretch();

        auto *v = new QLabel(valStr);
        v->setStyleSheet(QStringLiteral("color:#cdd6f4; font-size:13px;"
                                         " background:transparent; border:none;"));
        row->addWidget(v);

        vl->addLayout(row);
        vl->addSpacing(6);
        vl->addWidget(new TitanBar(used, total, fill));
        vl->addSpacing(8);
    };

    // ── LEFT: SYSTEM card ─────────────────────────────────────────────────────
    auto [leftCard, leftVL] = makeSection(QStringLiteral("SYSTEM"));

    for (const auto &[key, val] : d.fields) {
        if      (key == QLatin1String("OS"))
            addInfoRow(leftVL, key, val, C::text);
        else if (key == QLatin1String("Host"))
            addInfoRow(leftVL, key, val, C::text);
        else if (key == QLatin1String("Shell"))
            addInfoRow(leftVL, key, val, C::cyan);
        else if (key == QLatin1String("Resolution"))
            addInfoRow(leftVL, key, val, C::text);
        else if (key == QLatin1String("DE"))
            addInfoRow(leftVL, key, val, C::cyan);
        else if (key == QLatin1String("Local IP"))
            addInfoRow(leftVL, key, val, C::text);
    }

    body->addWidget(leftCard, 44);

    // ── RIGHT: HARDWARE card ──────────────────────────────────────────────────
    auto [rightCard, rightVL] = makeSection(QStringLiteral("HARDWARE"));

    for (const auto &[key, val] : d.fields) {
        if (key == QLatin1String("CPU"))
            addInfoRow(rightVL, key, val, C::subtle);
        else if (key == QLatin1String("GPU"))
            addInfoRow(rightVL, key, val, C::subtle);
    }

    rightVL->addSpacing(10);

    addProgressRow(rightVL, QStringLiteral("Memory"),
                   QStringLiteral("%1 / %2 MiB").arg(d.memUsedMiB).arg(d.memTotalMiB),
                   d.memUsedMiB, d.memTotalMiB, C::cyan);

    addProgressRow(rightVL, QStringLiteral("Disk (/)"),
                   QStringLiteral("%1 / %2 GiB").arg(d.diskUsedGiB).arg(d.diskTotalGiB),
                   d.diskUsedGiB, d.diskTotalGiB, C::red);

    if (d.batteryPct >= 0) {
        const QColor battColor = d.batteryPct > 50 ? C::green
                               : d.batteryPct > 20 ? C::yellow
                               : C::red;
        addProgressRow(rightVL, QStringLiteral("Battery"),
                       QStringLiteral("%1%").arg(d.batteryPct),
                       d.batteryPct, 100, battColor);
    }

    body->addWidget(rightCard, 56);

    rootVL->addLayout(body);
    rootVL->addStretch(1);

    // ═════════════════════════════════════════════════════════════════════════
    // FOOTER
    // ═════════════════════════════════════════════════════════════════════════
    auto *footer = new QWidget(root);
    footer->setFixedHeight(48);
    footer->setObjectName(QStringLiteral("ftr"));
    footer->setStyleSheet(
        QStringLiteral("QWidget#ftr {"
                       " background: #111120;"
                       " border-top: 1px solid #2a2a3e;"
                       " border-bottom-left-radius: 16px;"
                       " border-bottom-right-radius: 16px;"
                       "}"));

    auto *footerHL = new QHBoxLayout(footer);
    footerHL->setContentsMargins(22, 0, 22, 0);
    footerHL->setSpacing(8);

    auto *accentLbl = new QLabel(QStringLiteral("Workspace accent"), footer);
    accentLbl->setStyleSheet(
        QStringLiteral("color:#45475a; font-size:10px; background:transparent; border:none;"));
    footerHL->addWidget(accentLbl, 0, Qt::AlignVCenter);

    footerHL->addSpacing(6);

    // Live Hyprland workspace accent dots (parsed from hyprland.conf at startup)
    for (const QColor &col : d.accentColors) {
        auto *dot = new QWidget(footer);
        dot->setFixedSize(20, 20);
        dot->setStyleSheet(
            QStringLiteral("background:%1; border-radius:10px;").arg(col.name()));
        footerHL->addWidget(dot, 0, Qt::AlignVCenter);
    }

    footerHL->addStretch();

    auto *verLbl = new QLabel(QStringLiteral("TitanFetch v1.0.0  •  ArchTitan OS"), footer);
    verLbl->setStyleSheet(
        QStringLiteral("color:#45475a; font-size:10px; background:transparent; border:none;"));
    footerHL->addWidget(verLbl, 0, Qt::AlignVCenter);

    rootVL->addWidget(footer);
}

// ═════════════════════════════════════════════════════════════════════════════
// Paint: clip to rounded rect so translucent bg stays crisp
// ═════════════════════════════════════════════════════════════════════════════
void Gui::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QPainterPath clip;
    clip.addRoundedRect(rect(), 16, 16);
    p.setClipPath(clip);
    p.fillRect(rect(), QColor(0x0d, 0x0d, 0x14));
}

// ═════════════════════════════════════════════════════════════════════════════
// Drag-to-move (close button removed — use keyboard shortcut to quit)
// ═════════════════════════════════════════════════════════════════════════════
bool Gui::eventFilter(QObject *obj, QEvent *ev)
{
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
