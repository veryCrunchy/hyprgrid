#include "gridcell.h"

#include <QPainter>
#include <QStyleOption>

GridCell::GridCell(int row, int column, QWidget *parent)
    : QWidget(parent), m_row(row), m_column(column), m_isSelected(false), m_isHovered(false)
{
    // Enable mouse tracking for hover effects
    setMouseTracking(true);
    
    // Set minimum size
    setMinimumSize(30, 30);
    
    // Set cursor
    setCursor(Qt::PointingHandCursor);
}

void GridCell::setSelected(bool selected)
{
    if (m_isSelected != selected) {
        m_isSelected = selected;
        update();
    }
}

void GridCell::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Calculate border thickness
    int borderWidth = width() / 15;
    borderWidth = qMax(1, qMin(borderWidth, 3));
    
    // Draw background
    QColor bgColor = m_isSelected ? QColor(26, 115, 232, 180) : QColor(240, 240, 240);
    if (m_isHovered && !m_isSelected) {
        bgColor = QColor(200, 200, 200);
    }
    
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    
    // Draw rounded rectangle
    painter.drawRoundedRect(borderWidth, borderWidth, 
                       width() - 2 * borderWidth, 
                       height() - 2 * borderWidth, 
                       5, 5);
    
    // Draw border
    QColor borderColor = QColor(160, 160, 160);
    if (m_isSelected) {
        borderColor = QColor(0, 90, 200);
    } else if (m_isHovered) {
        borderColor = QColor(100, 100, 100);
    }
    
    painter.setPen(QPen(borderColor, borderWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(borderWidth / 2, borderWidth / 2, 
                        width() - borderWidth, 
                        height() - borderWidth, 
                        5, 5);
}

void GridCell::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_row, m_column);
    }
    
    QWidget::mousePressEvent(event);
}

void GridCell::enterEvent(QEnterEvent *event)
{
    m_isHovered = true;
    update();
    
    emit entered(m_row, m_column);
    
    QWidget::enterEvent(event);
}
