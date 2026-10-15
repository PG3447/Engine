# MimiCry Experiments

# Game

Two scientists at a secret research facility are studying demons in an attempt to harness their power. However, the experiment begins to spiral out of control.
Unable to see one of the primary colors (one is red-blind, the other green-blind), they must work together to solve the research procedures and bring the situation under control.

## Requirements

- GPU supporting OpenGL 4.6
- GPU supporting `GL_ARB_bindless_texture` ([Check compatibility](https://www.realtech-vr.com/glview/))
- [Visual C++ Redistributable (x64)](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist)

## How to Run

1. Download the latest release from the [Releases](../../releases) page.
2. Extract the downloaded archive.
3. Run the executable.

## Executables

- `MimiCry_Experiments.exe` - Main game.
- `MimiCry_Experiments_ImGui.exe` - Engine editor.

# 3D Game Engine

## About

A 3D game engine developed by Zespół pięśni i hazardu.

## How to Build?

Run the following command to generate the build:

```
cmake -B build
```

The generated `.sln` file will be located in the `/build` directory. Open it in Visual Studio, right-click the `MimiCry_Experiments` project in Solution Explorer, and select **Set as Startup Project**. Then build and run the application.

To compile C++ compiler with C++20 support is required. 
