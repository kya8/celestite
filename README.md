# Brief
**Celestite** (or abbreviated as `clst`) is a collection of utilities
and libraries for working with video files produced by some cameras,
written in C++.

# Supported vendors

* GoPro
* Insta360

# Command-line tools

Download from GitHub Actions:

[![CI](https://github.com/kya8/celestite/actions/workflows/ci.yml/badge.svg?event=push)](https://github.com/kya8/celestite/actions/workflows/ci.yml)

* `clst inspet`: Inspect and dump various information and metadata extracted from video files.

* `clst insta360_normalize`: Undistort panoramic images from Insta360 cameras.

* `clst mp4join`: Join consecutive mp4 files in a lossless manner. Useful for cameras (e.g. GoPro) that split output video into fixed-sized chapters.
Copy of [mp4join](https://github.com/kya8/mp4join).

* `clst calibrator`: Simple wrapper for camera calibration utilities in OpenCV.

# Names
__Celestite__: A mineral consisting of _strontium sulfate_ ($SrSO_4$).
  * C++ entities are scoped in `namespace clst`.
  * Macros and C functions are prefixed with `CLST_`.
