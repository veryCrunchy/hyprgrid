#include "gridpreview.h"

#include <QGridLayout>
#include <QResizeEvent>
#include <QDebug>

GridPreview::GridPreview(QWidget *parent)
    : QWidget(parent), m_rows(3), m_columns(3), m_gaps(5), m_isSelecting(false)
{
    // Set up default grid
    setupGrid();
}

void GridPreview::setGridDimensions(int rows, int columns)
{
    if (rows == m_rows && columns == m_columns) {
        return;
    }
    
    m_rows = rows;
    m_columns = columns;
    
    // Recreate the grid
    setupGrid();
}

void GridPreview::setGaps(int gaps)
{
    m_gaps = gaps;
    updateCellPositions();
}

void GridPreview::setSelection(int x, int y, int width, int height)
{
    // Validate parameters
    x = qMax(0, qMin(x, m_columns - 1));
    y = qMax(0, qMin(y, m_rows - 1));
    width = qMax(1, qMin(width, m_columns - x));
    height = qMax(1, qMin(height, m_rows - y));
    
    // Clear previous selection
    clearSelection();
    
    // Set new selection
    m_selection = QRect(x, y, width, height);
    
    // Update cell states
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_columns; ++col) {
            bool selected = (col >= x && col < x + width && row >= y && row < y + height);
            m_cells[row][col]->setSelected(selected);
        }
    }
    
    emit selectionChanged(x, y, width, height);
}

void GridPreview::clearSelection()
{
    m_selection = QRect();
    
    // Clear all cell selections
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_columns; ++col) {
            if (m_cells.size() > row && m_cells[row].size() > col) {
                m_cells[row][col]->setSelected(false);
            }
        }
    }
}

void GridPreview::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    updateCellPositions();
}

void GridPreview::setupGrid()
{
    // Clear any existing cells
    for (auto &row : m_cells) {
        for (auto cell : row) {
            delete cell;
        }
    }
    m_cells.clear();
    
    // Create new grid cells
    m_cells.resize(m_rows);
    
    for (int row = 0; row < m_rows; ++row) {
        m_cells[row].resize(m_columns);
        
        for (int col = 0; col < m_columns; ++col) {
            GridCell *cell = new GridCell(row, col, this);
            m_cells[row][col] = cell;
            
            // Connect signals
            connect(cell, &GridCell::clicked, this, &GridPreview::cellClicked);
            connect(cell, &GridCell::entered, this, &GridPreview::cellEntered);
        }
    }
    
    // Position the cells properly
    updateCellPositions();
}

void GridPreview::updateCellPositions()
{
    if (m_cells.isEmpty()) return;
    
    int availableWidth = width();
    int availableHeight = height();
    
    // Calculate cell dimensions
    int cellWidth = (availableWidth - (m_columns - 1) * m_gaps) / m_columns;
    int cellHeight = (availableHeight - (m_rows - 1) * m_gaps) / m_rows;
    
    // Ensure minimum cell size
    cellWidth = qMax(10, cellWidth);
    cellHeight = qMax(10, cellHeight);
    
    // Position cells
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_columns; ++col) {
            int x = col * (cellWidth + m_gaps);
            int y = row * (cellHeight + m_gaps);
            
            m_cells[row][col]->setGeometry(x, y, cellWidth, cellHeight);
            m_cells[row][col]->show();
        }
    }
}
