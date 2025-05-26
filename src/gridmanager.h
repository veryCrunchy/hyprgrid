#ifndef GRIDMANAGER_H
#define GRIDMANAGER_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>

#include "hyprlandapi.h"
#include "config.h"

// Struct to hold grid position data
struct GridPosition {
    int x;
    int y;
    int width;
    int height;
    bool centered = false;
    double scale = 1.0;
};

// Struct to hold pixel position data
struct PixelPosition {
    int x;
    int y;
    int width;
    int height;
};

// Struct to hold screen dimensions and properties
struct Screen {
    int width;
    int height;
    int reservedTop;
    int reservedBottom;
    int reservedLeft;
    int reservedRight;
    double scale;
};

class GridManager : public QObject
{
    Q_OBJECT
    
public:
    explicit GridManager(QObject *parent = nullptr);
    ~GridManager();
    
    // Core functionality
    bool initialize();
    bool applyPositionByCode(const QString &preset, const QString &code);
    bool applyGridPosition(const GridPosition &position);
    bool resetWindowState();
    bool testAllPositions();
    
    // Configuration
    void printConfig() const;
    Config* getConfig() const { return m_config; }
    
    // Preset management
    QStringList getPresetNames() const;
    QStringList getPositionCodesForPreset(const QString &preset) const;
    GridPosition getGridPosition(const QString &preset, const QString &code) const;
    void saveGridPosition(const QString &preset, const QString &code, const GridPosition &position);
    
signals:
    void gridPositionApplied(const QString &preset, const QString &code);
    void errorOccurred(const QString &message);
    
private:
    HyprlandAPI *m_hyprland;
    Config *m_config;
    
    // Helper methods
    PixelPosition gridToPixelPosition(const GridPosition &position, const Screen &screen) const;
    bool ensureFloating();
    bool ensureTiled();
    bool hasMultipleWindowsInWorkspace() const;
    Screen getScreenDimensions() const;
    
    // Logging
    void logDebug(const QString &message) const;
    void logInfo(const QString &message) const;
    void logWarning(const QString &message) const;
    void logError(const QString &message) const;
};

#endif // GRIDMANAGER_H
