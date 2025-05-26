#include "hyprlandapi.h"

#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <iostream>

HyprlandAPI::HyprlandAPI(QObject *parent) 
    : QObject(parent), m_initialized(false)
{
    // No additional initialization needed
}

HyprlandAPI::~HyprlandAPI()
{
    // Clean up any outstanding rules
    if (m_initialized && !m_currentWindowRuleIdentifier.isEmpty()) {
        clearWindowRules();
    }
}

bool HyprlandAPI::initialize()
{
    // Check if Hyprland is running
    QProcess process;
    process.start("pgrep", QStringList() << "-x" << "Hyprland");
    process.waitForFinished();
    
    if (process.exitCode() != 0) {
        emit errorOccurred("Hyprland is not running");
        return false;
    }
    
    // Get current window information
    QVariantMap windowData = getFocusedWindowData();
    if (windowData.isEmpty()) {
        qDebug() << "No focused window found, but continuing initialization";
    } else {
        m_currentWindowAddress = windowData["address"].toString();
        qDebug() << "Current window address:" << m_currentWindowAddress;
    }
    
    m_initialized = true;
    return true;
}

bool HyprlandAPI::moveAndResizeWindow(int x, int y, int width, int height)
{
    // Make sure we're initialized
    if (!m_initialized) {
        emit errorOccurred("HyprlandAPI not initialized");
        return false;
    }
    
    // Update current window information
    QVariantMap windowData = getFocusedWindowData();
    if (windowData.isEmpty()) {
        emit errorOccurred("No focused window");
        return false;
    }
    
    m_currentWindowAddress = windowData["address"].toString();
    std::cout << "[DEBUG] moveAndResizeWindow: address=" << m_currentWindowAddress.toStdString() << std::endl;
    
    // Execute the move and resize commands
    QString moveCmd = QString("movewindowpixel exact %1 %2,address:%3")
        .arg(x).arg(y).arg(m_currentWindowAddress);
    
    QString resizeCmd = QString("resizewindowpixel exact %1 %2,address:%3")
        .arg(width).arg(height).arg(m_currentWindowAddress);
    
    std::cout << "[DEBUG] moveAndResizeWindow: moveCmd=" << moveCmd.toStdString() << std::endl;
    std::cout << "[DEBUG] moveAndResizeWindow: resizeCmd=" << resizeCmd.toStdString() << std::endl;
    
    QString moveResult = executeHyprlandCommand(moveCmd);
    QString resizeResult = executeHyprlandCommand(resizeCmd);
    
    std::cout << "[DEBUG] moveAndResizeWindow: moveResult='" << moveResult.toStdString() << "'" << std::endl;
    std::cout << "[DEBUG] moveAndResizeWindow: resizeResult='" << resizeResult.toStdString() << "'" << std::endl;
    
    // Check for success - Hyprland returns "ok" for successful commands, or empty/error messages
    bool moveSuccess = moveResult.trimmed().isEmpty() || 
                       moveResult.trimmed() == "ok" ||
                       (!moveResult.contains("error", Qt::CaseInsensitive) && 
                        !moveResult.contains("failed", Qt::CaseInsensitive));
                        
    bool resizeSuccess = resizeResult.trimmed().isEmpty() || 
                         resizeResult.trimmed() == "ok" ||
                         (!resizeResult.contains("error", Qt::CaseInsensitive) && 
                          !resizeResult.contains("failed", Qt::CaseInsensitive));
    
    std::cout << "[DEBUG] moveAndResizeWindow: moveSuccess=" << (moveSuccess ? "true" : "false") << " resizeSuccess=" << (resizeSuccess ? "true" : "false") << std::endl;
    
    return moveSuccess && resizeSuccess;
}

bool HyprlandAPI::toggleFloating()
{
    // Make sure we're initialized
    if (!m_initialized) {
        emit errorOccurred("HyprlandAPI not initialized");
        return false;
    }
    
    // Update window address first
    QVariantMap windowData = getFocusedWindowData();
    if (windowData.isEmpty()) {
        emit errorOccurred("No focused window");
        return false;
    }
    
    m_currentWindowAddress = windowData["address"].toString();
    
    // Toggle floating state
    QString result = executeHyprlandCommand("togglefloating");
    // hyprctl dispatch commands typically return empty on success
    // or contain error messages on failure
    return !result.contains("error", Qt::CaseInsensitive) && !result.contains("failed", Qt::CaseInsensitive);
}

bool HyprlandAPI::isWindowFloating()
{
    // Make sure we're initialized
    if (!m_initialized) {
        emit errorOccurred("HyprlandAPI not initialized");
        return false;
    }
    
    // Get current window data
    QVariantMap windowData = getFocusedWindowData();
    if (windowData.isEmpty()) {
        emit errorOccurred("No focused window");
        return false;
    }
    
    // Check if floating
    return windowData["floating"].toBool();
}

bool HyprlandAPI::applyWindowRules(int x, int y, int width, int height)
{
    // Make sure we're initialized
    if (!m_initialized) {
        emit errorOccurred("HyprlandAPI not initialized");
        return false;
    }
    
    // Update window data
    QVariantMap windowData = getFocusedWindowData();
    if (windowData.isEmpty()) {
        emit errorOccurred("No focused window");
        return false;
    }
    
    // Clear any existing rules
    clearWindowRules();
    
    // Generate a new identifier
    m_currentWindowRuleIdentifier = generateWindowRuleIdentifier();
    
    // Get window class and title
    QString windowClass = windowData["class"].toString();
    QString windowTitle = windowData["title"].toString();
    
    if (windowClass.isEmpty()) {
        emit errorOccurred("Window class is empty");
        return false;
    }
    
    // Create the rules
    QStringList rules;
    
    // Position and size rule
    QString positionRule = QString("windowrulev2=float,class:%1,title:%2")
        .arg(windowClass).arg(windowTitle);
    
    QString dimensionsRule = QString("windowrulev2=move %1 %2,class:%3,title:%4")
        .arg(x).arg(y).arg(windowClass).arg(windowTitle);
    
    QString sizeRule = QString("windowrulev2=size %1 %2,class:%3,title:%4")
        .arg(width).arg(height).arg(windowClass).arg(windowTitle);
    
    // Apply the rules
    QProcess process;
    process.start("hyprctl", QStringList() << "keyword" << positionRule);
    process.waitForFinished();
    
    process.start("hyprctl", QStringList() << "keyword" << dimensionsRule);
    process.waitForFinished();
    
    process.start("hyprctl", QStringList() << "keyword" << sizeRule);
    process.waitForFinished();
    
    return true;
}

bool HyprlandAPI::clearWindowRules()
{
    // Make sure we're initialized
    if (!m_initialized) {
        emit errorOccurred("HyprlandAPI not initialized");
        return false;
    }
    
    // Execute the reload command to clear all rules
    QString result = executeHyprctlCommand(QStringList() << "reload");
    
    m_currentWindowRuleIdentifier.clear();
    
    return result.trimmed().isEmpty() || !result.contains("error", Qt::CaseInsensitive);
}

QVariantMap HyprlandAPI::getFocusedWindowData()
{
    QString output = executeHyprctlCommand(QStringList() << "activewindow" << "-j");
    return parseJsonOutput(output);
}

QVariantMap HyprlandAPI::getFocusedMonitorData()
{
    // Get the active monitor
    QString output = executeHyprctlCommand(QStringList() << "monitors" << "-j");
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    
    if (!doc.isArray()) {
        emit errorOccurred("Failed to parse monitor data");
        return QVariantMap();
    }
    
    QJsonArray monitors = doc.array();
    
    // Find the focused monitor
    for (const QJsonValue &val : monitors) {
        if (!val.isObject()) continue;
        
        QJsonObject monitor = val.toObject();
        if (monitor["focused"].toBool()) {
            return monitor.toVariantMap();
        }
    }
    
    // If no focused monitor found, return the first one
    if (!monitors.isEmpty() && monitors.first().isObject()) {
        return monitors.first().toObject().toVariantMap();
    }
    
    return QVariantMap();
}

QVariantMap HyprlandAPI::getWorkspaceData()
{
    QString output = executeHyprctlCommand(QStringList() << "workspaces" << "-j");
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    
    if (!doc.isArray()) {
        emit errorOccurred("Failed to parse workspace data");
        return QVariantMap();
    }
    
    QJsonArray workspaces = doc.array();
    
    // Find the focused workspace
    for (const QJsonValue &val : workspaces) {
        if (!val.isObject()) continue;
        
        QJsonObject workspace = val.toObject();
        if (workspace["id"].toInt() == workspace["lastwindow"].toInt()) {
            return workspace.toVariantMap();
        }
    }
    
    // If no workspace with last window found, return the first one
    if (!workspaces.isEmpty() && workspaces.first().isObject()) {
        return workspaces.first().toObject().toVariantMap();
    }
    
    return QVariantMap();
}

QStringList HyprlandAPI::getMonitors()
{
    QString output = executeHyprctlCommand(QStringList() << "monitors" << "-j");
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    
    if (!doc.isArray()) {
        emit errorOccurred("Failed to parse monitor data");
        return QStringList();
    }
    
    QStringList result;
    QJsonArray monitors = doc.array();
    
    for (const QJsonValue &val : monitors) {
        if (!val.isObject()) continue;
        
        QJsonObject monitor = val.toObject();
        result.append(monitor["name"].toString());
    }
    
    return result;
}

bool HyprlandAPI::sendNotification(const QString &title, const QString &message, int timeout)
{
    QProcess process;
    QStringList args;
    
    // Try to use notify-send if available
    QString notifySendPath = QStandardPaths::findExecutable("notify-send");
    if (!notifySendPath.isEmpty()) {
        args << "-a" << "Hypr Grid Manager" << title << message << "-t" << QString::number(timeout);
        process.start(notifySendPath, args);
    } else {
        // Fall back to using zenity
        QString zenityPath = QStandardPaths::findExecutable("zenity");
        if (!zenityPath.isEmpty()) {
            args << "--notification" << "--text" << QString("%1: %2").arg(title, message);
            process.start(zenityPath, args);
        } else {
            // No notification method available
            return false;
        }
    }
    
    process.waitForFinished(1000);
    return process.exitCode() == 0;
}

QString HyprlandAPI::executeHyprctlCommand(const QStringList &args) const
{
    QProcess process;
    process.start("hyprctl", args);
    process.waitForFinished(3000);
    
    QString output = process.readAllStandardOutput();
    QString errorOutput = process.readAllStandardError();
    
    if (process.exitCode() != 0) {
        qWarning() << "hyprctl error (exit code" << process.exitCode() << "):" << errorOutput;
        return errorOutput; // Return error output so we can check for error patterns
    }
    
    return output;
}

QString HyprlandAPI::executeHyprlandCommand(const QString &command) const
{
    return executeHyprctlCommand(QStringList() << "dispatch" << command);
}

QVariantMap HyprlandAPI::parseJsonOutput(const QString &output) const
{
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    
    if (doc.isObject()) {
        return doc.object().toVariantMap();
    }
    
    return QVariantMap();
}

QString HyprlandAPI::generateWindowRuleIdentifier() const
{
    // Generate a unique identifier for window rules
    return QString("gridmgr_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

bool HyprlandAPI::positionTiledWindow(int x, int y, int width, int height)
{
    // Make sure we're initialized
    if (!m_initialized) {
        emit errorOccurred("HyprlandAPI not initialized");
        return false;
    }
    
    // Update current window information and get initial state
    QVariantMap windowData = getFocusedWindowData();
    if (windowData.isEmpty()) {
        emit errorOccurred("No focused window");
        return false;
    }
    
    m_currentWindowAddress = windowData["address"].toString();
    
    // Log initial window state
    QJsonObject at = windowData["at"].toJsonObject();
    QJsonObject size = windowData["size"].toJsonObject();
    qDebug() << "BEFORE positioning:";
    qDebug() << "  Position: [" << at["x"].toInt() << "," << at["y"].toInt() << "]";
    qDebug() << "  Size: [" << size["x"].toInt() << "," << size["y"].toInt() << "]";
    qDebug() << "  Floating:" << windowData["floating"].toBool();
    
    // For precise positioning in tiling mode, we need to temporarily make the window floating
    // and then use exact positioning. The window will remain functionally "tiled" from the 
    // user's perspective but use floating for exact positioning.
    
    // First, ensure window is floating for precise positioning
    bool wasFloating = isWindowFloating();
    if (!wasFloating) {
        if (!toggleFloating()) {
            emit errorOccurred("Failed to make window floating for positioning");
            return false;
        }
    }
    
    // Apply the exact position using floating window commands
    QString moveCmd = QString("movewindowpixel exact %1 %2,address:%3")
        .arg(x).arg(y).arg(m_currentWindowAddress);
    
    QString resizeCmd = QString("resizewindowpixel exact %1 %2,address:%3")
        .arg(width).arg(height).arg(m_currentWindowAddress);
    
    QString moveResult = executeHyprlandCommand(moveCmd);
    QString resizeResult = executeHyprlandCommand(resizeCmd);
    
    // Check for errors
    bool moveSuccess = moveResult.trimmed().isEmpty() || 
                       (!moveResult.contains("error", Qt::CaseInsensitive) && 
                        !moveResult.contains("failed", Qt::CaseInsensitive));
    bool resizeSuccess = resizeResult.trimmed().isEmpty() || 
                         (!resizeResult.contains("error", Qt::CaseInsensitive) && 
                          !resizeResult.contains("failed", Qt::CaseInsensitive));
    
    // Get final window state to verify the changes
    QVariantMap finalWindowData = getFocusedWindowData();
    if (!finalWindowData.isEmpty()) {
        QJsonObject finalAt = finalWindowData["at"].toJsonObject();
        QJsonObject finalSize = finalWindowData["size"].toJsonObject();
        qDebug() << "AFTER positioning:";
        qDebug() << "  Position: [" << finalAt["x"].toInt() << "," << finalAt["y"].toInt() << "]";
        qDebug() << "  Size: [" << finalSize["x"].toInt() << "," << finalSize["y"].toInt() << "]";
        qDebug() << "  Floating:" << finalWindowData["floating"].toBool();
        qDebug() << "  Target was: [" << x << "," << y << "] size [" << width << "," << height << "]";
        
        // Check if position changed as expected
        int actualX = finalAt["x"].toInt();
        int actualY = finalAt["y"].toInt();
        int actualW = finalSize["x"].toInt();
        int actualH = finalSize["y"].toInt();
        
        bool positionMatch = (abs(actualX - x) <= 5) && (abs(actualY - y) <= 5);
        bool sizeMatch = (abs(actualW - width) <= 10) && (abs(actualH - height) <= 10);
        
        if (positionMatch && sizeMatch) {
            qDebug() << "✓ Window positioning successful!";
        } else {
            qDebug() << "⚠ Window positioning may have issues:";
            if (!positionMatch) {
                qDebug() << "  Position mismatch: got [" << actualX << "," << actualY << "] expected [" << x << "," << y << "]";
            }
            if (!sizeMatch) {
                qDebug() << "  Size mismatch: got [" << actualW << "," << actualH << "] expected [" << width << "," << height << "]";
            }
        }
    }
    
    return moveSuccess && resizeSuccess;
}

int HyprlandAPI::getCurrentWorkspaceId()
{
    QString output = executeHyprctlCommand(QStringList() << "activeworkspace" << "-j");
    QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
    
    if (!doc.isObject()) {
        emit errorOccurred("Failed to parse active workspace data");
        return -1;
    }
    
    return doc.object()["id"].toInt();
}
