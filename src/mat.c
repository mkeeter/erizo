#include "platform.h"

#include "mat.h"
#include "log.h"

void mat4_identity(float out[4][4]) {
    memset(out, 0, sizeof(float) * 4 * 4);
    for (unsigned i=0; i < 4; ++i) {
        out[i][i] = 1.0f;
    }
}

void mat4_translation(const float d[3], float out[4][4]) {
    mat4_identity(out);
    for (unsigned i=0; i < 3; ++i) {
        out[3][i] = -d[i];
    }
}

void mat4_scaling(const float s, float out[4][4]) {
    mat4_identity(out);
    for (unsigned i=0; i < 3; ++i) {
        out[i][i] = s;
    }
}

void mat4_mul(const float a_[4][4], const float b_[4][4], float out[4][4]) {
    /*  Copy to local matrices, in case out[][] overlaps a_ or b_ */
    float a[4][4];
    memcpy(a, a_, sizeof(a));
    float b[4][4];
    memcpy(b, b_, sizeof(a));

    for (unsigned i=0; i < 4; ++i) {
       for (unsigned j=0; j < 4; ++j) {
           out[i][j] = 0.0f;
           for (unsigned k=0; k < 4; ++k) {
               out[i][j] += a[i][k] * b[k][j];
           }
       }
   }
}

void mat4_apply(const float m_[4][4], const float v[3], float out[3]) {
    float m[4][4];
    memcpy(m, m_, sizeof(m));

    float w[4] = {v[0], v[1], v[2], 1.0f};
    float o[4];
    for (unsigned i=0; i < 4; ++i) {
        o[i] = 0.0f;
        for (unsigned j=0; j < 4; ++j) {
            o[i] += m[j][i] * w[j];
        }
    }
    for (unsigned k=0; k < 3; ++k) {
        out[k] = o[k] / o[3];
    }
}

void mat4_inv(const float in[4][4], float out[4][4]) {
   const float a00 = in[0][0];
   const float a01 = in[0][1];
   const float a02 = in[0][2];
   const float a03 = in[0][3];
   const float a10 = in[1][0];
   const float a11 = in[1][1];
   const float a12 = in[1][2];
   const float a13 = in[1][3];
   const float a20 = in[2][0];
   const float a21 = in[2][1];
   const float a22 = in[2][2];
   const float a23 = in[2][3];
   const float a30 = in[3][0];
   const float a31 = in[3][1];
   const float a32 = in[3][2];
   const float a33 = in[3][3];

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
       return;
   }

   out[0][0] = a11*a22*a33 + a12*a23*a31 + a13*a21*a32
             - a11*a23*a32 - a12*a21*a33 - a13*a22*a31;
   out[0][1] = a01*a23*a32 + a02*a21*a33 + a03*a22*a31
             - a01*a22*a33 - a02*a23*a31 - a03*a21*a32;
   out[0][2] = a01*a12*a33 + a02*a13*a31 + a03*a11*a32
             - a01*a13*a32 - a02*a11*a33 - a03*a12*a31;
   out[0][3] = a01*a13*a22 + a02*a11*a23 + a03*a12*a21
             - a01*a12*a23 - a02*a13*a21 - a03*a11*a22;
   out[1][0] = a10*a23*a32 + a12*a20*a33 + a13*a22*a30
             - a10*a22*a33 - a12*a23*a30 - a13*a20*a32;
   out[1][1] = a00*a22*a33 + a02*a23*a30 + a03*a20*a32
             - a00*a23*a32 - a02*a20*a33 - a03*a22*a30;
   out[1][2] = a00*a13*a32 + a02*a10*a33 + a03*a12*a30
             - a00*a12*a33 - a02*a13*a30 - a03*a10*a32;
   out[1][3] = a00*a12*a23 + a02*a13*a20 + a03*a10*a22
             - a00*a13*a22 - a02*a10*a23 - a03*a12*a20;
   out[2][0] = a10*a21*a33 + a11*a23*a30 + a13*a20*a31
             - a10*a23*a31 - a11*a20*a33 - a13*a21*a30;
   out[2][1] = a00*a23*a31 + a01*a20*a33 + a03*a21*a30
             - a00*a21*a33 - a01*a23*a30 - a03*a20*a31;
   out[2][2] = a00*a11*a33 + a01*a13*a30 + a03*a10*a31
             - a00*a13*a31 - a01*a10*a33 - a03*a11*a30;
   out[2][3] = a00*a13*a21 + a01*a10*a23 + a03*a11*a20
             - a00*a11*a23 - a01*a13*a20 - a03*a10*a21;
   out[3][0] = a10*a22*a31 + a11*a20*a32 + a12*a21*a30
             - a10*a21*a32 - a11*a22*a30 - a12*a20*a31;
   out[3][1] = a00*a21*a32 + a01*a22*a30 + a02*a20*a31
             - a00*a22*a31 - a01*a20*a32 - a02*a21*a30;
   out[3][2] = a00*a12*a31 + a01*a10*a32 + a02*a11*a30
             - a00*a11*a32 - a01*a12*a30 - a02*a10*a31;
   out[3][3] = a00*a11*a22 + a01*a12*a20 + a02*a10*a21
             - a00*a12*a21 - a01*a10*a22 - a02*a11*a20;

   for (unsigned i=0; i < 4; ++i) {
       for (unsigned j=0; j < 4; ++j) {
           out[i][j] /= det;
       }
   }
}

float vec3_length(const float v[3]) {
    float length = 0.0f;
    for (unsigned i=0; i < 3; ++i) {
        length += v[i] * v[i];
    }
    return sqrtf(length);
}

void vec3_normalize(float v[3]) {
    const float length = vec3_length(v);
    for (unsigned i=0; i < 3; ++i) {
        v[i] /= length;
    }
}

void vec3_cross(const float a_[3], const float b_[3], float out[3]) {
    float a[3];
    memcpy(a, a_, sizeof(a));
    float b[3];
    memcpy(b, b_, sizeof(b));

    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}
