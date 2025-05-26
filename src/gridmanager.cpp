#include "gridmanager.h"
#include <QDebug>
#include <QProcess>
#include <QJsonDocument>
#include <QThread>
#include <QDir>
#include <QStandardPaths>
#include <iostream>

GridManager::GridManager(QObject *parent)
    : QObject(parent), m_hyprland(nullptr), m_config(nullptr)
{
    // Constructor will be completed in initialize()
}

GridManager::~GridManager()
{
    delete m_hyprland;
    delete m_config;
}

bool GridManager::initialize()
{
    std::cout << "[DEBUG] GridManager::initialize() called" << std::endl;
    
    // Initialize configuration first
    m_config = new Config(this);
    if (!m_config->load()) {
        std::cout << "[ERROR] Failed to load configuration" << std::endl;
        return false;
    }
    
    std::cout << "[INFO] Configuration loaded successfully" << std::endl;
    logInfo("Grid Manager initializing");
    
    // Initialize Hyprland API
    m_hyprland = new HyprlandAPI(this);
    connect(m_hyprland, &HyprlandAPI::errorOccurred, this, &GridManager::errorOccurred);
    
    if (!m_hyprland->initialize()) {
        logError("Failed to initialize Hyprland API");
        return false;
    }
    
    logInfo("Grid Manager initialized successfully");
    return true;
}

bool GridManager::applyPositionByCode(const QString &preset, const QString &code)
{
    std::cout << "[INFO] Applying position " << code.toStdString() << " from preset " << preset.toStdString() << std::endl;
    logInfo(QString("Applying position %1 from preset %2").arg(code).arg(preset));
    
    // Debug: List available presets and positions
    QStringList availablePresets = getPresetNames();
    std::cout << "[DEBUG] Available presets: ";
    for (const QString &p : availablePresets) {
        std::cout << p.toStdString() << " ";
    }
    std::cout << std::endl;
    
    if (availablePresets.contains(preset)) {
        QStringList availablePositions = getPositionCodesForPreset(preset);
        std::cout << "[DEBUG] Available positions in " << preset.toStdString() << ": ";
        for (const QString &pos : availablePositions) {
            std::cout << pos.toStdString() << " ";
        }
        std::cout << std::endl;
    }
    
    // Get the position from config
    GridPosition position = getGridPosition(preset, code);
    if (position.width == 0 || position.height == 0) {
        std::cout << "[ERROR] Position '" << code.toStdString() << "' not found in preset '" << preset.toStdString() << "'" << std::endl;
        logError(QString("Position '%1' not found in preset '%2'").arg(code).arg(preset));
        return false;
    }
    
    std::cout << "[DEBUG] Found position: x=" << position.x << " y=" << position.y << " w=" << position.width << " h=" << position.height << std::endl;
    
    bool result = applyGridPosition(position);
    std::cout << "[DEBUG] applyPositionByCode returning: " << (result ? "true" : "false") << std::endl;
    return result;
}

bool GridManager::applyGridPosition(const GridPosition &position)
{
    std::cout << "[DEBUG] applyGridPosition called" << std::endl;
    
    // Get screen dimensions
    Screen screen = getScreenDimensions();
    if (screen.width <= 0 || screen.height <= 0) {
        std::cout << "[ERROR] Invalid screen dimensions: " << screen.width << "x" << screen.height << std::endl;
        logError("Invalid screen dimensions");
        return false;
    }
    
    std::cout << "[DEBUG] Screen dimensions: " << screen.width << "x" << screen.height << std::endl;
    
    // Convert grid position to pixel coordinates
    PixelPosition pixelPos = gridToPixelPosition(position, screen);
    
    std::cout << "[DEBUG] Converted to pixel position: x=" << pixelPos.x << " y=" << pixelPos.y << " w=" << pixelPos.width << " h=" << pixelPos.height << std::endl;
    
    logInfo(QString("Applying grid position: x=%1, y=%2, width=%3, height=%4")
        .arg(pixelPos.x).arg(pixelPos.y).arg(pixelPos.width).arg(pixelPos.height));
    
    // Check if we should use tiling mode
    bool useTiling = m_config->getAdvancedConfig()["useTiling"].toBool();
    
    std::cout << "[DEBUG] useTiling: " << (useTiling ? "true" : "false") << std::endl;
    
    // Check if there are multiple windows in the current workspace
    // Tiling only works effectively with multiple windows
    bool hasMultipleWindows = hasMultipleWindowsInWorkspace();
    
    std::cout << "[DEBUG] hasMultipleWindows: " << (hasMultipleWindows ? "true" : "false") << std::endl;
    
    if (useTiling && hasMultipleWindows) {
        // For tiling mode with multiple windows, use tiling commands
        if (!ensureTiled()) {
            logWarning("Failed to ensure window is tiled");
        }
        
        // Use Hyprland's tiling commands to position the window
        if (!m_hyprland->positionTiledWindow(pixelPos.x, pixelPos.y, pixelPos.width, pixelPos.height)) {
            logError("Failed to position tiled window");
            return false;
        }
    } else {
        // Use floating mode for precise positioning
        // This includes: single windows, forced floating, or legacy floating mode
        if (!hasMultipleWindows) {
            std::cout << "[DEBUG] Single window detected, using floating mode for precise positioning" << std::endl;
            logDebug("Single window detected, using floating mode for precise positioning");
        }
        
        if (!ensureFloating()) {
            std::cout << "[WARNING] Failed to ensure window is floating" << std::endl;
            logWarning("Failed to ensure window is floating");
        } else {
            std::cout << "[DEBUG] Window is now floating" << std::endl;
        }
        
        // Apply the position using floating window commands
        std::cout << "[DEBUG] Calling moveAndResizeWindow with: " << pixelPos.x << "," << pixelPos.y << "," << pixelPos.width << "," << pixelPos.height << std::endl;
        if (!m_hyprland->moveAndResizeWindow(pixelPos.x, pixelPos.y, pixelPos.width, pixelPos.height)) {
            std::cout << "[ERROR] Failed to move and resize window" << std::endl;
            logError("Failed to move and resize window");
            return false;
        } else {
            std::cout << "[DEBUG] moveAndResizeWindow returned success" << std::endl;
        }
    }
    
    // Show notification if enabled
    if (m_config->getAppearanceConfig()["showNotifications"].toBool()) {
        std::cout << "[DEBUG] Sending notification" << std::endl;
        m_hyprland->sendNotification(
            "Grid Manager", 
            QString("Applying %1×%2 position").arg(position.width).arg(position.height),
            m_config->getAppearanceConfig()["notificationDuration"].toInt()
        );
    }
    
    std::cout << "[DEBUG] Position application completed successfully" << std::endl;
    
    // Retry if configured and needed
    bool success = true;
    if (!success && m_config->getAdvancedConfig()["retryOnFailure"].toBool()) {
        int retries = m_config->getAdvancedConfig()["retryCount"].toInt();
        int delay = m_config->getAdvancedConfig()["retryDelay"].toInt();
        
        while (!success && retries > 0) {
            logDebug(QString("Retrying position application (%1 attempts left)").arg(retries));
            QThread::msleep(delay);
            
            if (useTiling) {
                success = m_hyprland->positionTiledWindow(pixelPos.x, pixelPos.y, pixelPos.width, pixelPos.height);
            } else {
                success = m_hyprland->moveAndResizeWindow(pixelPos.x, pixelPos.y, pixelPos.width, pixelPos.height);
            }
            retries--;
        }
    }
    
    if (success) {
        emit gridPositionApplied(QString(), QString()); // We don't know the preset/code here
        std::cout << "[DEBUG] gridPositionApplied signal emitted" << std::endl;
    }
    
    std::cout << "[DEBUG] applyGridPosition returning: " << (success ? "true" : "false") << std::endl;
    return success;
}

bool GridManager::resetWindowState()
{
    logInfo("Resetting window state");
    
    // Toggle floating twice to reset state
    m_hyprland->toggleFloating();
    QThread::msleep(100);
    m_hyprland->toggleFloating();
    
    // Clear any window rules
    m_hyprland->clearWindowRules();
    
    return true;
}

void GridManager::printConfig() const
{
    // Print the current configuration as JSON
    QJsonObject jsonObj = m_config->toJsonObject();
    QJsonDocument doc(jsonObj);
    qDebug().noquote() << doc.toJson();
}

QStringList GridManager::getPresetNames() const
{
    QMap<QString, QMap<QString, QVariantMap>> presets = m_config->getPresets();
    QStringList names;
    for (auto it = presets.begin(); it != presets.end(); ++it) {
        names << it.key();
    }
    return names;
}

QStringList GridManager::getPositionCodesForPreset(const QString &preset) const
{
    QMap<QString, QMap<QString, QVariantMap>> presets = m_config->getPresets();
    QMap<QString, QVariantMap> presetData = presets[preset];
    QStringList codes;
    for (auto it = presetData.begin(); it != presetData.end(); ++it) {
        codes << it.key();
    }
    return codes;
}

GridPosition GridManager::getGridPosition(const QString &preset, const QString &code) const
{
    QMap<QString, QMap<QString, QVariantMap>> presets = m_config->getPresets();
    QMap<QString, QVariantMap> presetData = presets[preset];
    QVariantMap positionData = presetData[code];
    
    GridPosition position;
    position.x = positionData["x"].toInt();
    position.y = positionData["y"].toInt();
    position.width = positionData["width"].toInt();
    position.height = positionData["height"].toInt();
    position.centered = positionData["centered"].toBool();
    position.scale = positionData["scale"].toDouble();
    
    // Set default scale if not specified
    if (position.scale <= 0.0) {
        position.scale = 1.0;
    }
    
    return position;
}

void GridManager::saveGridPosition(const QString &preset, const QString &code, const GridPosition &position)
{
    QVariantMap positionData;
    positionData["x"] = position.x;
    positionData["y"] = position.y;
    positionData["width"] = position.width;
    positionData["height"] = position.height;
    positionData["centered"] = position.centered;
    positionData["scale"] = position.scale;
    
    // Get current presets
    QMap<QString, QMap<QString, QVariantMap>> presetsData = m_config->getPresets();
    QMap<QString, QVariantMap> presetData = presetsData[preset];
    
    // Update the specific position
    presetData[code] = positionData;
    presetsData[preset] = presetData;
    
    // Save back to config
    m_config->setPresets(presetsData);
    m_config->save();
}

PixelPosition GridManager::gridToPixelPosition(const GridPosition &position, const Screen &screen) const
{
    // Get grid configuration
    QVariantMap gridConfig = m_config->getGridConfig();
    int rows = gridConfig["rows"].toInt();
    int cols = gridConfig["columns"].toInt();
    int gaps = gridConfig["gaps"].toInt();
    
    // Calculate cell dimensions
    int cellWidth = (screen.width - gaps * (cols + 1)) / cols;
    int cellHeight = (screen.height - gaps * (rows + 1)) / rows;
    
    PixelPosition pixelPos;
    
    if (position.centered && position.scale > 0.0 && position.scale < 1.0) {
        // Centered scaled window
        int scaledWidth = static_cast<int>(screen.width * position.scale);
        int scaledHeight = static_cast<int>(screen.height * position.scale);
        
        pixelPos.x = (screen.width - scaledWidth) / 2;
        pixelPos.y = (screen.height - scaledHeight) / 2;
        pixelPos.width = scaledWidth;
        pixelPos.height = scaledHeight;
    } else {
        // Grid-based positioning
        pixelPos.x = gaps + position.x * (cellWidth + gaps);
        pixelPos.y = gaps + position.y * (cellHeight + gaps);
        pixelPos.width = position.width * cellWidth + (position.width - 1) * gaps;
        pixelPos.height = position.height * cellHeight + (position.height - 1) * gaps;
    }
    
    return pixelPos;
}

bool GridManager::ensureFloating()
{
    // Check current window state
    bool isFloating = m_hyprland->isWindowFloating();
    
    if (!isFloating) {
        logDebug("Window is not floating, toggling to floating state");
        if (!m_hyprland->toggleFloating()) {
            logError("Failed to toggle floating state");
            return false;
        }
        
        // Wait a bit and verify
        QThread::msleep(100);
        isFloating = m_hyprland->isWindowFloating();
        
        if (!isFloating) {
            int retryCount = m_config->getAdvancedConfig()["retryCount"].toInt();
            
            // Try a few more times with longer delays
            while (!isFloating && retryCount > 0) {
                logWarning(QString("Window still not floating, retrying (attempts left: %1)").arg(retryCount));
                
                // Toggle twice to reset state
                m_hyprland->toggleFloating();
                QThread::msleep(200);
                m_hyprland->toggleFloating();
                QThread::msleep(200);
                
                isFloating = m_hyprland->isWindowFloating();
                retryCount--;
            }
        }
    }
    
    return isFloating;
}

bool GridManager::ensureTiled()
{
    // For the grid manager's "tiling" mode, we use floating windows positioned
    // precisely on the grid. This gives us exact control while maintaining the
    // visual grid layout the user expects.
    
    // Check current window state
    bool isFloating = m_hyprland->isWindowFloating();
    
    if (!isFloating) {
        logDebug("Window is tiled, making it floating for precise positioning");
        if (!m_hyprland->toggleFloating()) {
            logError("Failed to toggle floating state");
            return false;
        }
        
        // Wait a bit and verify
        QThread::msleep(100);
        isFloating = m_hyprland->isWindowFloating();
        
        if (!isFloating) {
            logError("Failed to make window floating");
            return false;
        }
    }
    
    return true;
}

Screen GridManager::getScreenDimensions() const
{
    QVariantMap monitorData = m_hyprland->getFocusedMonitorData();
    
    Screen screen;
    screen.width = monitorData["width"].toInt();
    screen.height = monitorData["height"].toInt();
    screen.reservedTop = 0;
    screen.reservedBottom = 0;
    screen.reservedLeft = 0;
    screen.reservedRight = 0;
    screen.scale = monitorData["scale"].toDouble();
    
    // Set default scale if not specified
    if (screen.scale <= 0.0) {
        screen.scale = 1.0;
    }
    
    return screen;
}

void GridManager::logDebug(const QString &message) const
{
    QString level = m_config->getAdvancedConfig()["logLevel"].toString();
    if (level == "debug") {
        qDebug() << "[DEBUG]" << message;
    }
}

void GridManager::logInfo(const QString &message) const
{
    QString level = m_config->getAdvancedConfig()["logLevel"].toString();
    if (level == "debug" || level == "info") {
        qDebug() << "[INFO]" << message;
    }
}

void GridManager::logWarning(const QString &message) const
{
    QString level = m_config->getAdvancedConfig()["logLevel"].toString();
    if (level == "debug" || level == "info" || level == "warn") {
        qDebug() << "[WARNING]" << message;
    }
}

void GridManager::logError(const QString &message) const
{
    qDebug() << "[ERROR]" << message;
    emit const_cast<GridManager*>(this)->errorOccurred(message);
}

bool GridManager::hasMultipleWindowsInWorkspace() const
{
    // Get current workspace ID
    int currentWorkspace = m_hyprland->getCurrentWorkspaceId();
    if (currentWorkspace < 0) {
        logError("Failed to get current workspace ID");
        return false; // Assume single window on error
    }
    
    // Get all windows using hyprctl
    QProcess process;
    process.start("hyprctl", QStringList() << "clients" << "-j");
    process.waitForFinished();
    
    if (process.exitCode() != 0) {
        logError("Failed to get window list from hyprctl");
        return false; // Assume single window on error
    }
    
    QString output = process.readAllStandardOutput();
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    
    if (!doc.isArray()) {
        logError("Invalid JSON response from hyprctl clients");
        return false;
    }
    
    // Count windows in current workspace (excluding special workspaces)
    int windowCount = 0;
    QJsonArray windows = doc.array();
    
    for (const QJsonValue &val : windows) {
        if (!val.isObject()) continue;
        
        QJsonObject window = val.toObject();
        int windowWorkspace = window["workspace"].toObject()["id"].toInt();
        
        // Only count windows in the current workspace, not special workspaces
        if (windowWorkspace == currentWorkspace && windowWorkspace > 0) {
            windowCount++;
        }
    }
    
    logDebug(QString("Found %1 windows in workspace %2").arg(windowCount).arg(currentWorkspace));
    return windowCount > 1;
}

bool GridManager::testAllPositions()
{
    logInfo("Starting grid position test - cycling through all available positions");
    
    if (!m_config) {
        logError("Configuration not loaded");
        return false;
    }
    
    QStringList presets = getPresetNames();
    if (presets.isEmpty()) {
        logError("No presets available for testing");
        return false;
    }
    
    // Use the first preset for testing
    QString testPreset = presets.first();
    QStringList positions = getPositionCodesForPreset(testPreset);
    
    if (positions.isEmpty()) {
        logError(QString("No positions available in preset '%1'").arg(testPreset));
        return false;
    }
    
    logInfo(QString("Testing %1 positions from preset '%2'").arg(positions.size()).arg(testPreset));
    
    bool allSuccess = true;
    for (const QString &positionCode : positions) {
        logInfo(QString("Testing position: %1:%2").arg(testPreset).arg(positionCode));
        
        if (!applyPositionByCode(testPreset, positionCode)) {
            logError(QString("Failed to apply position: %1:%2").arg(testPreset).arg(positionCode));
            allSuccess = false;
        } else {
            logInfo(QString("✓ Successfully applied position: %1:%2").arg(testPreset).arg(positionCode));
        }
        
        // Wait 2 seconds between positions to see the change
        QThread::msleep(2000);
    }
    
    logInfo(QString("Grid position test completed. Success: %1").arg(allSuccess ? "Yes" : "No"));
    return allSuccess;
}
