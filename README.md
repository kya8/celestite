# Introduction
**Celestite** is a collection of common C++ libraries. It is currently targeting C++17.

Celestite aims to be self-contained (no external dependencies), portable and conformant.

Everything (except macros, which are prefixed with **`CLST_`**) is placed under the **`clst`** namespace.

# What's included
* `clst::fixed_string`: A fixed-size string class, suitable for compile time string operations.
* `clst::type_name`: Generates human-readable name for types at compile time.
* `clst::thread_pool`: Simple thread pool implementation.
* `clst::meta`: Meta-programming facilities.
* `clst::iter`: Utilities for iterations.
* `clst::binary_stream`: Raw binary streams for endianness-aware I/O.
* Containers: Ring buffer, ...
* Miscellaneous utilities.

# To-do
* `clst::log`: Simple library for logging.
* Containers: `clst::inline_vector`, `static_vector`, ...
* `clst::hash`: Generic hashing functions.
* Scope guard.
* Type-erasing runtime-polymorphic object.
