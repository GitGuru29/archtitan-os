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

    // Taller logo to accommodate all info fields
    const QStringList logo = {
        QStringLiteral("\033[36m         ..::.          \033[0m"),
        QStringLiteral("\033[36m       .:::::::.        \033[0m"),
        QStringLiteral("\033[36m      /:::::::::\\       \033[0m"),
        QStringLiteral("\033[36m     /:::/  \\:::\\      \033[0m"),
        QStringLiteral("\033[36m    /:::/ T  \\:::\\     \033[0m"),
        QStringLiteral("\033[36m   /:::/ ITA  \\:::\\    \033[0m"),
        QStringLiteral("\033[36m  /:::/ TITAN  \\:::\\   \033[0m"),
        QStringLiteral("\033[36m /:::/    AN    \\:::\\  \033[0m"),
        QStringLiteral("\033[36m/:::/____________\\::::  \033[0m"),
        QStringLiteral("\033[36m\\:::\\            /:::/ \033[0m"),
        QStringLiteral("\033[36m \\:::\\__________/:::/  \033[0m"),
        QStringLiteral("\033[36m  \\:::::::::::::::::/ \033[0m"),
        QStringLiteral("\033[36m   \\:::::::::::::::/ \033[0m"),
        QStringLiteral("\033[36m    \\'\"\"\"\"\"\"\"\"\"\"\"\"'/  \033[0m"),
        QStringLiteral("\033[36m      ArchTitan      \033[0m"),
    };

    const QString pad(24, u' ');
    const int maxLines = std::max(logo.size(), fields.size());

    // Header
    out << u'\n';
    out << QStringLiteral("   \033[1;36m%1\033[0m\033[1;37m@\033[0m\033[1;36m%2\033[0m\n")
               .arg(data.user, data.host);
    out << QStringLiteral("   \033[36m");
    out << QString(data.user.size() + 1 + data.host.size(), u'─');
    out << QStringLiteral("\033[0m\n");

    for (int i = 0; i < maxLines; ++i) {
        const QString &logoLine = (i < logo.size()) ? logo.at(i) : pad;

        if (i < fields.size()) {
            const QString key = fields.at(i).first.leftJustified(12, u' ');
            out << logoLine
                << QStringLiteral("  \033[1;36m") << key
                << QStringLiteral("\033[0m: ") << fields.at(i).second << u'\n';
        } else {
            out << logoLine << u'\n';
        }
    }

    // Colour swatches
    out << u'\n' << pad << QStringLiteral("  ");
    for (int c = 30; c < 38; ++c)
        out << QStringLiteral("\033[%1m███\033[0m").arg(c);
    out << QStringLiteral("\n\n");

    out.flush();
}
