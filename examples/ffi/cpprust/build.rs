fn main()
{
    cxx_build::bridge("src/lib.rs")
        .file("src/animal.hpp")
        .compile("cpp_from_rust");
    
    println!("cargo:rerun-if-changed=src/lib.rs");
    println!("cargo:rerun-if-changed=src/animal.hpp");
}