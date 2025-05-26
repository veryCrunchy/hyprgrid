#ifndef CONFIG_H
#define CONFIG_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonDocument>

class Config : public QObject
{
    Q_OBJECT
    
public:
    explicit Config(QObject *parent = nullptr);
    
    // Config file operations
    bool load();
    bool save() const;
    
    // Configuration access methods
    QVariantMap getGridConfig() const { return m_gridConfig; }
    QVariantMap getAppearanceConfig() const { return m_appearanceConfig; }
    QVariantMap getAdvancedConfig() const { return m_advancedConfig; }
    QMap<QString, QMap<QString, QVariantMap>> getPresets() const { return m_presets; }
    
    // Configuration update methods
    void setGridConfig(const QVariantMap &config);
    void setAppearanceConfig(const QVariantMap &config);
    void setAdvancedConfig(const QVariantMap &config);
    void setPresets(const QMap<QString, QMap<QString, QVariantMap>> &presets);
    
    // Convert to JSON for saving/display
    QJsonObject toJsonObject() const;
    
signals:
    void configChanged();
    void errorOccurred(const QString &message);
    
private:
    // Find all config locations
    QString findConfigFile() const;
    QString getDefaultConfigDir() const;
    
    // Load default configuration
    void loadDefaultConfig();
    
    // Configuration sections
    QVariantMap m_gridConfig;
    QVariantMap m_appearanceConfig;
    QVariantMap m_advancedConfig;
    QMap<QString, QMap<QString, QVariantMap>> m_presets;
    
    // Config file path
    QString m_configPath;
};

#endif // CONFIG_H
