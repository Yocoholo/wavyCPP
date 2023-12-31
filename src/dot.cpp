#include <OpenSimplex.cpp>

class dot
{
private:
    /* data */
public:
    float x;
    float y;
    float s_x;
    float s_y;
    float value;
    float simplex_X;
    float simplex_Y;    
    OpenSimplex* noise;

    dot(/* args */);
    dot(float x, float y, float s_x, float s_y, float value, OpenSimplex* noise)
    {
        this->x = x;
        this->y = y;
        this->value = value;
        this->noise = noise;
        this->s_x = s_x;
        this->s_y = s_y;
        this->simplex_X = s_x/20;
        this->simplex_Y = s_y/20;
    }

    ~dot();
    void update(float time){
        double res = this->noise->noise3_XYBeforeZ(simplex_X, simplex_Y, time);
        res = (res + 1) / 2;
        this->value = float(res);
    }
};

dot::dot(){}
dot::~dot(){}
