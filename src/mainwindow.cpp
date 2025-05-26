#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QGridLayout>
#include <QSpinBox>
#include <QCheckBox>

MainWindow::MainWindow(GridManager &gridManager, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_gridManager(gridManager), m_isEditingGrid(false)
{
    ui->setupUi(this);
    
    // Set window properties
    setWindowTitle(tr("Hypr Grid Manager"));
    setMinimumSize(800, 600);
    
    // Setup the UI components
    setupUI();
    setupConnections();
    
    // Load settings and refresh UI
    loadSettings();
    refreshPresetList();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupUI()
{
    // Create the grid preview
    m_gridPreview = new GridPreview(this);
    m_gridPreview->setMinimumSize(300, 200);
    ui->previewLayout->addWidget(m_gridPreview);
    
    // Populate rows/columns spinboxes with config values
    int rows = m_gridManager.getConfig()->getGridConfig()["rows"].toInt();
    int cols = m_gridManager.getConfig()->getGridConfig()["columns"].toInt();
    ui->rowsSpinBox->setValue(rows);
    ui->columnsSpinBox->setValue(cols);
    
    // Setup grid editor
    ui->gridEditorWidget->setVisible(false);
    
    // Set up gaps spinbox
    ui->gapsSpinBox->setValue(m_gridManager.getConfig()->getGridConfig()["gaps"].toInt());
    
    // Set up advanced options
    ui->floatingOnlyCheckBox->setChecked(m_gridManager.getConfig()->getAdvancedConfig()["floatingOnly"].toBool());
    ui->forceFloatCheckBox->setChecked(m_gridManager.getConfig()->getAdvancedConfig()["forceFloat"].toBool());
    ui->retryFailureCheckBox->setChecked(m_gridManager.getConfig()->getAdvancedConfig()["retryOnFailure"].toBool());
    ui->showNotificationsCheckBox->setChecked(m_gridManager.getConfig()->getAppearanceConfig()["showNotifications"].toBool());
    
    // Set up log level combo
    ui->logLevelCombo->addItems(QStringList() << "debug" << "info" << "warn" << "error");
    ui->logLevelCombo->setCurrentText(m_gridManager.getConfig()->getAdvancedConfig()["logLevel"].toString());
}

void MainWindow::setupConnections()
{
    // Grid manager connections
    connect(&m_gridManager, &GridManager::errorOccurred, this, &MainWindow::onErrorOccurred);
    
    // Preset selection
    connect(ui->presetComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &MainWindow::onPresetSelected);
    
    // Button connections
    connect(ui->applyButton, &QPushButton::clicked, this, &MainWindow::onApplyButtonClicked);
    connect(ui->resetButton, &QPushButton::clicked, this, &MainWindow::onResetButtonClicked);
    connect(ui->saveButton, &QPushButton::clicked, this, &MainWindow::onSaveButtonClicked);
    
    // Settings buttons
    connect(ui->saveSettingsButton, &QPushButton::clicked, this, &MainWindow::onSaveSettingsClicked);
    connect(ui->cancelSettingsButton, &QPushButton::clicked, this, &MainWindow::onCancelSettingsClicked);
    
    // Preset management
    connect(ui->addPresetButton, &QPushButton::clicked, this, &MainWindow::onAddPresetClicked);
    connect(ui->removePresetButton, &QPushButton::clicked, this, &MainWindow::onRemovePresetClicked);
    connect(ui->addPositionButton, &QPushButton::clicked, this, &MainWindow::onAddPositionClicked);
    connect(ui->removePositionButton, &QPushButton::clicked, this, &MainWindow::onRemovePositionClicked);
    
    // Grid editor connections
    connect(ui->editGridButton, &QPushButton::clicked, this, &MainWindow::onGridEditStarted);
    connect(ui->cancelEditButton, &QPushButton::clicked, this, &MainWindow::onGridEditCancelled);
    connect(ui->applyEditButton, &QPushButton::clicked, this, &MainWindow::onGridEditApplied);
    
    // Connect the grid preview signals
    connect(m_gridPreview, &GridPreview::cellClicked, this, &MainWindow::onGridCellClicked);
}

void MainWindow::loadSettings()
{
    // Nothing to do here since settings are loaded by the grid manager
}

void MainWindow::saveSettings()
{
    // Get values from UI
    QVariantMap gridConfig;
    gridConfig["rows"] = ui->rowsSpinBox->value();
    gridConfig["columns"] = ui->columnsSpinBox->value();
    gridConfig["gaps"] = ui->gapsSpinBox->value();
    
    // Update GridPreview dimensions
    m_gridPreview->setGridDimensions(ui->rowsSpinBox->value(), ui->columnsSpinBox->value());
    
    // Advanced settings
    QVariantMap advancedConfig = m_gridManager.getConfig()->getAdvancedConfig();
    advancedConfig["floatingOnly"] = ui->floatingOnlyCheckBox->isChecked();
    advancedConfig["forceFloat"] = ui->forceFloatCheckBox->isChecked();
    advancedConfig["retryOnFailure"] = ui->retryFailureCheckBox->isChecked();
    advancedConfig["logLevel"] = ui->logLevelCombo->currentText();
    
    // Appearance settings
    QVariantMap appearanceConfig = m_gridManager.getConfig()->getAppearanceConfig();
    appearanceConfig["showNotifications"] = ui->showNotificationsCheckBox->isChecked();
    
    // Update the config
    m_gridManager.getConfig()->setGridConfig(gridConfig);
    m_gridManager.getConfig()->setAdvancedConfig(advancedConfig);
    m_gridManager.getConfig()->setAppearanceConfig(appearanceConfig);
    
    // Save to disk
    m_gridManager.getConfig()->save();
}

void MainWindow::refreshPresetList()
{
    ui->presetComboBox->clear();
    
    QStringList presetNames = m_gridManager.getPresetNames();
    ui->presetComboBox->addItems(presetNames);
    
    // Select the first preset if available
    if (!presetNames.isEmpty()) {
        ui->presetComboBox->setCurrentIndex(0);
        onPresetSelected(0);
    }
}

void MainWindow::refreshPositionList()
{
    // Clear existing buttons
    for (auto btn : m_positionButtons) {
        delete btn;
    }
    m_positionButtons.clear();
    
    // Clear layout
    QLayoutItem *item;
    while ((item = ui->positionsLayout->takeAt(0)) != nullptr) {
        delete item;
    }
    
    // Get current preset
    m_currentPreset = ui->presetComboBox->currentText();
    if (m_currentPreset.isEmpty()) return;
    
    // Get positions for this preset
    QStringList positions = m_gridManager.getPositionCodesForPreset(m_currentPreset);
    
    // Create buttons for each position
    int row = 0, col = 0;
    const int maxCols = 4; // Limit columns for better layout
    
    for (const QString &pos : positions) {
        QPushButton *btn = new QPushButton(pos, this);
        btn->setCheckable(true);
        
        connect(btn, &QPushButton::clicked, this, [this, pos]() {
            onPositionSelected(pos);
        });
        
        ui->positionsLayout->addWidget(btn, row, col);
        m_positionButtons[pos] = btn;
        
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
    }
    
    // Select the first position if available
    if (!positions.isEmpty()) {
        onPositionSelected(positions.first());
    }
}

void MainWindow::updateGridPreview()
{
    if (m_currentPreset.isEmpty() || m_currentPositionCode.isEmpty()) {
        m_gridPreview->clearSelection();
        return;
    }
    
    // Get the current position
    m_currentPosition = m_gridManager.getGridPosition(m_currentPreset, m_currentPositionCode);
    
    // Update the grid preview
    m_gridPreview->setGridDimensions(
        m_gridManager.getConfig()->getGridConfig()["rows"].toInt(),
        m_gridManager.getConfig()->getGridConfig()["columns"].toInt()
    );
    
    m_gridPreview->setSelection(m_currentPosition.x, m_currentPosition.y, 
                             m_currentPosition.width, m_currentPosition.height);
}

void MainWindow::updateCurrentPosition()
{
    if (m_currentPreset.isEmpty() || m_currentPositionCode.isEmpty() || m_isEditingGrid) {
        return;
    }
    
    // Update UI with position details
    ui->xSpinBox->setValue(m_currentPosition.x);
    ui->ySpinBox->setValue(m_currentPosition.y);
    ui->widthSpinBox->setValue(m_currentPosition.width);
    ui->heightSpinBox->setValue(m_currentPosition.height);
    ui->centeredCheckBox->setChecked(m_currentPosition.centered);
    ui->scaleSpinBox->setValue(m_currentPosition.scale);
}

void MainWindow::onPresetSelected(int index)
{
    if (index < 0) return;
    
    m_currentPreset = ui->presetComboBox->itemText(index);
    refreshPositionList();
}

void MainWindow::onPositionSelected(const QString &code)
{
    // Uncheck all buttons
    for (auto btn : m_positionButtons) {
        btn->setChecked(false);
    }
    
    // Check the selected button
    if (m_positionButtons.contains(code)) {
        m_positionButtons[code]->setChecked(true);
    }
    
    m_currentPositionCode = code;
    updateGridPreview();
    updateCurrentPosition();
}

void MainWindow::onGridCellClicked(int row, int column)
{
    if (!m_isEditingGrid) return;
    
    // Update the spinboxes
    ui->xSpinBox->setValue(column);
    ui->ySpinBox->setValue(row);
}

void MainWindow::onApplyButtonClicked()
{
    if (m_currentPreset.isEmpty() || m_currentPositionCode.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a preset and position"));
        return;
    }
    
    // Apply the position
    bool success = m_gridManager.applyPositionByCode(m_currentPreset, m_currentPositionCode);
    
    if (!success) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to apply position"));
    }
}

void MainWindow::onResetButtonClicked()
{
    bool success = m_gridManager.resetWindowState();
    
    if (!success) {
        QMessageBox::warning(this, tr("Error"), tr("Failed to reset window state"));
    }
}

void MainWindow::onSaveButtonClicked()
{
    if (m_currentPreset.isEmpty() || m_currentPositionCode.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a preset and position"));
        return;
    }
    
    // Get values from UI
    GridPosition position;
    position.x = ui->xSpinBox->value();
    position.y = ui->ySpinBox->value();
    position.width = ui->widthSpinBox->value();
    position.height = ui->heightSpinBox->value();
    position.centered = ui->centeredCheckBox->isChecked();
    position.scale = ui->scaleSpinBox->value();
    
    // Save the position
    m_gridManager.saveGridPosition(m_currentPreset, m_currentPositionCode, position);
    
    // Update the preview
    m_currentPosition = position;
    updateGridPreview();
    
    QMessageBox::information(this, tr("Success"), tr("Position saved"));
}

void MainWindow::onSaveSettingsClicked()
{
    saveSettings();
    QMessageBox::information(this, tr("Success"), tr("Settings saved"));
}

void MainWindow::onCancelSettingsClicked()
{
    // Reload settings from disk
    m_gridManager.getConfig()->load();
    loadSettings();
    
    ui->tabWidget->setCurrentIndex(0);
}

void MainWindow::onAddPresetClicked()
{
    bool ok;
    QString presetName = QInputDialog::getText(this, tr("Add Preset"),
                                           tr("Preset name:"), QLineEdit::Normal,
                                           tr("New Preset"), &ok);
    
    if (ok && !presetName.isEmpty()) {
        // Add the preset to config
        QMap<QString, QMap<QString, QVariantMap>> presets = m_gridManager.getConfig()->getPresets();
        presets[presetName] = QMap<QString, QVariantMap>();
        m_gridManager.getConfig()->setPresets(presets);
        m_gridManager.getConfig()->save();
        
        // Refresh the preset list
        refreshPresetList();
        
        // Select the new preset
        ui->presetComboBox->setCurrentText(presetName);
    }
}

void MainWindow::onRemovePresetClicked()
{
    QString presetName = ui->presetComboBox->currentText();
    
    if (presetName.isEmpty()) return;
    
    int reply = QMessageBox::question(this, tr("Confirm"),
                                  tr("Are you sure you want to remove preset '%1'?").arg(presetName),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Remove the preset
        QMap<QString, QMap<QString, QVariantMap>> presets = m_gridManager.getConfig()->getPresets();
        presets.remove(presetName);
        m_gridManager.getConfig()->setPresets(presets);
        m_gridManager.getConfig()->save();
        
        // Refresh the preset list
        refreshPresetList();
    }
}

void MainWindow::onAddPositionClicked()
{
    QString presetName = ui->presetComboBox->currentText();
    
    if (presetName.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a preset first"));
        return;
    }
    
    bool ok;
    QString positionCode = QInputDialog::getText(this, tr("Add Position"),
                                             tr("Position code:"), QLineEdit::Normal,
                                             tr("new-position"), &ok);
    
    if (ok && !positionCode.isEmpty()) {
        // Create default position
        GridPosition position;
        position.x = 0;
        position.y = 0;
        position.width = 1;
        position.height = 1;
        
        // Add the position
        m_gridManager.saveGridPosition(presetName, positionCode, position);
        
        // Refresh the position list
        refreshPositionList();
        
        // Select the new position
        onPositionSelected(positionCode);
    }
}

void MainWindow::onRemovePositionClicked()
{
    QString presetName = ui->presetComboBox->currentText();
    QString positionCode = m_currentPositionCode;
    
    if (presetName.isEmpty() || positionCode.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a position first"));
        return;
    }
    
    int reply = QMessageBox::question(this, tr("Confirm"),
                                  tr("Are you sure you want to remove position '%1'?").arg(positionCode),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // Remove the position
        QMap<QString, QMap<QString, QVariantMap>> presets = m_gridManager.getConfig()->getPresets();
        presets[presetName].remove(positionCode);
        m_gridManager.getConfig()->setPresets(presets);
        m_gridManager.getConfig()->save();
        
        // Refresh the position list
        refreshPositionList();
    }
}

void MainWindow::onGridEditStarted()
{
    m_isEditingGrid = true;
    ui->gridEditorWidget->setVisible(true);
    ui->positionDetailsWidget->setVisible(false);
}

void MainWindow::onGridEditCancelled()
{
    m_isEditingGrid = false;
    ui->gridEditorWidget->setVisible(false);
    ui->positionDetailsWidget->setVisible(true);
    
    // Reset to current position
    updateCurrentPosition();
    updateGridPreview();
}

void MainWindow::onGridEditApplied()
{
    if (m_currentPreset.isEmpty() || m_currentPositionCode.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please select a preset and position first"));
        return;
    }
    
    // Get the selection from grid editor
    GridPosition position;
    position.x = ui->xSpinBox->value();
    position.y = ui->ySpinBox->value();
    position.width = ui->widthSpinBox->value();
    position.height = ui->heightSpinBox->value();
    position.centered = ui->centeredCheckBox->isChecked();
    position.scale = ui->scaleSpinBox->value();
    
    // Save the position
    m_gridManager.saveGridPosition(m_currentPreset, m_currentPositionCode, position);
    
    // Update UI
    m_currentPosition = position;
    updateGridPreview();
    
    // Exit edit mode
    m_isEditingGrid = false;
    ui->gridEditorWidget->setVisible(false);
    ui->positionDetailsWidget->setVisible(true);
}

void MainWindow::onErrorOccurred(const QString &message)
{
    QMessageBox::critical(this, tr("Error"), message);
}
