#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QPushButton>

#include "gridmanager.h"
#include "gridcell.h"
#include "gridpreview.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(GridManager &gridManager, QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    // UI interaction handlers
    void onPresetSelected(int index);
    void onPositionSelected(const QString &code);
    void onGridCellClicked(int row, int column);
    void onApplyButtonClicked();
    void onResetButtonClicked();
    void onSaveButtonClicked();
    
    // Settings handlers
    void onSaveSettingsClicked();
    void onCancelSettingsClicked();
    void onAddPresetClicked();
    void onRemovePresetClicked();
    void onAddPositionClicked();
    void onRemovePositionClicked();
    
    // Grid editor handlers
    void onGridEditStarted();
    void onGridEditCancelled();
    void onGridEditApplied();
    
    // Error handling
    void onErrorOccurred(const QString &message);
    
private:
    // UI setup
    void setupUI();
    void setupConnections();
    void refreshPresetList();
    void refreshPositionList();
    void updateGridPreview();
    void updateCurrentPosition();
    
    // Configuration handling
    void loadSettings();
    void saveSettings();
    
    // Grid editor
    void showGridEditor();
    
    // UI elements
    Ui::MainWindow *ui;
    GridPreview *m_gridPreview;
    QMap<QString, QPushButton*> m_positionButtons;
    
    // Grid manager reference
    GridManager &m_gridManager;
    
    // Current state
    QString m_currentPreset;
    QString m_currentPositionCode;
    GridPosition m_currentPosition;
    bool m_isEditingGrid;
};

#endif // MAINWINDOW_H
