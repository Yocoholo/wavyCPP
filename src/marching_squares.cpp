#include "marching_squares.hpp"

namespace MarchingSquares {

int getOperation(const Dot* dots, int index, int xCount) {
    return (dots[index].value >= 0.5f ? 1 : 0)
         + (dots[index + 1].value >= 0.5f ? 2 : 0)
         + (dots[index + xCount + 1].value >= 0.5f ? 4 : 0)
         + (dots[index + xCount].value >= 0.5f ? 8 : 0);
}

float mid(const Dot* d1, const Dot* d2, bool x) {
    float v1 = x ? d1->x : d1->y;
    float v2 = x ? d2->x : d2->y;
    float denom = d1->value + d2->value;
    if (denom < 1e-6f) return (v1 + v2) * 0.5f;
    return (v1 * d1->value + v2 * d2->value) / denom;
}

float edgeUpX(const Dot* dots, int index, int xCount)    { (void)xCount; return mid(&dots[index], &dots[index + 1], true); }
float edgeUpY(const Dot* dots, int index)                { return dots[index].y; }
float edgeDownX(const Dot* dots, int index, int xCount)  { return mid(&dots[index + xCount], &dots[index + xCount + 1], true); }
float edgeDownY(const Dot* dots, int index, int xCount)  { return dots[index + xCount].y; }
float edgeLeftX(const Dot* dots, int index)              { return dots[index].x; }
float edgeLeftY(const Dot* dots, int index, int xCount)  { return mid(&dots[index], &dots[index + xCount], false); }
float edgeRightX(const Dot* dots, int index, int xCount) { return dots[index + 1].x; }
float edgeRightY(const Dot* dots, int index, int xCount) { return mid(&dots[index + 1], &dots[index + xCount + 1], false); }

void getLinePos(const Dot* dots, int index, int xCount, float* pos) {
    int op = getOperation(dots, index, xCount);
    for (int i = 0; i < 8; i++) pos[i] = -1.0f;

    switch (op) {
    case 1:
    case 14:
        pos[0] = edgeUpX(dots, index, xCount);
        pos[1] = edgeUpY(dots, index);
        pos[2] = edgeLeftX(dots, index);
        pos[3] = edgeLeftY(dots, index, xCount);
        break;
    case 2:
    case 13:
        pos[0] = edgeUpX(dots, index, xCount);
        pos[1] = edgeUpY(dots, index);
        pos[2] = edgeRightX(dots, index, xCount);
        pos[3] = edgeRightY(dots, index, xCount);
        break;
    case 4:
    case 11:
        pos[0] = edgeDownX(dots, index, xCount);
        pos[1] = edgeDownY(dots, index, xCount);
        pos[2] = edgeRightX(dots, index, xCount);
        pos[3] = edgeRightY(dots, index, xCount);
        break;
    case 8:
    case 7:
        pos[0] = edgeDownX(dots, index, xCount);
        pos[1] = edgeDownY(dots, index, xCount);
        pos[2] = edgeLeftX(dots, index);
        pos[3] = edgeLeftY(dots, index, xCount);
        break;
    case 3:
    case 12:
        pos[0] = edgeLeftX(dots, index);
        pos[1] = edgeLeftY(dots, index, xCount);
        pos[2] = edgeRightX(dots, index, xCount);
        pos[3] = edgeRightY(dots, index, xCount);
        break;
    case 6:
    case 9:
        pos[0] = edgeUpX(dots, index, xCount);
        pos[1] = edgeUpY(dots, index);
        pos[2] = edgeDownX(dots, index, xCount);
        pos[3] = edgeDownY(dots, index, xCount);
        break;
    case 5:
        pos[0] = edgeUpX(dots, index, xCount);
        pos[1] = edgeUpY(dots, index);
        pos[2] = edgeRightX(dots, index, xCount);
        pos[3] = edgeRightY(dots, index, xCount);
        pos[4] = edgeDownX(dots, index, xCount);
        pos[5] = edgeDownY(dots, index, xCount);
        pos[6] = edgeLeftX(dots, index);
        pos[7] = edgeLeftY(dots, index, xCount);
        break;
    case 10:
        pos[0] = edgeUpX(dots, index, xCount);
        pos[1] = edgeUpY(dots, index);
        pos[2] = edgeLeftX(dots, index);
        pos[3] = edgeLeftY(dots, index, xCount);
        pos[4] = edgeDownX(dots, index, xCount);
        pos[5] = edgeDownY(dots, index, xCount);
        pos[6] = edgeRightX(dots, index, xCount);
        pos[7] = edgeRightY(dots, index, xCount);
        break;
    default:
        break;
    }
}

} // namespace MarchingSquares
