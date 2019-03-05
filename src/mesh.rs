use std::f32::INFINITY;

use rayon::prelude::*;

pub struct IndexedMesh {
    pub vertices: Vec<f32>,
    pub triangles: Vec<u32>,
    pub lower: glm::Vec3,
    pub upper: glm::Vec3,
}

impl IndexedMesh {
    pub fn new(vertices: Vec<f32>, triangles: Vec<u32>) -> Self {

        /* Use map + reduce to find the bounds of the model */
        let id_fn = || (glm::vec3( INFINITY,  INFINITY,  INFINITY),
                        glm::vec3(-INFINITY, -INFINITY, -INFINITY));
        let bounds = vertices.par_chunks(3)
            .map(|v| glm::vec3(v[0], v[1], v[2]))
            .fold(  id_fn, |a, v| (glm::min2(&a.0, &v),
                                   glm::max2(&a.1, &v)))
            .reduce(id_fn, |a, b| (glm::min2(&a.0, &b.0),
                                   glm::max2(&a.1, &b.1)));

        IndexedMesh {
            vertices: vertices,
            triangles: triangles,
            lower: bounds.0,
            upper: bounds.1,
        }
    }
}
