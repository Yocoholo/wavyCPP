#pragma once

#include "grid.hpp"

namespace MarchingSquares {

// Returns the marching squares case (0–15) for the cell at grid index.
int getOperation(const Dot* dots, int index, int xCount);

// Interpolates the midpoint between two dots along the given axis.
// x=true interpolates x coordinates, x=false interpolates y coordinates.
float mid(const Dot* d1, const Dot* d2, bool x);

// Edge interpolation helpers — return interpolated coordinates for each cell edge.
float edgeUpX(const Dot* dots, int index, int xCount);
float edgeUpY(const Dot* dots, int index);
float edgeDownX(const Dot* dots, int index, int xCount);
float edgeDownY(const Dot* dots, int index, int xCount);
float edgeLeftX(const Dot* dots, int index);
float edgeLeftY(const Dot* dots, int index, int xCount);
float edgeRightX(const Dot* dots, int index, int xCount);
float edgeRightY(const Dot* dots, int index, int xCount);

// Fills pos[8] with line segment endpoints for the cell at grid index.
// pos[0..3] = first line (x1,y1,x2,y2), pos[4..7] = second line (or -1 if none).
void getLinePos(const Dot* dots, int index, int xCount, float* pos);

} // namespace MarchingSquares
