#ifndef GRIDCELL_H
#define GRIDCELL_H

#include <QWidget>
#include <QMouseEvent>

class GridCell : public QWidget
{
    Q_OBJECT
    
public:
    explicit GridCell(int row, int column, QWidget *parent = nullptr);
    
    // State management
    void setSelected(bool selected);
    bool isSelected() const { return m_isSelected; }
    
    // Position getters
    int getRow() const { return m_row; }
    int getColumn() const { return m_column; }
    
signals:
    void clicked(int row, int column);
    void entered(int row, int column);
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    
private:
    int m_row;
    int m_column;
    bool m_isSelected;
    bool m_isHovered;
};

#endif // GRIDCELL_H
