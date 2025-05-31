# QuickJS Tools

A set of tools for developing code which runs in QuickJS.

## Building

### macOS

First, install XCode command line tools with:
```shell
xcode-select --install
```

Next, to install dependencies with [Homebrew](https://brew.sh):
```shell
brew install boost
```

### Debian and derivatives (Ubuntu, Linux Mint, etc.)

To install dependencies with `apt`:
```shell
apt install build-essential cmake libboost-program-options-dev
```

### Build

To build this project, use `CMake`:
```shell
cmake -B build
cmake --build build --config Release
```

Add the `-j` argument to run multiple compilation jobs in parallel,
for example. `cmake --build build --config Release -j 4` will run 4 jobs in parallel.

Built executables will be available in the `build/` folder.

# Tools

For more information, all tools support the `-h`/`--help` flag for help.

## QuickJS Interrupt Explorer

This tool facilitates exploring interruption points within JS code,
and allows for easy simulation of interruptions.

**Notes:**
- Interruption point indexing starts at zero

**Example Usage:**

```shell
# Run the file test.js and list the total number of interruption points
quickjs_interrupt_explorer -f test.js

# Same as above, but in addition log whenever an interruption point is reached
quickjs_interrupt_explorer -f test.js -v

# Run test.js and interrupt the second interruption point (NOTE: interruption indexing starts at 0)
quickjs_interrupt_explorer -f test.js -i 1
```
