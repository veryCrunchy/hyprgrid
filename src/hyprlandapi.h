#ifndef HYPRLANDAPI_H
#define HYPRLANDAPI_H

#include <QObject>
#include <QProcess>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>

class HyprlandAPI : public QObject
{
    Q_OBJECT
    
public:
    explicit HyprlandAPI(QObject *parent = nullptr);
    ~HyprlandAPI();
    
    // Initialization
    bool initialize();
    
    // Core window management functions
    bool moveAndResizeWindow(int x, int y, int width, int height);
    bool positionTiledWindow(int x, int y, int width, int height);
    bool toggleFloating();
    bool isWindowFloating();
    bool applyWindowRules(int x, int y, int width, int height);
    bool clearWindowRules();
    
    // Hyprland information functions
    QVariantMap getFocusedWindowData();
    QVariantMap getFocusedMonitorData();
    QVariantMap getWorkspaceData();
    int getCurrentWorkspaceId();
    QStringList getMonitors();
    
    // Notification function
    bool sendNotification(const QString &title, const QString &message, int timeout = 3000);
    
signals:
    void errorOccurred(const QString &message);
    
private:
    // Helper methods for executing Hyprland commands
    QString executeHyprctlCommand(const QStringList &args) const;
    QString executeHyprlandCommand(const QString &command) const;
    
    // Parse JSON results from hyprctl
    QVariantMap parseJsonOutput(const QString &output) const;
    
    // Generate unique window rules
    QString generateWindowRuleIdentifier() const;
    QString m_currentWindowRuleIdentifier;
    
    // Store current window information
    QString m_currentWindowAddress;
    bool m_initialized;
};

#endif // HYPRLANDAPI_H
