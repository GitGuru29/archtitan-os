#include "cli.h"
#include "sysinfo.h"

#include <QTextStream>
#include <QStringList>
#include <QStringConverter>
#include <algorithm>

void Cli::run()
{
    QTextStream out(stdout);
    out.setEncoding(QStringConverter::Utf8);

    const SysData data = SysInfo::fetch();
    const auto &fields = data.fields;

    // High-density Arch logo matching user specifications
    const QStringList logo = {
        QStringLiteral("\033[1;36m                  -\\               \033[0m"),
        QStringLiteral("\033[1;36m                .o+`               \033[0m"),
        QStringLiteral("\033[1;36m               `ooo/               \033[0m"),
        QStringLiteral("\033[1;36m              `+oooo:              \033[0m"),
        QStringLiteral("\033[1;36m             `+oooooo:             \033[0m"),
        QStringLiteral("\033[1;36m            -+oooooo+:             \033[0m"),
        QStringLiteral("\033[1;36m          `/:--++oooo+:            \033[0m"),
        QStringLiteral("\033[1;36m         `/++++/+++++++:           \033[0m"),
        QStringLiteral("\033[1;36m        `/+++++++++++++:           \033[0m"),
        QStringLiteral("\033[1;36m       `/+++oooooooooooo/`         \033[0m"),
        QStringLiteral("\033[1;36m      ./ooosssso++ossssso+`        \033[0m"),
        QStringLiteral("\033[1;36m     .oosssso-````/osssss+`        \033[0m"),
        QStringLiteral("\033[1;36m    -ossssso.          :sssssso.   \033[0m"),
        QStringLiteral("\033[1;36m   :ossssss/          osssso+++.   \033[0m"),
        QStringLiteral("\033[1;36m  /osssssss/          +sssooo/-    \033[0m"),
        QStringLiteral("\033[1;36m `/ossssso+/-          -:/+ossso+- \033[0m"),
        QStringLiteral("\033[1;36m`+sso+:-              `.-/+oso:    \033[0m"),
        QStringLiteral("\033[1;36m`++:.                    `-/+/     \033[0m"),
        QStringLiteral("\033[1;36m`                         `/       \033[0m")
    };

    const QString pad(35, u' ');
    const int totalRightLines = 2 + fields.size() + 2; // header(2) + fields + space + swatches
    const int maxLines = std::max(static_cast<int>(logo.size()), totalRightLines);

    out << u'\n';

    for (int i = 0; i < maxLines; ++i) {
        const QString &logoLine = (i < logo.size()) ? logo.at(i) : pad;
        out << logoLine << QStringLiteral("  ");

        if (i == 0) {
            out << QStringLiteral("\033[1;36m%1\033[0m\033[1;37m@\033[0m\033[1;36m%2\033[0m")
                       .arg(data.user, data.host);
        } else if (i == 1) {
            out << QStringLiteral("\033[36m")
                << QString(data.user.size() + 1 + data.host.size(), u'─')
                << QStringLiteral("\033[0m");
        } else if (i - 2 < fields.size()) {
            const auto &field = fields.at(i - 2);
            const QString key = field.first.leftJustified(11, u' ');
            out << QStringLiteral("\033[1;36m") << key
                << QStringLiteral("\033[0m: ") << field.second;
        } else if (i == 2 + fields.size() + 1) {
            for (int c = 30; c < 38; ++c)
                out << QStringLiteral("\033[%1m███\033[0m").arg(c);
        }

        out << u'\n';
    }

    out << u'\n';
    out.flush();
}
