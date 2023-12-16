import math
import sys

proj_src            = [0.974279, 0.000000, 0.000000, 0.000000, 0.000000, 1.732051, 0.000000, 0.000000, 0.000000, 0.000000, 1.001001, 1.000000, 0.000000, 0.000000, -0.100100, 0.000000]
inv_projection_mtx  = [1.026401, 0.000000, 0.000000, 0.000000, 0.000000, 0.577350, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, -9.990001, 0.000000, 0.000000, 1.000000, 10.000000]

full_domain = [
    [1.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000],
    [-1.000000, -1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000],
    [-1.000000, 1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000],
    [1.000000, -1.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000],
]


def mtxInverse(a):
    xx = a[0]
    xy = a[1]
    xz = a[2]
    xw = a[3]
    yx = a[4]
    yy = a[5]
    yz = a[6]
    yw = a[7]
    zx = a[8]
    zy = a[9]
    zz = a[10]
    zw = a[11]
    wx = a[12]
    wy = a[13]
    wz = a[14]
    ww = a[15]

    det = 0.0
    det += xx * (yy*(zz*ww - zw*wz) - yz*(zy*ww - wy*zw) + yw*(zy*wz - zz*wy))
    det -= xy * (yx*(zz*ww - zw*wz) - yz*(zx*ww - zw*wx) + yw*(zx*wz - zz*wx))
    det += xz * (yx*(zy*ww - zw*wy) - yy*(zx*ww - zw*wx) + yw*(zx*wy - zy*wx))
    det -= xw * (yx*(zy*wz - zz*wy) - yy*(zx*wz - zz*wx) + yz*(zx*wy - zy*wx))

    invDet = 1.0 / det

    return [    
        +(yy*(zz*ww - wz*zw) - yz*(zy*ww - wy*zw) + yw*(zy*wz - wy*zz)) * invDet,   # 0     x
        -(xy*(zz*ww - wz*zw) - xz*(zy*ww - wy*zw) + xw*(zy*wz - wy*zz)) * invDet,   # 1     x
        +(xy*(yz*ww - wz*yw) - xz*(yy*ww - wy*yw) + xw*(yy*wz - wy*yz)) * invDet,   # 2     x
        -(xy*(yz*zw - zz*yw) - xz*(yy*zw - zy*yw) + xw*(yy*zz - zy*yz)) * invDet,   # 3     x
        
        -(yx*(zz*ww - wz*zw) - yz*(zx*ww - wx*zw) + yw*(zx*wz - wx*zz)) * invDet,   # 4
        +(xx*(zz*ww - wz*zw) - xz*(zx*ww - wx*zw) + xw*(zx*wz - wx*zz)) * invDet,   # 5
        -(xx*(yz*ww - wz*yw) - xz*(yx*ww - wx*yw) + xw*(yx*wz - wx*yz)) * invDet,   # 6
        +(xx*(yz*zw - zz*yw) - xz*(yx*zw - zx*yw) + xw*(yx*zz - zx*yz)) * invDet,   # 7
        
        +(yx*(zy*ww - wy*zw) - yy*(zx*ww - wx*zw) + yw*(zx*wy - wx*zy)) * invDet,   # 8
        -(xx*(zy*ww - wy*zw) - xy*(zx*ww - wx*zw) + xw*(zx*wy - wx*zy)) * invDet,   # 9
        +(xx*(yy*ww - wy*yw) - xy*(yx*ww - wx*yw) + xw*(yx*wy - wx*yy)) * invDet,   # 10
        -(xx*(yy*zw - zy*yw) - xy*(yx*zw - zx*yw) + xw*(yx*zy - zx*yy)) * invDet,   # 11
        
        -(yx*(zy*wz - wy*zz) - yy*(zx*wz - wx*zz) + yz*(zx*wy - wx*zy)) * invDet,   # 12
        +(xx*(zy*wz - wy*zz) - xy*(zx*wz - wx*zz) + xz*(zx*wy - wx*zy)) * invDet,   # 13
        -(xx*(yy*wz - wy*yz) - xy*(yx*wz - wx*yz) + xz*(yx*wy - wx*yy)) * invDet,   # 14
        +(xx*(yy*zz - zy*yz) - xy*(yx*zz - zx*yz) + xz*(yx*zy - zx*yy)) * invDet    # 15
    ]



def mtxProjXYWH(x, y, width, height, near, far):
    diff = far - near
    aa =  far / diff
    bb =  near * aa

    return [width, 0.0, 0.0, 0.0, 0.0, height, 0.0, 0.0, -x, -y, aa, 1.0, 0.0, 0.0, -bb, 0.0] 


def toRad(_deg):
    return _deg * 3.1415926535897932384626433832795 / 180.0


def mtxProj (_fovy, _aspect, _near, _far):
    height = 1.0/math.tan(toRad(_fovy)*0.5)
    width  = height * 1.0/_aspect
    return mtxProjXYWH(0.0, 0.0, width, height, _near, _far)



def vec4MulMtx(_vec, _mat, _start):
    _result =[
        _vec[_start] * _mat[ 0] + _vec[_start + 1] * _mat[4] + _vec[_start + 2] * _mat[ 8] + _vec[_start + 3] * _mat[12],
        _vec[_start] * _mat[ 1] + _vec[_start + 1] * _mat[5] + _vec[_start + 2] * _mat[ 9] + _vec[_start + 3] * _mat[13],
        _vec[_start] * _mat[ 2] + _vec[_start + 1] * _mat[6] + _vec[_start + 2] * _mat[10] + _vec[_start + 3] * _mat[14],
        _vec[_start] * _mat[ 3] + _vec[_start + 1] * _mat[7] + _vec[_start + 2] * _mat[11] + _vec[_start + 3] * _mat[15]
    ]
    return _result


def mtxMul(_a, _b):
    _result =[
        vec4MulMtx(_a, _b, 0),
        vec4MulMtx(_a, _b, 4),
        vec4MulMtx(_a, _b, 8),
        vec4MulMtx(_a, _b, 12)
    ]
    return _result
        
def print_proj_mat():
    proj = mtxProj(120.0, 3440/1440.0, 0.1, 100.0)
    print(sys._getframe().f_code.co_name)
    print(proj)
    return proj

def print_inverse_proj_mat():
    inv_proj = mtxInverse(print_proj_mat())
    print(sys._getframe().f_code.co_name)
    print(inv_proj)
    return inv_proj
    

def print_world_pos():
    inv_proj = print_inverse_proj_mat()
    print(sys._getframe().f_code.co_name)
    for domain in full_domain:
        world_pos = mtxMul(inv_proj, domain)
        print(world_pos)
        
def get_bounds():
    
    print("bounds")    
    
print_world_pos()