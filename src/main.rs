extern crate nalgebra as na;

use nom::types::CompleteByteSlice;
use nom::{le_f32, le_u32, named, do_parse, call, take};

use failure::{Error, err_msg};

use memmap::MmapOptions;

use std::fs::File;
use std::mem::size_of;
use std::collections::{HashMap, HashSet};
use std::hash::{BuildHasher, BuildHasherDefault, Hasher};
use fnv::FnvHasher;

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

struct StlBuildHasher<'a> {
    data: &'a [u8],
    builder: BuildHasherDefault<FnvHasher>,
}

impl <'a> StlBuildHasher<'a> {
    fn new(data: &'a [u8]) -> Self {
        StlBuildHasher {
            data: data,
            builder: BuildHasherDefault::<FnvHasher>::default(),
        }
    }
}

impl<'a> BuildHasher for StlBuildHasher<'a> {
    type Hasher = StlHasher<'a>;
    fn build_hasher(&self) -> Self::Hasher {
        StlHasher {
            data: self.data,
            hasher: self.builder.build_hasher(),
        }
    }
}

struct StlHasher<'a> {
    data: &'a [u8],
    hasher: FnvHasher,
}

impl<'a> Hasher for StlHasher<'a> {
    fn write(&mut self, bytes: &[u8]) {
        unimplemented!();
    }
    fn write_u32(&mut self, index: u32) {
        println!("Hashing {} ({:?})", index, vertex_range(index));
        println!("{:?}", parse_vec(&self.data[vertex_range(index)]));
        let i = index as usize;
        self.hasher.write(&self.data[vertex_range(index)]);
    }
    fn finish(&self) -> u64 {
        println!("finishing");
        let out = self.hasher.finish();
        println!("{}", out);
        out
    }
}

fn vertex_range(v: u32) -> std::ops::Range<usize> {
    let triangle = (v / 3) as usize;
    let vertex = (v % 3) as usize;
    let i = 80
        + size_of::<u32>()
        + triangle * (3 * 4 * size_of::<f32>() + size_of::<u16>())
        + 3 * size_of::<f32>()
        + vertex * 3 * size_of::<f32>();
    i..i + 3 * size_of::<f32>()
}

struct StlKey<'a> {
    v: u32,
    data: &'a [u8]
}

impl<'a> PartialEq for StlKey<'a> {
    fn eq(&self, other: &StlKey) -> bool {
        self.data[vertex_range(self.v)] == self.data[vertex_range(other.v)]
    }
}

impl Borrow<StlKey> for u32 {
    fn borrow(&self) -> &Id {
        &self.id
    }
}

////////////////////////////////////////////////////////////////////////////////

struct Worker<'a> {
    chunk: &'a [u8],
    verts: HashMap<[u8; 12], u32>,
    tris: &'a mut [Vec3i],
    vert_count: u32,
}

impl <'a> Worker <'a> {
    fn new(chunk: &'a [u8], tris: &'a mut [Vec3i]) -> Worker<'a> {
        Worker {
            chunk: chunk,
            verts: HashMap::new(),
            tris: tris,
            vert_count: 0,
        }
    }

    fn run(&mut self) {
        let mut i = 0;
        let mut key: [u8; 12] = [0; 12];
        let mut tri: [u32; 3] = [0; 3];
        loop {
            i += 12; // normal
            for t in 0..3 {
                key.copy_from_slice(&self.chunk[i..(i + 12)]);
                /*
                tri[t] = self.verts.entry(key)
                    .or_insert_with(|| {
                        let v = self.vert_count;
                        self.vert_count += 1;
                        v}).clone();
                        */
                i += 12; // triangle
            }
            i += 2; // array
        }
    }
}

fn main() -> Result<(), Error> {
    //let file = File::open("/Users/mkeeter/Models/porsche.stl")?;
    let file = File::open("cube.stl")?;
    let mmap = unsafe { MmapOptions::new().map(&file)? };
    println!("Loading stl");

    let header = parse_header(&mmap)
        .map_err(|s| err_msg(format!("{:?}", s)))?;
    let triangle_count = (header.1).1;
    println!("Triangle count: {}", triangle_count);

    let hasher = StlBuildHasher::new(&mmap);
    let mut set = HashSet::<u32, _>::with_hasher(hasher);
    for v in 0..triangle_count * 3 {
        let k = StlKey { v: v, data: &mmap };
        set.get(&k);
    }
    println!("total vertex count: {}", set.len());

    Ok(())
}
