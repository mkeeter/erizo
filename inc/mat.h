/*  Constructs an identity matrix */
void mat4_identity(float out[4][4]);

/*  Constructs a translation matrix */
void mat4_translation(const float d[3], float out[4][4]);

/*  Constructs a uniform scaling matrix */
void mat4_scaling(const float s, float out[4][4]);

/*  Matrix math */
void mat4_mul(const float a[4][4], const float b[4][4], float out[4][4]);
void mat4_apply(const float a[4][4], const float v[3], float out[3]);
void mat4_inv(const float in[4][4], float out[4][4]);

/* Basic vector length */
float vec3_length(const float v[3]);

/* In-place normalization of a vector */
void vec3_normalize(float v[3]);
