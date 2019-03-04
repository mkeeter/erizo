use failure::Error;

mod stl;
mod mesh;
use crate::stl::load_from_file;

////////////////////////////////////////////////////////////////////////////////


fn main() -> Result<(), Error> {
    let out = load_from_file("/Users/mkeeter/Models/porsche.stl")?;
    Ok(())
}
