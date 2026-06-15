#include "cli.h"
#include "gui.h"

#include <QApplication>
#include <QCoreApplication>

#include <cstring>
#include <cstdio>

// ── helpers ───────────────────────────────────────────────────────────────────

static void printHelp()
{
    std::puts(
        "titanfetch — ArchTitan system information tool\n"
        "\n"
        "Usage:\n"
        "  titanfetch          Print system info in the terminal\n"
        "  titanfetch --gui    Open a graphical info window\n"
        "  titanfetch --help   Show this help text\n"
        "  titanfetch --version\n"
        "                      Show version\n");
}

static void printVersion()
{
    std::puts("titanfetch 1.0.0 (ArchTitan)");
}

// ── main ─────────────────────────────────────────────────────────────────────

int main(int argc, char *argv[])
{
    // Parse args with plain strcmp — no QCoreApplication needed yet (BUG-03 fix)
    bool useGui  = false;
    bool showHelp    = false;
    bool showVersion = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--gui")     == 0) useGui       = true;
        else if (std::strcmp(argv[i], "--help")    == 0) showHelp     = true;
        else if (std::strcmp(argv[i], "--version") == 0) showVersion  = true;
    }

    if (showHelp)    { printHelp();    return 0; }
    if (showVersion) { printVersion(); return 0; }

    if (useGui) {
        QApplication app(argc, argv);
        Gui gui;
        gui.show();
        return app.exec();
    }

    // CLI path: QCoreApplication only — no widget machinery loaded (BUG-03 fix)
    QCoreApplication app(argc, argv);
    Cli::run();
    return 0;
}
