#ifndef UVRECT_H
#define UVRECT_H

struct UVRect {
    float uMin, vMin;
    float uMax, vMax;
};

UVRect GetUVRect(int row, int col, int totalRows, int totalCols) {
    float cellWidth = 1.0f / totalCols;
    float cellHeight = 1.0f / totalRows;

    UVRect rect;
    rect.uMin = col * cellWidth;
    rect.vMin = 1.0f - (row + 1) * cellHeight; // OpenGL UV origin is bottom-left
    rect.uMax = (col + 1) * cellWidth;
    rect.vMax = 1.0f - row * cellHeight;

    return rect;
}


#endif //UVRECT_H
