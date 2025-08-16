# krkrrel-ng

`krkrrel-ng` is a port of [Kirikiri Releaser](https://krkrz.github.io/krkr2doc/kr2doc/contents/Releaser.html) that can be compiled with newer toolchains and run on modern platforms.  

## Download

The following builds are automatically built from the latest source code by Github Actions.  

* [Win32 build](https://github.com/krkrsdl2/krkrrel-ng/releases/download/latest/krkrrel-win32.zip)
* [macOS build](https://github.com/krkrsdl2/krkrrel-ng/releases/download/latest/krkrrel-macos.zip)
* [Ubuntu build](https://github.com/krkrsdl2/krkrrel-ng/releases/download/latest/krkrrel-ubuntu.zip)

## Usage

Releaser accepts the following command line options:

* Folder name: Specify the target folder.
* `-go`: This option is skipped for compatibility purposes with the original Kirikiri Releaser.
* `-nowriterpf`: Do not write `default.rpf` in the project directory on exit.
* `-out <filename>`: Sets the output file path to `<filename>`.
* `-rpf <filename>`: Loads the profile from `<filename>` before starting the archive operation.

For example, specify:
```bash
/path/to/krkrrel /path/to/project/directory -out /path/to/data.xp3 -nowriterpf -go
```

## Cloning

To clone the repository, please use the following command in a terminal:

```bash
git clone --recursive https://github.com/krkrsdl2/krkrrel-ng
```
If you do not use the exact command above, source files will be missing files since the project uses Git submodules.

## Building

This project can be built by using the CMake build system.  
For more information about the system, please visit the following location: https://cmake.org/  

## Original project

Code from this project is based on the following projects:
* [Kirikiri Z](https://github.com/krkrz/krkrz) dev_multi_platform branch
* [krdevui](https://github.com/krkrz/krdevui)

## IRC Channel

Members of the Kirikiri SDL2 project can be found in the [#krkrsdl2 channel on freenode](https://webchat.freenode.net/?channel=#krkrsdl2).

## License

This code is based on a modified 3-clause BSD license. Please read `LICENSE` for more information.  
This project contains third-party components. Please view the license file in each component for more information.
