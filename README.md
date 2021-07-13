# sksv

Simple Keystroke Visualizer.

`sksv` is a small C program to display keystrokes. 
Key press detection and rendering are both done through X11 functions.

## Requirements

Building `sksv` requires the Xlib header files.

Optionally, `Xinerama` support can be removed by commenting out `XINERAMALIBS` and `XINERAMAFLAGS` in `config.mk`.

## Installation

Edit `config.mk` to match your local setup. `sksv` will be installed to `/usr/local` by default.

Run the following to install (with root if necessary):

    make clean install

## Usage

`sksv` can be used by simply running the binary with no additional arguments:

    sksv

### Font

By default, `sksv` will automatically source a font using fontconfig. 

Optionally, a custom font can be specified:

    sksv -fn "Fira Code"

Font size can be specified using the same argument:

    sksv -fn "Fira Code:size=30"

### Geometry 

The geometry argument can be used to specify the dimensions and position of the window.
This is done by passing a geometry argument using the X11 standard geometry format:

    [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]

The geometry can be passed to `sksv` manually:

    sksv -g 1920x50+0+800

Alternatively, an external program such as `slop` can be used to interactively query for a region:

    sksv -g $(slop)
