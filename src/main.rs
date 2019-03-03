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

#[derive(Copy, Clone)]
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

#[derive(Copy, Clone)]
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

unsafe impl std::marker::Send for StlKey { }

////////////////////////////////////////////////////////////////////////////////

struct Chunk<'a> {
    set: FnvHashSet<StlKey>,
    vs: Vec<&'a mut [u32]>,
}

impl<'a> Chunk<'a> {
    fn empty() -> Self {
        Self {
            set: Default::default(),
            vs: vec![],
        }
    }

    fn is_empty(&self) -> bool {
        self.set.is_empty()
    }

    /*
     *  range is in terms of global vertex indices
     *  vertices is a local slice of the mutable vertices array
     */
    fn build(range: std::ops::Range<u32>,
             vertices: &'a mut [u32],
             stl: &RawStl) -> Self
    {
        assert!(range.len() == vertices.len());
        let mut out = Self::empty();
        for (i, v) in range.into_iter().zip(vertices.iter_mut()) {
            let key = stl.key(i);
            *v = if let Some(ptr) = out.set.get(&key) {
                stl.index(ptr)
            } else {
                out.set.insert(key);
                i
            };
        }
        out.vs.push(vertices);
        out
    }

    fn merge(mut self, mut other: Self, stl: &RawStl) -> Self {
        // Special-casing empty objects
        if self.is_empty() {
            other
        } else if other.is_empty() {
            self
        } else {
            // We build an array to quickly check whether to remap
            let mut min = u32::max_value();
            let mut remap = {
                let mut max = u32::min_value();
                for k in other.set.iter() {
                    let i = stl.index(k);
                    if i < min { min = i; }
                    if i > max { max = i; }
                }
                vec![0; (max - min) as usize + 1]
            };

            for k in other.set.drain() {
                if !self.set.insert(k) {
                    let v = self.set.get(&k).unwrap();
                    remap[(stl.index(&k) - min) as usize] = stl.index(v) + 1;
                }
            }

            // Steal all of the other chunk's remapped vertices
            for vs in other.vs.drain(..) {
                for v in vs.iter_mut() {
                    let w = remap[(*v - min) as usize];
                    if w != 0 {
                        *v = w - 1;
                    }
                }
                self.vs.push(vs);
            }
            self
        }
    }
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

    // We split into one chunk per thread
    let n = num_cpus::get();
    let chunk_size = (vertex_count as usize + n - 1) / n;

    // It's cleaner to do this as a fold + reduce, but that's a lot
    // slower, because it creates many intermediate structures which
    // have be merged (instead of one per physical thread).
    let out = (0..n).into_par_iter()
        .zip(vertices.as_mut_slice()
                     .par_chunks_mut(chunk_size))
        .map(|(i, vs)| {
            let start = (i * chunk_size) as u32;
            let end = start + vs.len() as u32;
            Chunk::build(start..end, vs, &stl)
        })
        .reduce(|| Chunk::empty(), |a, b| a.merge(b, &stl));
    println!("Unique vertices: {}", out.set.len());

    // This is memory-inefficient but fast.
    let mut remap = vec![0; vertex_count as usize];
    for (u, v) in out.set.iter().enumerate() {
        remap[stl.index(v) as usize] = u;
    }

    Ok(())
}
