use failure::Error;
use fnv::FnvHashSet;
use rayon::prelude::*;

use crate::mesh::IndexedMesh;
use crate::key::{Keyed, VertexKey, parse_vec};

struct Chunk<'a> {
    set: FnvHashSet<VertexKey>,
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
             mesh: &Keyed) -> Self
    {
        assert!(range.len() == vertices.len());
        let mut out = Self::empty();
        for (i, v) in range.into_iter().zip(vertices.iter_mut()) {
            let key = mesh.key(i);
            *v = if let Some(ptr) = out.set.get(&key) {
                mesh.index(ptr)
            } else {
                out.set.insert(key);
                i
            };
        }
        out.vs.push(vertices);
        out
    }

    fn merge(mut self, mut other: Self, mesh: &Keyed) -> Self {
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
                    let i = mesh.index(k);
                    if i < min { min = i; }
                    if i > max { max = i; }
                }
                vec![0; (max - min) as usize + 1]
            };

            for k in other.set.drain() {
                if !self.set.insert(k) {
                    let v = self.set.get(&k).unwrap();
                    remap[(mesh.index(&k) - min) as usize] = mesh.index(v) + 1;
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


pub fn load_from_file<M: Keyed + std::marker::Sync>(mesh: M) -> Result<IndexedMesh, Error>
{
    let vertex_count = mesh.len();

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
            Chunk::build(start..end, vs, &mesh)
        })
        .reduce(|| Chunk::empty(), |a, b| a.merge(b, &mesh));
    println!("Unique vertices: {}", out.set.len());

    // This is a memory-inefficient but fast way to control global remapping
    let mut remap = vec![0; vertex_count as usize];

    let mut coords = Vec::new();
    coords.reserve(out.set.len() * 3);
    for (u, v) in out.set.iter().enumerate() {
        for w in parse_vec(v.get_slice()).unwrap().1.iter() {
            coords.push(*w);
        }
        remap[mesh.index(v) as usize] = u as u32;
    }

    vertices.par_iter_mut()
        .for_each(|v| *v = remap[(*v) as usize]);

    Ok(IndexedMesh::new(coords, vertices))
}
