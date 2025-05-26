#ifndef GRIDPREVIEW_H
#define GRIDPREVIEW_H

#include <QWidget>
#include <QVector>
#include <QPoint>
#include <QRect>

#include "gridcell.h"

class GridPreview : public QWidget
{
    Q_OBJECT
    
public:
    explicit GridPreview(QWidget *parent = nullptr);
    
    // Grid configuration
    void setGridDimensions(int rows, int columns);
    void setGaps(int gaps);
    
    // Selection management
    void setSelection(int x, int y, int width, int height);
    void clearSelection();
    
    // Get selection
    QRect getSelection() const { return m_selection; }
    
signals:
    void cellClicked(int row, int column);
    void cellEntered(int row, int column);
    void selectionChanged(int x, int y, int width, int height);
    
protected:
    void resizeEvent(QResizeEvent *event) override;
    
private:
    // Setup methods
    void setupGrid();
    void updateCellPositions();
    
    // Grid dimensions
    int m_rows;
    int m_columns;
    int m_gaps;
    
    // Selection
    QRect m_selection;
    bool m_isSelecting;
    QPoint m_selectionStart;
    
    // Grid cells
    QVector<QVector<GridCell*>> m_cells;
};

#endif // GRIDPREVIEW_H
