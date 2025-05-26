#include "config.h"

#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

Config::Config(QObject *parent) : QObject(parent)
{
    // Initialize with defaults
    loadDefaultConfig();
    
    // Try to find an existing config file
    m_configPath = findConfigFile();
}

bool Config::load()
{
    if (m_configPath.isEmpty()) {
        qDebug() << "No config file found, using defaults";
        return true;
    }
    
    qDebug() << "Loading config from:" << m_configPath;
    
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit errorOccurred(QString("Cannot open config file: %1").arg(m_configPath));
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit errorOccurred("Invalid JSON in config file");
        return false;
    }
    
    QJsonObject obj = doc.object();
    
    // Load grid configuration
    if (obj.contains("grid") && obj["grid"].isObject()) {
        m_gridConfig = obj["grid"].toObject().toVariantMap();
    }
    
    // Load appearance configuration
    if (obj.contains("appearance") && obj["appearance"].isObject()) {
        m_appearanceConfig = obj["appearance"].toObject().toVariantMap();
    }
    
    // Load advanced configuration
    if (obj.contains("advanced") && obj["advanced"].isObject()) {
        m_advancedConfig = obj["advanced"].toObject().toVariantMap();
    }
    
    // Load presets
    if (obj.contains("presets") && obj["presets"].isObject()) {
        QJsonObject presetsObj = obj["presets"].toObject();
        m_presets.clear();
        
        for (auto presetIt = presetsObj.begin(); presetIt != presetsObj.end(); ++presetIt) {
            QString presetName = presetIt.key();
            QJsonObject presetObj = presetIt.value().toObject();
            
            QMap<QString, QVariantMap> positions;
            
            for (auto posIt = presetObj.begin(); posIt != presetObj.end(); ++posIt) {
                QString posCode = posIt.key();
                QJsonObject posObj = posIt.value().toObject();
                
                positions[posCode] = posObj.toVariantMap();
            }
            
            m_presets[presetName] = positions;
        }
    }
    
    qDebug() << "Loaded configuration from" << m_configPath;
    return true;
}

bool Config::save() const
{
    if (m_configPath.isEmpty()) {
        QString configDir = getDefaultConfigDir();
        QDir dir(configDir);
        
        if (!dir.exists() && !dir.mkpath(configDir)) {
            emit const_cast<Config*>(this)->errorOccurred(
                QString("Cannot create config directory: %1").arg(configDir));
            return false;
        }
        
        const_cast<Config*>(this)->m_configPath = configDir + "/config.json";
    }
    
    QFile file(m_configPath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit const_cast<Config*>(this)->errorOccurred(
            QString("Cannot write to config file: %1").arg(m_configPath));
        return false;
    }
    
    QJsonDocument doc(toJsonObject());
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    qDebug() << "Saved configuration to" << m_configPath;
    return true;
}

void Config::setGridConfig(const QVariantMap &config)
{
    m_gridConfig = config;
    emit configChanged();
}

void Config::setAppearanceConfig(const QVariantMap &config)
{
    m_appearanceConfig = config;
    emit configChanged();
}

void Config::setAdvancedConfig(const QVariantMap &config)
{
    m_advancedConfig = config;
    emit configChanged();
}

void Config::setPresets(const QMap<QString, QMap<QString, QVariantMap>> &presets)
{
    m_presets = presets;
    emit configChanged();
}

QJsonObject Config::toJsonObject() const
{
    QJsonObject obj;
    
    // Add grid config
    obj["grid"] = QJsonObject::fromVariantMap(m_gridConfig);
    
    // Add appearance config
    obj["appearance"] = QJsonObject::fromVariantMap(m_appearanceConfig);
    
    // Add advanced config
    obj["advanced"] = QJsonObject::fromVariantMap(m_advancedConfig);
    
    // Add presets
    QJsonObject presetsObj;
    
    for (auto it = m_presets.constBegin(); it != m_presets.constEnd(); ++it) {
        QJsonObject presetObj;
        
        for (auto posIt = it.value().constBegin(); posIt != it.value().constEnd(); ++posIt) {
            presetObj[posIt.key()] = QJsonObject::fromVariantMap(posIt.value());
        }
        
        presetsObj[it.key()] = presetObj;
    }
    
    obj["presets"] = presetsObj;
    
    return obj;
}

QString Config::findConfigFile() const
{
    // Look in standard locations
    QStringList configLocations = {
        getDefaultConfigDir() + "/config.json",
        QDir::homePath() + "/.config/hypr/grid-config.json",
        QDir::homePath() + "/.config/hypr/grid/config.json"
    };
    
    for (const QString &path : configLocations) {
        if (QFile::exists(path)) {
            return path;
        }
    }
    
    return QString();
}

QString Config::getDefaultConfigDir() const
{
    return QDir::homePath() + "/.config/hypr/qt-grid-manager";
}

void Config::loadDefaultConfig()
{
    // Default grid config
    m_gridConfig["rows"] = 3;
    m_gridConfig["columns"] = 3;
    m_gridConfig["gaps"] = 5;
    
    // Default appearance config
    m_appearanceConfig["theme"] = "system";
    m_appearanceConfig["showNotifications"] = true;
    m_appearanceConfig["notificationDuration"] = 2000;
    m_appearanceConfig["primaryColor"] = "#D667EE";
    m_appearanceConfig["accentColor"] = "#1B1723";
    // Default advanced config
    m_advancedConfig["logLevel"] = "info";
    m_advancedConfig["floatingOnly"] = true;
    m_advancedConfig["forceFloat"] = true;
    m_advancedConfig["retryOnFailure"] = true;
    m_advancedConfig["retryCount"] = 3;
    m_advancedConfig["retryDelay"] = 200;
    
    // Default presets
    QMap<QString, QVariantMap> defaultPreset;
    
    // Center positions
    QVariantMap centerFull;
    centerFull["x"] = 0;
    centerFull["y"] = 0;
    centerFull["width"] = 3;
    centerFull["height"] = 3;
    
    QVariantMap centerLarge;
    centerLarge["x"] = 0;
    centerLarge["y"] = 0;
    centerLarge["width"] = 3;
    centerLarge["height"] = 3;
    centerLarge["centered"] = true;
    centerLarge["scale"] = 0.85;
    
    QVariantMap centerMedium;
    centerMedium["x"] = 0;
    centerMedium["y"] = 0;
    centerMedium["width"] = 3;
    centerMedium["height"] = 3;
    centerMedium["centered"] = true;
    centerMedium["scale"] = 0.65;
    
    QVariantMap centerSmall;
    centerSmall["x"] = 0;
    centerSmall["y"] = 0;
    centerSmall["width"] = 3;
    centerSmall["height"] = 3;
    centerSmall["centered"] = true;
    centerSmall["scale"] = 0.4;
    
    defaultPreset["full"] = centerFull;
    defaultPreset["large"] = centerLarge;
    defaultPreset["medium"] = centerMedium;
    defaultPreset["small"] = centerSmall;
    
    // Half screen positions
    QVariantMap leftHalf;
    leftHalf["x"] = 0;
    leftHalf["y"] = 0;
    leftHalf["width"] = 1;
    leftHalf["height"] = 3;
    
    QVariantMap rightHalf;
    rightHalf["x"] = 2;
    rightHalf["y"] = 0;
    rightHalf["width"] = 1;
    rightHalf["height"] = 3;
    
    QVariantMap topHalf;
    topHalf["x"] = 0;
    topHalf["y"] = 0;
    topHalf["width"] = 3;
    topHalf["height"] = 1;
    
    QVariantMap bottomHalf;
    bottomHalf["x"] = 0;
    bottomHalf["y"] = 2;
    bottomHalf["width"] = 3;
    bottomHalf["height"] = 1;
    
    defaultPreset["left"] = leftHalf;
    defaultPreset["right"] = rightHalf;
    defaultPreset["top"] = topHalf;
    defaultPreset["bottom"] = bottomHalf;
    
    // Corner positions
    QVariantMap topLeft;
    topLeft["x"] = 0;
    topLeft["y"] = 0;
    topLeft["width"] = 1;
    topLeft["height"] = 1;
    
    QVariantMap topRight;
    topRight["x"] = 2;
    topRight["y"] = 0;
    topRight["width"] = 1;
    topRight["height"] = 1;
    
    QVariantMap bottomLeft;
    bottomLeft["x"] = 0;
    bottomLeft["y"] = 2;
    bottomLeft["width"] = 1;
    bottomLeft["height"] = 1;
    
    QVariantMap bottomRight;
    bottomRight["x"] = 2;
    bottomRight["y"] = 2;
    bottomRight["width"] = 1;
    bottomRight["height"] = 1;
    
    defaultPreset["top-left"] = topLeft;
    defaultPreset["top-right"] = topRight;
    defaultPreset["bottom-left"] = bottomLeft;
    defaultPreset["bottom-right"] = bottomRight;
    
    // Add to presets
    m_presets["default"] = defaultPreset;
}
