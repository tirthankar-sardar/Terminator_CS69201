# Terminator — Custom Terminal Emulator

**Terminator** is a custom terminal emulator and shell implemented in C using the X11 library. It provides a graphical terminal interface with support for command execution, multiple tabs, shell features, process management, and job control.

The project demonstrates operating system concepts such as process creation, inter-process communication, pipes, file descriptors, signal handling, and event-driven programming.

## 1. Features

### Graphical Interface
- Native X11 terminal window.
- Interactive command prompt and output display.
- Keyboard input handling using Xlib.
- Scrolling through terminal output.
- Multiple tabs with independent terminal sessions.

### Shell Functionality
- Execution of external commands using `fork()` and `execvp()`.
- Built-in `cd` and `exit` commands.
- Command pipelines using `|`.
- Input and output redirection using `<` and `>`.
- Support for multiple commands and command sequences.
- Wildcard expansion and argument parsing.
- Tab-specific working directories.
- Tab-scoped shell variables.
- Multi-line command continuation using `\`.

### Process Management
- Foreground and background process execution.
- Process groups and signal handling.
- Job control using `Ctrl+Z` and `Ctrl+C`.
- Background job status tracking.
- Multi-command monitoring using `multiWatch`.
- Non-blocking event handling for terminal responsiveness.

### Additional Features
- Command history.
- Command autocomplete.
- Keyboard cursor navigation and editing.
- Multiple terminal tabs with independent state.

## 2. Dependencies

Terminator requires a C compiler, Make, and the X11 development libraries.

### Linux (Ubuntu/Debian)

Install the required dependencies:

```bash
sudo apt update
sudo apt install build-essential libx11-dev
```

If you are using a Linux environment without a graphical desktop, you may also need an X server to display the terminal window.

### macOS

Terminator can be built and run on macOS using XQuartz.

Install the dependencies:

```bash
brew install gcc make
```

Install XQuartz from:

https://www.xquartz.org/

After installation, launch XQuartz and ensure that X11 applications are permitted to connect to the X server.

The project Makefile currently uses the following X11 paths:

```makefile
CFLAGS = -Wall -Wextra -I/opt/X11/include
LDFLAGS = -L/opt/X11/lib -lX11
```

These paths are appropriate for a typical XQuartz installation on macOS. If your X11 installation uses different paths, update the Makefile accordingly.

## 3. Building the Project

Clone the repository:

```bash
git clone <repository-url>
```

Navigate to the project directory:

```bash
cd terminator
```

Compile the project using Make:

```bash
make
```

This compiles the source files and generates the executable:

```text
terminator
```

To remove the compiled executable:

```bash
make clean
```

## 4. Running Terminator

Terminator supports two execution modes.

### Mode 1: Normal Execution

Run Terminator using:

```bash
./terminator
```

This launches the graphical terminal emulator with the normal execution mode.

### Mode 2: Process-Based Execution

Run Terminator using:

```bash
./terminator useProcess=True
```

This launches Terminator with the process-based execution mode enabled.

The process-based mode is intended to demonstrate the alternative process execution mechanism implemented in the project.

## 5. Usage

Once Terminator launches, commands can be entered directly into the graphical terminal window.

### Basic Commands

```bash
ls
pwd
echo Hello World
date
```

### Changing Directories

```bash
cd /tmp
pwd
```

Each terminal tab maintains its own working directory.

### Input and Output Redirection

Read input from a file:

```bash
./myprog < infile.txt
```

Redirect output to a file:

```bash
echo Hello World > outfile.txt
```

Combine input and output redirection:

```bash
./myprog < infile.txt > outfile.txt
```

### Pipes

```bash
ls | wc -l
```

Multiple commands can be connected using pipelines:

```bash
cat infile.txt | grep hello | wc -l
```

### Command Continuation

Long commands can be continued across multiple lines using `\`.

### Multiple Tabs

- Open new terminal tabs.
- Switch between tabs.
- Close tabs.
- Execute commands independently in different tabs.

### Job Control

The terminal supports process management and job control operations, including:

- `Ctrl+C` — interrupt the foreground process.
- `Ctrl+Z` — suspend the foreground process.
- Background job management and status tracking.

## 6. Project Structure

The project is organized into modular C source and header files.

| File | Description |
|---|---|
| `main.c` | Application entry point and main event loop |
| `gui.c` | X11 window creation, rendering, and tab interface |
| `input.c` | Keyboard input and command handling |
| `shell.c` | Shell command parsing and execution |
| `io_redirect.c` | Input and output redirection |
| `variables.c` | Shell variable management |
| `scroll.c` | Terminal output scrolling |
| `hotkeys.c` | Keyboard shortcuts and cursor navigation |
| `history.c` | Command history |
| `completion.c` | Command autocomplete |
| `script.c` | Script execution support |
| `terminal.h` | Shared terminal structures and declarations |
| `Makefile` | Build configuration |

## 7. Known Limitations

- Terminator relies on X11 and requires a working graphical display.
- Some advanced features may behave differently from a standard Linux shell such as Bash.
- Shell parsing is implemented by the project and may not support every Bash syntax or edge case.
- Terminal rendering is based on basic Xlib drawing and does not provide a complete terminal-emulation specification.
- Compatibility depends on the operating system and availability of the required POSIX and X11 libraries.
- The process-based execution mode depends on the process management implementation provided by the project.

## 8. Technologies Used

- **C** — Core implementation.
- **Xlib / X11** — Graphical terminal interface.
- **POSIX system calls** — Process creation, pipes, file descriptors, and signals.
- **Make** — Build automation.
- **Unix process management** — Foreground/background execution and job control.

## 9. Learning Objectives

This project explores practical operating system concepts, including:

- Process creation and execution.
- Inter-process communication using pipes.
- File descriptor manipulation.
- Input/output redirection.
- Signal handling and process groups.
- Job control and process synchronization.
- Event-driven graphical programming.
- Modular systems programming in C.

## 10. Authors and Contributions

This project was developed as part of an operating systems / systems programming project.

Contributions include the implementation of the graphical terminal interface, shell functionality, process management, command execution, and additional terminal features.

---

**Terminator — A custom terminal emulator and shell built from scratch in C.**
