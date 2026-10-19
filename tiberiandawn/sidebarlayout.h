// Stateless layout helpers: no native save object layout changes.
#ifndef SIDEBARLAYOUT_H
#define SIDEBARLAYOUT_H
namespace SidebarLayout {
constexpr int Columns = 2;
constexpr int Rows = 8;
constexpr int Visible = Columns * Rows;
constexpr int Width = 80;
inline int Column(int slot) { return slot % Columns; }
inline int Row(int slot) { return slot / Columns; }
inline int MaximumTop(int count) {
    return count <= Visible ? 0 : ((count - Visible + Columns - 1) / Columns) * Columns;
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
