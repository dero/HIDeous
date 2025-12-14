# Contributing to HIDeous

Thank you for your interest in contributing to HIDeous!

## Prerequisites

- **Visual Studio 2022** (Community edition is fine)
- **CMake** (3.15 or newer)

## Building the Project

You can build the project using Visual Studio directly or via the command line with CMake.

### Option 1: Visual Studio

1. Open `HIDeous.sln` in Visual Studio.
2. Select **Release** configuration.
3. Right-click on the Solution in the Solution Explorer and select **Build Solution**.

### Option 2: Command Line (CMake)

1. Open a terminal in the project root.
2. Configure the project:
   ```bash
   cmake -S . -B build
   ```
3. Build the project:
   ```bash
   cmake --build build --config Release
   ```
4. The executables will be in `build/bin/Release/`.

## Project Structure

- `src/app`: The main GUI application code.
- `src/dll`: The hook DLL that intercepts keyboard input.
- `src/common`: Shared code between the app and the DLL (settings, logging, constant).
