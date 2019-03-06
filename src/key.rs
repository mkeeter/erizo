use std::hash::{Hash, Hasher};

#[derive(Copy, Clone)]
pub struct VertexKey(pub *const u8);

impl VertexKey {
    pub fn get_slice(&self) -> &[u8] {
        unsafe { std::slice::from_raw_parts(self.0, 12) }
    }
}

impl PartialEq for VertexKey {
    fn eq(&self, other: &VertexKey) -> bool {
        self.get_slice() == other.get_slice()
    }
}
impl Eq for VertexKey {}

impl Hash for VertexKey {
    fn hash<H: Hasher>(&self, state: &mut H) {
        self.get_slice().hash(state);
    }
}

unsafe impl std::marker::Send for VertexKey { }

pub trait Keyed {
    /// Returns the key for a particular vertex
    fn key(&self, v: u32) -> VertexKey;

    /// Returns the index of a particular vertex
    fn index(&self, k: &VertexKey) -> u32;

    /// Returns the number of vertices in this buffer
    fn len(&self) -> u32;
}
