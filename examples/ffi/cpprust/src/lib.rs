#[macro_use]
extern crate log;
extern crate env_logger;
use cxx::{CxxString, CxxVector};

#[cxx::bridge]
mod ffi {
    struct Animal {
        value: i32,
    }

    struct Person {
        name: String,
        age: i32,
    }

    extern "Rust" {
        fn log_string_from_cpp_to_rust_log_crate(message: &CxxString);
        fn log_int_from_cpp_to_rust_log_crate(level: i32);
        fn log_vector_from_cpp_to_rust_log_crate(attributes: &CxxVector<CxxString>);
        fn log_struct_from_cpp_to_rust_log_crate(person: &Person);
        fn log_class_from_cpp_to_rust_log_crate(animal: &Animal);
        fn init_rust_logger() -> ();
    }

    unsafe extern "C++" {
        include!("cpprust/src/animal.hpp");
        type Animal;
        fn get_age(&self) -> i32;
    }
}

pub fn log_string_from_cpp_to_rust_log_crate(message: &CxxString) {
    info!("{}", message);
}

pub fn log_int_from_cpp_to_rust_log_crate(level: i32) {
    info!("{}", level);
}

pub fn log_vector_from_cpp_to_rust_log_crate(attributes: &CxxVector<CxxString>) {
    info!("{:?}", attributes);
}

pub fn log_struct_from_cpp_to_rust_log_crate(person: &ffi::Person) {
    info!("Received persons: {} who is {} years old", person.name, person.age);
}

pub fn log_class_from_cpp_to_rust_log_crate(animal: &ffi::Animal) {
    let value = animal.get_age();
    info!("{}", value);
}

pub fn init_rust_logger() -> () {
    env_logger::init();
}