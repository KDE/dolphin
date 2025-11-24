#include <KConfigGroup>
#include <KSharedConfig>

int main()
{
    const auto closeActiveSplitView = QStringLiteral("CloseActiveSplitView");
    const auto closeSplitViewChoice = QStringLiteral("CloseSplitViewChoice");
    auto config = KSharedConfig::openConfig(QStringLiteral("dolphinrc"));

    KConfigGroup general = config->group(QStringLiteral("General"));
    if (!general.hasKey(closeActiveSplitView)) {
        return EXIT_SUCCESS;
    }

    const auto value = general.readEntry(closeActiveSplitView);
    if (value == QStringLiteral("true")) {
        general.deleteEntry(closeActiveSplitView);
        general.writeEntry(closeSplitViewChoice, QStringLiteral("ActiveView"));
    } else if (value == QStringLiteral("false")) {
        general.deleteEntry(closeActiveSplitView);
        general.writeEntry(closeSplitViewChoice, QStringLiteral("InactiveView"));
    }

    return general.sync() ? EXIT_SUCCESS : EXIT_FAILURE;
}
