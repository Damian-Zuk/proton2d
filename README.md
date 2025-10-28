# proton

## About
<b>Proton2D</b> is a simple open-source game engine specifically designed for making 2D platformer games. 

The main features of the engine are:
- Game Editor written using <a href="https://github.com/ocornut/imgui">ImGui</a> library,
- Entity Component System (ECS) architecture powered by the <a href="https://github.com/skypjack/entt">EnTT</a> library,
- External Physics Engine <a href="https://github.com/erincatto/box2d">Box2D</a>,
- Prebuilt Entity Components,
- Scenes system to manage game entities,
- Native C++ Entity Scripting,
- Spritesheet support,
- Spritesheet Tile-based Animation,
- Resizable Sprites using the 9-scaling method,
- Entity Prefabs (will be reworked).

<details>
<summary><b>Game Editor Preview (expand)</b></summary>
<img src="https://i.imgur.com/jJWpWKr.png" alt="Game editor"></img>
</details>

## Getting Started
At the moment, the Proton engine is compatible with Windows only. Linux support is planned to be added in the near future.

#### Cloning The Repository
Proton uses the <b>git submodules</b>, therefore repository cloning should be done with the following command:
```
git clone --recursive https://github.com/Proton2D/proton
```
If you happen to clone this repository non-recursively, use `git submodule update --init ` to clone the necessary submodules.

## Building
<b>Build configuration tool:</b> Premake 5

<b>Supported platforms:</b> Windows

### Building Solution Projects
Run the ```Win-Build-VS22``` script to generate solution files for <b>Microsoft Visual Studio 2022</b> via <b>Premake5</b>.

### Building The Game
Run the ```Win-Build-Game``` script to build and copy the game executable and content from the project directory to a separate output build directory. You can choose a project by providing its name, the configuration of the build, and the target output directory. Default values can be modified inside the script. Proton does not currently support binary asset packing, all files from the `content` directory will be copied directly to the output directory while running the script.

### Build Configurations
There are three types of build configurations in Proton2D:
- <b>Debug configuration</b>: This build is intended for development and debugging purposes.
- <b>Release configuration</b> This build offers more optimized performance than the debug build, while still retaining the full functionality of the game editor.
- <b>Distribution configuration</b>: This is the deployment-ready build, optimized for end users. It excludes a game editor, ensuring an efficient application.

## The Game Engine Architecture
### Inspiration
The Proton2D game engine architecture is mainly based on the 
<a href="https://github.com/TheCherno/Hazel">Hazel Engine</a> architecture. The resources and materials provided there by the Cherno helped me learn a lot about the game engine and software architecture, which I am very thankful for. I highly recommend checking it out for anyone who is interested in the game engine architecture. Note: I am not an expert game engine developer, this is just my personal project that I worked on, and I will continue working on for some time.

Modules as 
<b>Renderer</b>,
<b>Event</b> system,
<b>Debug</b> utilities and implementation for Windows
<b>Input</b> and application 
<b>Window</b> were directly copied from the Hazel source code with some slight modifications. The organization of code into modules, represented by the `src/Proton` directory structure, closely resembles the structure found in Hazel and other game engines.
Most of the libraries used in Proton are also used in Hazel.
The only difference is the library for entity serialization, which Proton happens to use, is the <a href="https://github.com/nlohmann/json">nlohmann/json</a> library.

### Libraries Used
- <a href="https://github.com/glfw/glfw">GLFW</a>
- <a href="https://glad.dav1d.de/">Glad</a>
- <a href="https://github.com/nothings/stb/blob/master/stb_image.h">stb_image</a>
- <a href="https://github.com/g-truc/glm">glm</a>
- <a href="https://github.com/ocornut/imgui">ImGui</a>
- <a href="https://github.com/skypjack/entt">EnTT</a>
- <a href="https://github.com/erincatto/box2d">Box2D</a>
- <a href="https://github.com/nlohmann/json">nlohmann/json</a>
- <a href="https://github.com/gabime/spdlog">spdlog</a>

### Entity Scripting
Proton at the moment, offers only Native C++ Scripting. To create an entity script, you must create a class that derives from the `EntityScript` base class. Inside the created class, a macro `ENTITY_SCRIPT_CLASS(class)` must be placed (under the public members). It will register a script inside the `ScriptFactory` class.

The approach of native scripting has the disadvantage of requiring a recompilation and restart of the application for script changes, as it cannot hot-reload scripts during runtime. Native C++ scripting however, is generally faster in terms of performance than external language scripting. The external scripting engine will probably be added in the future. It was not implemented yet due to the lack of time.

## Sandbox Project
The `sandbox` is the project in which the game is developed. It contains an example game made in Proton2D. Window properties can be changed by modifing the `sandbox/app-config.json` config file. A proper script for creating new game projects from the template will be added soon.

## License
&copy; Licensed under the MIT License.
