typedef struct mat4_ {
    float m[4][4];
} mat4_t;

typedef struct vec3_ {
    float v[3];
} vec3_t;

/*  Constructs an identity matrix */
mat4_t mat4_identity(void);

/*  Constructs a translation matrix */
mat4_t mat4_translation(const vec3_t d);

/*  Constructs a uniform scaling matrix */
mat4_t mat4_scaling(const float s);

/*  Matrix math */
mat4_t mat4_mul(const mat4_t a, const mat4_t b);
vec3_t mat4_apply(const mat4_t a, const vec3_t v);
mat4_t mat4_inv(const mat4_t in);

/*  Basic vector length */
float vec3_length(const vec3_t v);

/*  In-place normalization of a vector */
vec3_t vec3_normalized(const vec3_t v);

/*  Returns the center between two points */
vec3_t vec3_center(const vec3_t a, const vec3_t b);

/*  Cross product */
vec3_t vec3_cross(const vec3_t a, const vec3_t b);
