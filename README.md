# Introduction
**Celestite** is a collection of common C++ libraries. It is currently targeting C++17.

Celestite aims to be self-contained (no external dependencies), portable and conformant.

Everything (except macros) is placed under the **`clst`** namespace.

___WARNING___: This is currently a toy project intended for my personal use only.

# What's included
* `clst::fixed_string`: A fixed-size string class, suitable for compile time string operations.
* `clst::type_name`: Generates human-readable name for types at compile time.
* `clst::thread_pool`: Simple thread pool implementation.
* `clst::meta`: Meta-programming facilities.
* `clst::binary_stream`: Raw binary streams for endianness-aware I/O.
* Miscellaneous utilities, found under `clst::util`, `clst::types`, ...

# To-do
* `clst::log`: Simple library for logging.
* `clst::num_cast`: Numerical type cast that throws error if the arithmetic value cannot be preserved.
* `clst::container::inline_vector`: Small-size-optimized vector.
* `clst::argparse`: Command-line argument parsing.
* `clst::numerical`, `clst::algorithm`...
