#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QProcess>

#include "mainwindow.h"
#include "gridmanager.h"

void ensureGridManagerFloating()
{
    // Apply a window rule to keep the grid manager floating
    QProcess process;
    QStringList args;
    args << "keyword" << "windowrulev2" << "float,class:^(hypr-grid-manager)$";
    process.start("hyprctl", args);
    process.waitForFinished(1000);
    
    // Also set it to be always on top for better accessibility
    args.clear();
    args << "keyword" << "windowrulev2" << "stayfocused,class:^(hypr-grid-manager)$";
    process.start("hyprctl", args);
    process.waitForFinished(1000);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Hypr Grid Manager");
    app.setOrganizationName("Hyprland");
    app.setApplicationVersion("1.0.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Window grid manager for Hyprland");
    parser.addHelpOption();
    parser.addVersionOption();

    // Command line arguments
    QCommandLineOption applyOption(QStringList() << "a" << "apply", 
        "Apply a window position from a preset", "preset:position");
    QCommandLineOption resetOption(QStringList() << "r" << "reset", 
        "Reset window state and clear rules");
    QCommandLineOption configOption(QStringList() << "c" << "config", 
        "Print current configuration");
    QCommandLineOption uiOption(QStringList() << "u" << "ui", 
        "Show the configuration UI");
    QCommandLineOption testOption(QStringList() << "t" << "test", 
        "Test all grid positions by cycling through them");

    parser.addOption(applyOption);
    parser.addOption(resetOption);
    parser.addOption(configOption);
    parser.addOption(uiOption);
    parser.addOption(testOption);

    parser.process(app);

    // Handle CLI commands
    GridManager gridManager;
    
    // Initialize the grid manager
    if (!gridManager.initialize()) {
        qCritical() << "Failed to initialize grid manager";
        return 1;
    }

    // Check if we have CLI commands
    if (parser.isSet(resetOption)) {
        return gridManager.resetWindowState() ? 0 : 1;
    } 
    else if (parser.isSet(applyOption)) {
        QString applyArg = parser.value(applyOption);
        QStringList parts = applyArg.split(":");
        
        if (parts.size() != 2) {
            qCritical() << "Invalid apply format. Use preset:position";
            return 1;
        }
        
        return gridManager.applyPositionByCode(parts[0], parts[1]) ? 0 : 1;
    }
    else if (parser.isSet(configOption)) {
        gridManager.printConfig();
        return 0;
    }
    else if (parser.isSet(testOption)) {
        return gridManager.testAllPositions() ? 0 : 1;
    }
    // Show UI if requested or no other commands specified
    else if (parser.isSet(uiOption)) {
        // Ensure grid manager window stays floating
        ensureGridManagerFloating();
        
        MainWindow mainWindow(gridManager);
        mainWindow.show();
        return app.exec();
    }
    
    // Handle positional arguments for applying positions
    // Format: hypr-grid-manager <preset> <position>
    // Example: hypr-grid-manager quarters tl
    QStringList positionalArgs = parser.positionalArguments();
    if (positionalArgs.size() == 2) {
        QString preset = positionalArgs[0];
        QString position = positionalArgs[1];
        return gridManager.applyPositionByCode(preset, position) ? 0 : 1;
    }
    
    // If no arguments provided or just one argument, show UI
    if (argc == 1 || positionalArgs.size() == 0) {
        // Ensure grid manager window stays floating
        ensureGridManagerFloating();
        
        MainWindow mainWindow(gridManager);
        mainWindow.show();
        return app.exec();
    }
    
    // Invalid argument format
    if (positionalArgs.size() == 1) {
        qCritical() << "Invalid format. Use: hypr-grid-manager <preset> <position>";
        qCritical() << "Example: hypr-grid-manager quarters tl";
        return 1;
    }
    
    return 0;
}
