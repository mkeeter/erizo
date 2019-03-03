extern crate nalgebra as na;

use failure::{Error, err_msg};
use fnv::{FnvHashSet, FnvHashMap};
use nom::types::CompleteByteSlice;
use nom::{le_f32, le_u32, named, do_parse, call, take};
use memmap::MmapOptions;
use rayon::prelude::*;

use std::collections::HashSet;
use std::cmp::Ordering;
use std::fs::File;
use std::hash::{Hash, Hasher};
use std::mem::size_of;

type Vec3f = na::Vector3<f32>;
type Vec3i = na::Vector3<u32>;

named!(parse_vec<&[u8], Vec3f>,
    do_parse!(
        x: le_f32 >>
        y: le_f32 >>
        z: le_f32 >>
        (Vec3f::new(x, y, z))));
/*
named!(parse_line<&[u8], ([u8; 12], [u8; 12], [u8; 12])>,
    do_parse!(
        _normal: take!(12) >>
        a:       take!(12)  >>
        b:       take!(12) >>
        c:       take!(12) >>
        _attrs:  take!(size_of::<u16>()) >>
        (*a, *b, *c)));
        */
named!(parse_header<&[u8], (&[u8], u32)>,
    do_parse!(
        header: take!(80) >>
        count:  le_u32 >>
        (header, count)));

////////////////////////////////////////////////////////////////////////////////

struct RawStl<'a>(&'a [u8]);
impl<'a> RawStl<'a> {
    const HEADER_SIZE: usize = 80;
    const VEC_SIZE: usize = 3 * size_of::<f32>();
    const TRIANGLE_SIZE: usize = 4 * Self::VEC_SIZE + size_of::<u16>();

    fn key(&self, v: u32) -> StlKey {
        let triangle = (v / 3) as usize;
        let vertex = (v % 3) as usize;
        let i =
            // Header
            Self::HEADER_SIZE
            // Number of triangles
            + size_of::<u32>()
            // Skipped triangles
            + triangle * Self::TRIANGLE_SIZE
            // Normal
            + Self::VEC_SIZE
            // Skipped vertices
            + vertex * 3 * size_of::<f32>();
        StlKey(self.0[i..].as_ptr())
    }

    fn index(&self, k: &StlKey) -> u32 {
        // Find the pointer offset in bytes
        let mut diff = k.0 as usize - self.0.as_ptr() as usize;

        // Skip the header + triangle count
        diff -= Self::HEADER_SIZE + size_of::<u32>();

        // Pick out which triangle this vertex lives in
        let tri = diff / Self::TRIANGLE_SIZE;
        diff -= tri * Self::TRIANGLE_SIZE;

        // skip the normal
        diff -= Self::VEC_SIZE;

        let vert = diff / Self::VEC_SIZE;
        assert!(vert < 3);

        (tri * 3 + vert) as u32
    }

    fn triangle_count(&self) -> u32 {
        le_u32(&self.0[Self::HEADER_SIZE..]).unwrap().1
    }
}

struct StlKey(*const u8);

impl StlKey {
    fn get_slice(&self) -> &[u8] {
        unsafe { std::slice::from_raw_parts(self.0, 12) }
    }
}

impl PartialEq for StlKey {
    fn eq(&self, other: &StlKey) -> bool {
        self.get_slice() == other.get_slice()
    }
}
impl Eq for StlKey {}

impl Hash for StlKey {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.get_slice().hash(state);
    }
}

////////////////////////////////////////////////////////////////////////////////

/*
 *  Runs on a particular set of triangles, writing them into the
 *  given array and returning the vertex hashset.
 *
 *  range is in terms of global vertex indices
 *  vertices is a local slice of the mutable vertices array
 */
fn run(stl: &RawStl, vertices: &mut [u32],
       range: std::ops::Range<u32>) -> FnvHashSet<StlKey>
{
    let mut set: FnvHashSet<StlKey> = FnvHashSet::default();
    for (i, v) in range.zip(vertices.iter_mut()) {
        let key = stl.key(i);
        *v = if let Some(ptr) = set.get(&key) {
            stl.index(ptr)
        } else {
            set.insert(key);
            i
        };
    }
    set
}

fn main() -> Result<(), Error> {
    let file = File::open("/Users/mkeeter/Models/porsche.stl")?;
    //let file = File::open("cube.stl")?;
    let mmap = unsafe { MmapOptions::new().map(&file)? };
    println!("Loading stl");

    let stl = RawStl(&mmap);
    let triangle_count = stl.triangle_count();
    println!("Triangle count: {}", triangle_count);

    let vertex_count = triangle_count * 3;

    // This is our buffer of raw vertex indices
    let mut vertices = Vec::new();
    vertices.resize(vertex_count as usize, 0);

    let set = run(&stl, vertices.as_mut_slice(), 0..vertex_count);
    println!("total vertex count: {}", set.len());
    Ok(())
}
