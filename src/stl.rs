use std::fs::File;
use std::mem::size_of;
use std::f32::INFINITY;

use failure::Error;
use memmap::{Mmap, MmapOptions};
use nom::le_u32;
use rayon::prelude::*;

use crate::key::{VertexKey, Keyed, parse_vec};

////////////////////////////////////////////////////////////////////////////////

pub struct Stl(pub Mmap);

impl Stl {
    pub fn open(filename: &str) -> Result<Stl, Error> {
        let file = File::open(filename)?;
        let mmap = unsafe { MmapOptions::new().map(&file)? };
        Ok(Stl(mmap))
    }

    const HEADER_SIZE: usize = 80;
    const VEC_SIZE: usize = 3 * size_of::<f32>();
    const TRIANGLE_SIZE: usize = 4 * Self::VEC_SIZE + size_of::<u16>();

    pub fn view(&self) -> StlView {
        StlView(&self.0)
    }

    pub fn bounds(&self) -> (glm::Vec3, glm::Vec3) {
        /* Use map + reduce to find the bounds of the model */
        let id_fn = || (glm::vec3( INFINITY,  INFINITY,  INFINITY),
                        glm::vec3(-INFINITY, -INFINITY, -INFINITY));
        let v = self.view();
        (0..v.len()).into_par_iter()
            .map(|i| parse_vec(v.key(i).get_slice()).unwrap().1)
            .map(|p| glm::vec3(p[0], p[1], p[2]))
            .fold(  id_fn, |a, p| (glm::min2(&a.0, &p),
                                   glm::max2(&a.1, &p)))
            .reduce(id_fn, |a, b| (glm::min2(&a.0, &b.0),
                                   glm::max2(&a.1, &b.1)))
    }
}

#[derive(Copy, Clone)]
pub struct StlView<'a>(pub &'a [u8]);

impl<'a> Keyed for StlView<'a> {
    fn key(&self, v: u32) -> VertexKey {
        let triangle = (v / 3) as usize;
        let vertex = (v % 3) as usize;
        let i =
            // Header
            Stl::HEADER_SIZE
            // Number of triangles
            + size_of::<u32>()
            // Skipped triangles
            + triangle * Stl::TRIANGLE_SIZE
            // Normal
            + Stl::VEC_SIZE
            // Skipped vertices
            + vertex * 3 * size_of::<f32>();
        VertexKey(self.0[i..].as_ptr())
    }

    fn index(&self, k: &VertexKey) -> u32 {
        // Find the pointer offset in bytes
        let mut diff = k.0 as usize - self.0.as_ptr() as usize;

        // Skip the header + triangle count
        diff -= Stl::HEADER_SIZE + size_of::<u32>();

        // Pick out which triangle this vertex lives in
        let tri = diff / Stl::TRIANGLE_SIZE;
        diff -= tri * Stl::TRIANGLE_SIZE;

        // skip the normal
        diff -= Stl::VEC_SIZE;

        let vert = diff / Stl::VEC_SIZE;
        assert!(vert < 3);

        (tri * 3 + vert) as u32
    }

    fn len(&self) -> u32 {
        le_u32(&self.0[Stl::HEADER_SIZE..]).unwrap().1 * 3
    }
}
