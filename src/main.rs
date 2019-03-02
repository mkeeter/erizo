#[macro_use] extern crate nom;
use nom::types::CompleteByteSlice;
use nom::{le_f32, le_u16, le_u32};

use failure::{Error, err_msg};

use std::str::FromStr;

use memmap::MmapOptions;
use std::fs::File;

named!(parse_vec<&[u8], [f32; 3]>,
    do_parse!(
        x: le_f32 >>
        y: le_f32 >>
        z: le_f32 >>
        ([x, y, z])));
named!(parse_line<&[u8], ([f32; 3], [f32;3], [f32;3])>,
    do_parse!(
        n: parse_vec >>
        a: parse_vec >>
        b: parse_vec >>
        c: parse_vec >>
        attr: le_u16 >>
        (a, b, c)));
named!(parse_header<&[u8], (&[u8], u32)>,
    do_parse!(
        header: take!(80) >>
        count:  le_u32 >>
        (header, count)));

fn main() -> Result<(), Error> {
    let file = File::open("/Users/mkeeter/Models/porsche.stl")?;
    let mmap = unsafe { MmapOptions::new().map(&file)? };
    println!("Loading stl");

    let ookay = parse_header(&mmap).map_err(|s| err_msg(format!("{:?}", s)))?;
    println!("{:?}", (ookay.1).1);

    Ok(())
}
