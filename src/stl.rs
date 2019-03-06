use std::fs::File;
use std::mem::size_of;

use failure::Error;
use memmap::{Mmap, MmapOptions};
use nom::le_u32;

use crate::key::{VertexKey, Keyed};

////////////////////////////////////////////////////////////////////////////////

pub struct Stl(pub Mmap);

#[derive(Copy, Clone)]
pub struct StlView<'a>(pub &'a [u8]);

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
}

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
