extern crate nalgebra_glm as glm;

use std::ffi::CString;
use std::env;
use std::mem;
use std::ptr;
use std::str;

use gl::types::*;
use glutin::dpi::*;
use glutin::GlContext;
use failure::Error;

mod stl;
mod mesh;
mod loader;
mod key;
use crate::loader::load_from_file;
use crate::stl::{Stl, StlView};
use crate::key::{Keyed};

////////////////////////////////////////////////////////////////////////////////

macro_rules! gl_check {
    ($item:ident, $status:ident, $get_iv:ident, $get_log:ident) => {

        let mut status = gl::FALSE as GLint;
        gl::$get_iv($item, gl::$status, &mut status);

        if status != (gl::TRUE as GLint) {
            let mut len = 0;
            gl::$get_iv($item, gl::INFO_LOG_LENGTH, &mut len);

            let mut buf = vec![0; len as usize];
            gl::$get_log($item, len, ptr::null_mut(),
                         buf.as_mut_ptr() as *mut GLchar);

            panic!("{}", str::from_utf8(&buf)
                .expect("Log info not valid utf8"));
        }
    }
}

fn compile_shader(src: &[u8], ty: GLenum) -> GLuint {
    let c_str = CString::new(src).unwrap();
    unsafe {
        let shader = gl::CreateShader(ty);
        gl::ShaderSource(shader, 1, &c_str.as_ptr(), ptr::null());
        gl::CompileShader(shader);
        gl_check!(shader, COMPILE_STATUS, GetShaderiv, GetShaderInfoLog);
        shader
    }
}

fn link_program(vs: GLuint, gs: GLuint, fs: GLuint) -> GLuint {
    unsafe {
        let program = gl::CreateProgram();
        gl::AttachShader(program, vs);
        gl::AttachShader(program, gs);
        gl::AttachShader(program, fs);
        gl::LinkProgram(program);
        gl_check!(program, LINK_STATUS, GetProgramiv, GetProgramInfoLog);
        program
    }
}

////////////////////////////////////////////////////////////////////////////////

fn slow_path() {
    /*
    // Create GLSL shaders
    let vs = compile_shader(include_bytes!("../gl/model.vs"),
                            gl::VERTEX_SHADER);
    let gs = compile_shader(include_bytes!("../gl/indexed.gs"),
                            gl::GEOMETRY_SHADER);
    let fs = compile_shader(include_bytes!("../gl/model.fs"),
                            gl::FRAGMENT_SHADER);
    let program = link_program(vs, gs, fs);

    let proj_loc = {
        let c_str = CString::new("proj")?;
        unsafe { gl::GetUniformLocation(program, c_str.as_ptr()) }
    };
    assert!(proj_loc != -1);

    let model_loc = {
        let c_str = CString::new("model")?;
        unsafe { gl::GetUniformLocation(program, c_str.as_ptr()) }
    };
    assert!(model_loc != -1);

    let mut vao = 0;
    let mut vert_vbo = 0;
    let mut tri_vbo = 0;

    unsafe {
        // Create Vertex Array Object
        gl::GenVertexArrays(1, &mut vao);
        gl::BindVertexArray(vao);

        // Create a VBO for the mesh vertices
        gl::GenBuffers(1, &mut vert_vbo);
        gl::BindBuffer(gl::ARRAY_BUFFER, vert_vbo);
        gl::BufferData(
            gl::ARRAY_BUFFER,
            (mesh.vertices.len() * mem::size_of::<f32>()) as GLsizeiptr,
            mem::transmute(&mesh.vertices[0]),
            gl::STATIC_DRAW,
        );

        // Then, create a VBO for the triangle indices
        gl::GenBuffers(1, &mut tri_vbo);
        gl::BindBuffer(gl::ELEMENT_ARRAY_BUFFER, tri_vbo);
        gl::BufferData(
            gl::ELEMENT_ARRAY_BUFFER,
            (mesh.triangles.len() * mem::size_of::<u32>()) as GLsizeiptr,
            mem::transmute(&mesh.triangles[0]),
            gl::STATIC_DRAW,
        );

        // Specify the layout of the vertex data
        gl::EnableVertexAttribArray(0);
        gl::VertexAttribPointer(
            0, /* Vertex attribute location */
            3, /* Number of elements */
            gl::FLOAT,
            gl::FALSE as GLboolean,
            0, /* Offset */
            ptr::null(),
        );
    }
    */
}

struct Shader {
    vs: GLuint,
    gs: GLuint,
    fs: GLuint,
    prog: GLuint,

    proj_loc: i32,
    model_loc: i32,
}

impl Shader {
    fn fast() -> Result<Shader, Error> {
        // Create GLSL shaders
        let vs = compile_shader(include_bytes!("../gl/raw.vs"),
                                gl::VERTEX_SHADER);
        let gs = compile_shader(include_bytes!("../gl/raw.gs"),
                                gl::GEOMETRY_SHADER);
        let fs = compile_shader(include_bytes!("../gl/model.fs"),
                                gl::FRAGMENT_SHADER);
        let program = link_program(vs, gs, fs);

        let proj_loc = {
            let c_str = CString::new("proj")?;
            unsafe { gl::GetUniformLocation(program, c_str.as_ptr()) }
        };
        assert!(proj_loc != -1);

        let model_loc = {
            let c_str = CString::new("model")?;
            unsafe { gl::GetUniformLocation(program, c_str.as_ptr()) }
        };
        assert!(model_loc != -1);

        Ok(Shader {
            vs: vs,
            gs: gs,
            fs: fs,
            prog: program,
            proj_loc: proj_loc,
            model_loc: model_loc,
        })
    }

    fn bind(&self, proj: &glm::Mat4, model: &glm::Mat4) {
        unsafe {
            gl::UseProgram(self.prog);
            gl::UniformMatrix4fv(self.proj_loc, 1, gl::FALSE, proj.as_ptr());
            gl::UniformMatrix4fv(self.model_loc, 1, gl::FALSE, model.as_ptr());
        }
    }
}

struct RawMesh {
    vbo: GLuint,
    vao: GLuint,
    num_points: u32,
}

impl RawMesh {
    fn new(stl: &StlView) -> Result<RawMesh, Error> {
        let mut out = RawMesh {
            vbo: 0,
            vao: 0,
            num_points: stl.len() / 3 / 3,
        };
        println!("{:?}", stl.0.as_ptr());
        println!("len: {}", stl.len());
        println!("buffer len {}", stl.0.len());

        unsafe {
            // Create Vertex Array Object
            gl::GenVertexArrays(1, &mut out.vao);
            gl::BindVertexArray(out.vao);

            // Create a VBO for the mesh vertices
            gl::GenBuffers(1, &mut out.vbo);
            gl::BindBuffer(gl::ARRAY_BUFFER, out.vbo);
            gl::BufferData(
                gl::ARRAY_BUFFER,
                (stl.len() * 3 * mem::size_of::<f32>() as u32) as GLsizeiptr,
                mem::transmute(stl.key(0).0),
                gl::STATIC_DRAW,
            );

            // Specify the layout of the vertex data
            gl::EnableVertexAttribArray(0);
            gl::VertexAttribPointer(
                0, /* Vertex attribute location */
                3, /* Number of elements */
                gl::FLOAT,
                gl::FALSE as GLboolean,
                0, /* Offset */
                ptr::null(),
            );

            gl::EnableVertexAttribArray(1);
            gl::VertexAttribPointer(
                1, /* Vertex attribute location */
                3, /* Number of elements */
                gl::FLOAT,
                gl::FALSE as GLboolean,
                50, /* stride */
                (3 * mem::size_of::<f32>()) as *const std::ffi::c_void,
            );

            gl::EnableVertexAttribArray(2);
            gl::VertexAttribPointer(
                1, /* Vertex attribute location */
                3, /* Number of elements */
                gl::FLOAT,
                gl::FALSE as GLboolean,
                50, /* stride */
                (6 * mem::size_of::<f32>()) as *const std::ffi::c_void,
            );
        }

        Ok(out)
    }

    fn draw(&self) {
        unsafe {
            gl::DrawArrays(gl::POINTS, 0, self.num_points as i32);
        }
    }
}

fn main() -> Result<(), Error> {
    let now = std::time::Instant::now();
    let args = env::args().collect::<Vec<_>>();
    let stl = Stl::open(&args[1])?;

    let mut events_loop = glutin::EventsLoop::new();
    let window = glutin::WindowBuilder::new()
        .with_title("Hello, world!")
        .with_dimensions(LogicalSize::new(1024.0, 768.0));
    let context = glutin::ContextBuilder::new()
        .with_vsync(true);
    let gl_window = glutin::GlWindow::new(window, context, &events_loop).unwrap();

    unsafe {
        gl_window.make_current().unwrap();
    }

    gl::load_with(|symbol| gl_window.get_proc_address(symbol) as *const _);

    let mesh = RawMesh::new(&stl.view())?;
    let model_matrix = {
        let (lower, upper) = stl.bounds();
        let center = (lower + upper) / 2.0;
        let scale = 2.0 / glm::comp_max(&(upper - lower));
        let mut mat = glm::Mat4::identity();
        mat = glm::scale(&mat, &glm::vec3(scale, scale, scale));
        mat = glm::translate(&mat, &center);
        println!("{:?}, {:?}, {:?}", lower, upper, mat);
        mat
    };

    let shader = Shader::fast()?;

    let mut running = true;
    let mut first = true;
    while running {
        events_loop.poll_events(|event| {
            match event {
                glutin::Event::WindowEvent{ event, .. } => match event {
                    glutin::WindowEvent::CloseRequested => running = false,
                    glutin::WindowEvent::Resized(logical_size) => {
                        let dpi_factor = gl_window.get_hidpi_factor();
                        gl_window.resize(logical_size.to_physical(dpi_factor));
                    },
                    _ => ()
                },
                _ => ()
            }
        });

        unsafe {
            // Clear the screen to black
            gl::ClearColor(0.3, 0.3, 0.3, 1.0);
            gl::ClearDepth(1.0);
            gl::Clear(gl::COLOR_BUFFER_BIT | gl::DEPTH_BUFFER_BIT);
            gl::Enable(gl::DEPTH_TEST);

            let proj_matrix = glm::scale(&glm::Mat4::identity(), &glm::vec3(0.5, 0.7, 1.0));
            shader.bind(&proj_matrix, &model_matrix);
            mesh.draw();
        }

        gl_window.swap_buffers().unwrap();
        if first {
            first = false;
            println!("First draw in {:?}", now.elapsed());
        }
    }

    Ok(())
}
