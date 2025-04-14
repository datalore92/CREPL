# C REPL - Interactive C Expression Evaluator

An enhanced, graphical Read-Eval-Print Loop (REPL) for evaluating C expressions, built with SDL2.

![C REPL](https://via.placeholder.com/800x600.png?text=C+REPL+Screenshot)

## Features

- **Interactive Console**: A modern, graphical interface for evaluating C expressions
- **Expression Evaluation**: Calculate arithmetic expressions like `5 + 3`, `10 * (3 + 2)`
- **Variable Support**: Define and use variables (e.g., `x = 5`)
- **Command History**: Navigate through previously entered commands with Up/Down keys
- **Syntax Highlighting**: Color-coded output for prompts, results, and errors
- **Built-in Commands**:
  - `help` - Display help information
  - `clear` - Clear the console
  - `vars` - Display all defined variables
  - `version` - Display version information
  - `exit`/`quit` - Exit the REPL
- **Scrolling with Mouse**: Scroll through output history with mouse wheel
- **Customizable View Modes**: Toggle between scrolling, fixed, and paged views
- **Cross-platform**: Works on both Windows and Linux systems

## Requirements

- Windows or Linux operating system
- SDL2 and SDL2_ttf libraries

## Building from Source

### Prerequisites

- CMake (version 3.10 or later)
- A C compiler that supports C11
- SDL2 and SDL2_ttf development libraries

#### Installing Dependencies

**On Windows:**
- Download SDL2 and SDL2_ttf from their official websites and place them in the `lib/` directory
- Or install via MSYS2/MinGW: `pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_ttf`

**On Linux (Ubuntu/Debian):**
```bash
sudo apt-get update
sudo apt-get install libsdl2-dev libsdl2-ttf-dev
```

**On Linux (Fedora/RHEL/CentOS):**
```bash
sudo dnf install SDL2-devel SDL2_ttf-devel
```

**On Linux (Arch):**
```bash
sudo pacman -S sdl2 sdl2_ttf
```

### Build Steps

1. Clone the repository:

```bash
git clone https://github.com/yourusername/REPL.git
cd REPL
```

2. Create and navigate to a build directory:

```bash
mkdir build
cd build
```

3. Generate the build files with CMake:

```bash
cmake ..
```

4. Build the project:

```bash
make
```
or on Windows with MinGW:
```bash
mingw32-make
```

5. Run the application:

```bash
./crepl
```

## Project Structure

```
├── include/                # Header files
│   ├── repl_core.h         # Core REPL definitions and functions
│   ├── repl_eval.h         # Expression evaluation
│   ├── repl_history.h      # Command history management
│   ├── repl_input.h        # Input handling
│   ├── repl_ui.h           # UI rendering functions
│   ├── repl_variables.h    # Variable management
│   └── repl.h              # Main header that includes all components
├── src/                    # Source files
│   ├── main.c              # Entry point
│   ├── repl_core.c         # Core REPL implementation
│   ├── repl_eval.c         # Expression evaluation implementation
│   ├── repl_history.c      # Command history implementation
│   ├── repl_input.c        # Input handling implementation
│   ├── repl_ui.c           # UI rendering implementation
│   └── repl_variables.c    # Variable management implementation
├── lib/                    # Library dependencies
│   ├── SDL2/               # SDL2 library files
│   └── SDL2_ttf/           # SDL2_ttf library files
├── build/                  # Build output directory
├── CMakeLists.txt          # CMake build configuration
├── LICENSE                 # MIT License file
└── README.md               # This file
```

## Keyboard Shortcuts

| Shortcut | Function |
|----------|----------|
| Up/Down | Navigate command history |
| Left/Right | Move cursor |
| Home/End | Jump to start/end of line |
| Alt+Home/End | Jump to top/bottom of output |
| PageUp/PageDown | Scroll output by pages |
| Alt+V | Toggle view mode (scroll/fixed/paged) |
| Alt+S | Toggle auto-scroll |
| Escape | Clear current input |

## Mouse Controls

- Mouse wheel - Scroll through output
- Click & drag scrollbar - Navigate history

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## Acknowledgments

- SDL2 and SDL2_ttf libraries for providing graphical capabilities