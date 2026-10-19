// Layout state stays outside serialized native game objects.
#ifndef SIDEBARLAYOUT_H
#define SIDEBARLAYOUT_H
namespace SidebarLayout {
constexpr int Columns = 2;
constexpr int Rows = 8;
constexpr int Visible = Columns * Rows;
constexpr int Width = 120;
constexpr int StripWidth = 53;
constexpr int IconHeight = 18;
constexpr int Capacity = 30;
inline int& ActiveRows() { static int rows = Rows; return rows; }
inline int VisibleCount() { return ActiveRows() * Columns; }
inline void ConfigureRows(int height, int top, int rowHeight, int arrowHeight) {
    int rows = (height - top - arrowHeight) / rowHeight;
    ActiveRows() = rows < 1 ? 1 : (rows > Capacity / Columns ? Capacity / Columns : rows);
}
inline int Column(int slot) { return slot % Columns; }
inline int Row(int slot) { return slot / Columns; }
inline int MaximumTop(int count) {
    return count <= VisibleCount() ? 0 : ((count - VisibleCount() + Columns - 1) / Columns) * Columns;
}
inline int ClampTop(int top, int count) {
    if (top < 0) return 0;
    top = (top / Columns) * Columns;
    return top > MaximumTop(count) ? MaximumTop(count) : top;
}
inline int ScrollTop(int top, int count, bool up) {
    return ClampTop(ClampTop(top,count) + (up ? -Columns : Columns),count);
}
inline int RadarInset(int factor) { return ((Width - 80) / 2) << factor; }
}
#endif
