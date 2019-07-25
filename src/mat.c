#include "platform.h"

#include "mat.h"
#include "log.h"

mat4_t mat4_identity() {
    mat4_t out;
    memset(&out, 0, sizeof(float) * 4 * 4);
    for (unsigned i=0; i < 4; ++i) {
        out.m[i][i] = 1.0f;
    }
    return out;
}

mat4_t mat4_translation(const vec3_t d) {
    mat4_t out = mat4_identity();
    for (unsigned i=0; i < 3; ++i) {
        out.m[3][i] = -d.v[i];
    }
    return out;
}

mat4_t mat4_scaling(const float s) {
    mat4_t out = mat4_identity();
    for (unsigned i=0; i < 3; ++i) {
        out.m[i][i] = s;
    }
    return out;
}

mat4_t mat4_mul(const mat4_t a, const mat4_t b) {
    mat4_t out;
    for (unsigned i=0; i < 4; ++i) {
       for (unsigned j=0; j < 4; ++j) {
           out.m[i][j] = 0.0f;
           for (unsigned k=0; k < 4; ++k) {
               out.m[i][j] += a.m[i][k] * b.m[k][j];
           }
       }
   }
    return out;
}

vec3_t mat4_apply(const mat4_t m, const vec3_t v) {
    float w[4] = {v.v[0], v.v[1], v.v[2], 1.0f};
    float o[4] = {0.0f};
    for (unsigned i=0; i < 4; ++i) {
        for (unsigned j=0; j < 4; ++j) {
            o[i] += m.m[j][i] * w[j];
        }
    }
    vec3_t out;
    for (unsigned k=0; k < 3; ++k) {
        out.v[k] = o[k] / o[3];
    }
    return out;
}

mat4_t mat4_inv(const mat4_t in) {
   const float a00 = in.m[0][0];
   const float a01 = in.m[0][1];
   const float a02 = in.m[0][2];
   const float a03 = in.m[0][3];
   const float a10 = in.m[1][0];
   const float a11 = in.m[1][1];
   const float a12 = in.m[1][2];
   const float a13 = in.m[1][3];
   const float a20 = in.m[2][0];
   const float a21 = in.m[2][1];
   const float a22 = in.m[2][2];
   const float a23 = in.m[2][3];
   const float a30 = in.m[3][0];
   const float a31 = in.m[3][1];
   const float a32 = in.m[3][2];
   const float a33 = in.m[3][3];

   const float det = a00*a11*a22*a33 + a00*a12*a23*a31 + a00*a13*a21*a32
                   + a01*a10*a23*a32 + a01*a12*a20*a33 + a01*a13*a22*a30
                   + a02*a10*a21*a33 + a02*a11*a23*a30 + a02*a13*a20*a31
                   + a03*a10*a22*a31 + a03*a11*a20*a32 + a03*a12*a21*a30
                   - a00*a11*a23*a32 - a00*a12*a21*a33 - a00*a13*a22*a31
                   - a01*a10*a22*a33 - a01*a12*a23*a30 - a01*a13*a20*a32
                   - a02*a10*a23*a31 - a02*a11*a20*a33 - a02*a13*a21*a30
                   - a03*a10*a21*a32 - a03*a11*a22*a30 - a03*a12*a20*a31;

   if (det == 0.0f) {
       log_warn("Tried to invert noninvertible matrix");
       return mat4_identity();
   }

   mat4_t out;
   out.m[0][0] = a11*a22*a33 + a12*a23*a31 + a13*a21*a32
               - a11*a23*a32 - a12*a21*a33 - a13*a22*a31;
   out.m[0][1] = a01*a23*a32 + a02*a21*a33 + a03*a22*a31
               - a01*a22*a33 - a02*a23*a31 - a03*a21*a32;
   out.m[0][2] = a01*a12*a33 + a02*a13*a31 + a03*a11*a32
               - a01*a13*a32 - a02*a11*a33 - a03*a12*a31;
   out.m[0][3] = a01*a13*a22 + a02*a11*a23 + a03*a12*a21
               - a01*a12*a23 - a02*a13*a21 - a03*a11*a22;
   out.m[1][0] = a10*a23*a32 + a12*a20*a33 + a13*a22*a30
               - a10*a22*a33 - a12*a23*a30 - a13*a20*a32;
   out.m[1][1] = a00*a22*a33 + a02*a23*a30 + a03*a20*a32
               - a00*a23*a32 - a02*a20*a33 - a03*a22*a30;
   out.m[1][2] = a00*a13*a32 + a02*a10*a33 + a03*a12*a30
               - a00*a12*a33 - a02*a13*a30 - a03*a10*a32;
   out.m[1][3] = a00*a12*a23 + a02*a13*a20 + a03*a10*a22
               - a00*a13*a22 - a02*a10*a23 - a03*a12*a20;
   out.m[2][0] = a10*a21*a33 + a11*a23*a30 + a13*a20*a31
               - a10*a23*a31 - a11*a20*a33 - a13*a21*a30;
   out.m[2][1] = a00*a23*a31 + a01*a20*a33 + a03*a21*a30
               - a00*a21*a33 - a01*a23*a30 - a03*a20*a31;
   out.m[2][2] = a00*a11*a33 + a01*a13*a30 + a03*a10*a31
               - a00*a13*a31 - a01*a10*a33 - a03*a11*a30;
   out.m[2][3] = a00*a13*a21 + a01*a10*a23 + a03*a11*a20
               - a00*a11*a23 - a01*a13*a20 - a03*a10*a21;
   out.m[3][0] = a10*a22*a31 + a11*a20*a32 + a12*a21*a30
               - a10*a21*a32 - a11*a22*a30 - a12*a20*a31;
   out.m[3][1] = a00*a21*a32 + a01*a22*a30 + a02*a20*a31
               - a00*a22*a31 - a01*a20*a32 - a02*a21*a30;
   out.m[3][2] = a00*a12*a31 + a01*a10*a32 + a02*a11*a30
               - a00*a11*a32 - a01*a12*a30 - a02*a10*a31;
   out.m[3][3] = a00*a11*a22 + a01*a12*a20 + a02*a10*a21
               - a00*a12*a21 - a01*a10*a22 - a02*a11*a20;

   for (unsigned i=0; i < 4; ++i) {
       for (unsigned j=0; j < 4; ++j) {
           out.m[i][j] /= det;
       }
   }
   return out;
}

float vec3_length(const vec3_t v) {
    float length = 0.0f;
    for (unsigned i=0; i < 3; ++i) {
        length += v.v[i] * v.v[i];
    }
    return sqrtf(length);
}

vec3_t vec3_normalized(const vec3_t v) {
    const float length = vec3_length(v);
    vec3_t out;
    for (unsigned i=0; i < 3; ++i) {
        out.v[i] = v.v[i] / length;
    }
    return out;
}

vec3_t vec3_cross(const vec3_t a, const vec3_t b) {
    return (vec3_t){{
        a.v[1] * b.v[2] - a.v[2] * b.v[1],
        a.v[2] * b.v[0] - a.v[0] * b.v[2],
        a.v[0] * b.v[1] - a.v[1] * b.v[0]}};
}

vec3_t vec3_center(const vec3_t a, const vec3_t b) {
    vec3_t out;
    for (unsigned i=0; i < 3; ++i) {
        out.v[i] = (a.v[i] + b.v[i]) / 2.0f;
    }
    return out;
}
