# TermUtils

Header-only library primarily meant for my own various projects.

C++17 or higher required (only tested with C++17).

## Overview

### CompatUtil

Defines aliases for filesystem and optional depending on whether it's experimental or not. Meant for increased C++17-compatibility

### TermUtils

Contains various detection utils for the terminal

### ColorPrinter

Stdout but more colorful. Supports 256 colors at the moment; may be augmented to support truecolor in the future.

### FilesystemUtil

Contains a few filesystem utils; aside path joining, it has a function for expanding home directory paths.
