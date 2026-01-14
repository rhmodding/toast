# toast
[icon?]
the ultimate cellanim modding tool for Rhythm Heaven Fever and Rhythm Heaven Megamix

[![version 0.9](https://img.shields.io/badge/version-0.9-slateblue)]()

toast is a more modern RH cellanim (BCCAD/BRCAD) editor, made with QoL in mind, for modders with varying levels of experience.

![Screenshot of toast in action](img/image.png)

## Why not Bread?

Here's a list of cool stuff toast can do but not Bread:

- open / save .szs & .zlib cellanim archives directly
- advanced editing features (multi-select, canvas transform, etc.)
- auto tweening
- conversion between RVL <-> CTR (Fever <-> Megamix) formats
- automatic image format conversion
- auto sheet reorganisation (repacking)

## Releases

You can get the latest stable* release at [the releases page](https://github.com/rhmodding/toast/releases/latest), or check out the Actions tab for latest master!

\* toast is pre-1.0 right now, so bugs ahead!

## Building

You'll need a C/C++ toolchain of some sort installed (MinGW, XCode, build-essentials or equivalent, ...). MSVC is __not supported__.

1. Clone this repository with `git clone --recursive https://github.com/rhmodding/toast`
2. Prepare CMake for build with `mkdir build && cmake -B build`
3. Build the project with `cmake --build build`
4. You should now have a toast executable in the `build` folder! Enjoy!

## Texture format support

CTPK (3DS texture) support on toast is still underway! Currently, the only formats supported are:

- RGBA4444 (read)
- ETC1A4 (read/write)

###### toat