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

struct CustomBuildHasher<'a> {
    chunk: &'a [u8],
    fnv_builder: BuildHasherDefault<FnvHasher>,
}

impl <'a> CustomBuildHasher<'a> {
    fn new(chunk: &'a [u8]) -> CustomBuildHasher<'a> {
        CustomBuildHasher {
            chunk: chunk,
            fnv_builder: BuildHasherDefault::<FnvHasher>::default(),
        }
    }
}

impl<'a> BuildHasher for CustomBuildHasher<'a> {
    type Hasher = CustomHasher<'a>;
    fn build_hasher(&self) -> Self::Hasher {
        CustomHasher {
            chunk: self.chunk,
            hasher: self.fnv_builder.build_hasher(),
        }
    }
}

struct CustomHasher<'a> {
    chunk: &'a [u8],
    hasher: FnvHasher,
}

impl<'a> Hasher for CustomHasher<'a> {
    fn write(&mut self, bytes: &[u8]) {
        unimplemented!();
    }
    fn write_u32(&mut self, index: u32) {
        let i = index as usize;
        self.hasher.write(&self.chunk[i..i + 12]);
    }
    fn finish(&self) -> u64 {
        self.hasher.finish()
    }
}

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

fn load_from_chunk<'a>(data: &'a [u8]) {
    let hasher = CustomBuildHasher::new(data);
    let mut set = HashSet::<u32, _>::with_hasher(hasher);
    set.insert(10);
    println!("{:?}", set.contains(&10));
    println!("{:?}", set.contains(&11));
}

fn main() -> Result<(), Error> {
    let file = File::open("/Users/mkeeter/Models/porsche.stl")?;
    let mmap = unsafe { MmapOptions::new().map(&file)? };
    println!("Loading stl");

    let header = parse_header(&mmap)
        .map_err(|s| err_msg(format!("{:?}", s)))?;
    println!("{:?}", (header.1).1);

    load_from_chunk(&mmap);

    Ok(())
}
