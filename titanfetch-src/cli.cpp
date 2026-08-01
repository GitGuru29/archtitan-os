#include "cli.h"
#include "sysinfo.h"

#include <QTextStream>
#include <QStringList>
#include <QStringConverter>
#include <algorithm>
#include <sys/ioctl.h>
#include <unistd.h>

void Cli::run()
{
    QTextStream out(stdout);
    out.setEncoding(QStringConverter::Utf8);

    const SysData data = SysInfo::fetch();
    const auto &fields = data.fields;

    // 19-Line ArchTitan OS ASCII Logo — A(+) Bold Blue, T(#) Bold White; each line 47 visible chars
    const QStringList logo = {
        QStringLiteral("\033[1;34m                          +                    \033[0m"),
        QStringLiteral("\033[1;34m                         +++                   \033[0m"),
        QStringLiteral("\033[1;34m                        +++++                  \033[0m"),
        QStringLiteral("\033[1;34m                       +++++++                 \033[0m"),
        QStringLiteral("\033[1;34m                      +++++++++                \033[0m"),
        QStringLiteral("\033[1;34m                     ++++    ++++              \033[0m"),
        QStringLiteral("\033[1;34m                    *++++    ++++*             \033[0m"),
        QStringLiteral("\033[1;34m                   +++++      +++++            \033[0m"),
        QStringLiteral("\033[1;34m                  +++++\033[0m \033[1;37m#######\033[0m \033[1;34m+++++          \033[0m"),
        QStringLiteral("\033[1;34m                 *++++\033[0m \033[1;37m#########\033[0m \033[1;34m++++*         \033[0m"),
        QStringLiteral("\033[1;34m                +++++\033[0m    \033[1;37m#####\033[0m    \033[1;34m+++++        \033[0m"),
        QStringLiteral("\033[1;34m               ++++\033[0m       \033[1;37m###\033[0m       \033[1;34m++++       \033[0m"),
        QStringLiteral("\033[1;34m              ++++++\033[0m      \033[1;37m###\033[0m      \033[1;34m++++++      \033[0m"),
        QStringLiteral("\033[1;34m             ++++++\033[0m       \033[1;37m###\033[0m       \033[1;34m++++++     \033[0m"),
        QStringLiteral("\033[1;34m            ++++++\033[0m        \033[1;37m###\033[0m        \033[1;34m++++++    \033[0m"),
        QStringLiteral("\033[1;34m           *++++\033[0m           \033[1;37m#\033[0m           \033[1;34m++++*   \033[0m"),
        QStringLiteral("\033[1;34m          +++\033[0m              \033[1;37m#\033[0m              \033[1;34m+++  \033[0m"),
        QStringLiteral("\033[1;34m         ++\033[0m                \033[1;37m#\033[0m                \033[1;34m++ \033[0m"),
        QStringLiteral("\033[1;34m        +\033[0m                  \033[1;37m#\033[0m                  \033[1;34m+\033[0m")
    };

    // Fallback padding matches max logo visual width (47 characters)
    const QString pad(47, u' ');
    const int totalRightLines = 2 + fields.size() + 2; // header(2) + fields + space + swatches
    const int maxLines = std::max(static_cast<int>(logo.size()), totalRightLines);

    // Detect terminal width — truncate long values in split-screen / narrow terminals
    int termWidth = 220;
    struct winsize ws{};
    if (::ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0)
        termWidth = ws.ws_col;
    // logo(47) + sep(2) + key(11) + ": "(2) = 62 chars consumed before value starts
    const int valueWidth = termWidth - 62;

    out << u'\n';

    for (int i = 0; i < maxLines; ++i) {
        const QString &logoLine = (i < logo.size()) ? logo.at(i) : pad;
        out << logoLine << QStringLiteral("  ");

        if (i == 0) {
            out << QStringLiteral("\033[1;34m%1\033[0m\033[1;37m@\033[0m\033[1;34m%2\033[0m")
                       .arg(data.user, data.host);
        } else if (i == 1) {
            out << QStringLiteral("\033[34m")
                << QString(data.user.size() + 1 + data.host.size(), u'\u2500')
                << QStringLiteral("\033[0m");
        } else if (i - 2 < fields.size()) {
            const auto &field = fields.at(i - 2);
            const QString key = field.first.leftJustified(11, u' ');
            QString val = field.second;
            if (valueWidth > 3 && val.size() > valueWidth)
                val = val.left(valueWidth - 1) + QStringLiteral("\u2026");
            out << QStringLiteral("\033[1;34m") << key
                << QStringLiteral("\033[0m: ") << val;
        } else if (i == 2 + fields.size() + 1) {
            for (int c = 30; c < 38; ++c)
                out << QStringLiteral("\033[%1m\u2588\u2588\u2588\033[0m").arg(c);
        }

        out << u'\n';
    }

    out << u'\n';
    out.flush();
}