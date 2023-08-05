use opentelemetry::sdk::trace::TracerProvider;

#[cxx::bridge]
mod ffi {
    struct TracerProvider {
        name: String,
    }

    extern "Rust" {
        fn get_tracer_provider() -> &TracerProvider;
    }
}

#[derive(Default)]
pub struct RustTracerProvider {
    provider: TracerProvider,
}

impl RustTracerProvider {
    pub fn new() -> Self {
        Self {
            provider: TracerProvider::default(),
        }
    }
}

pub fn get_tracer_provider() -> *mut TracerProvider {
    let provider = Box::new(RustTracerProvider::new().provider);
    Box::into_raw(provider)
}