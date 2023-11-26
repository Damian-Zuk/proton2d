# proton

## About
<b>Proton2D</b> is a game engine build with simplicity in mind.
It has following featues:
- Game Editor (written using <a href="https://github.com/ocornut/imgui">ImGui</a>)
- Entity Component System (ECS) architecture (powered by <a href="https://github.com/skypjack/entt">EnTT</a> library)
- Integrated external physics engine <a href="https://github.com/erincatto/box2d">Box2D</a>
- Scene system to manage game entities
- Native C++ Entity Scripting
- Spritesheet support
- Tile-based animation
- Resizable sprites (9-scaling method)
- Entity prefabs (will be reworked)

## Getting started
At the moment, Proton engine is compatible with Windows only. Linux support is planned to be added in the near future.

#### Cloning the repository
Proton uses the <b>git submodules</b>, therefore repository cloning should be done with the following command:
```
git clone --recursive https://github.com/Proton2D/proton
```

If you happen to clone this repository non-recursively, use `git submodule update --init ` to clone the necessary submodules.

## Building
<b>Build configuration tool:</b> Premake 5

<b>Supported platforms:</b> Windows

### Building solution projects
Run ```Win-Build-VS22``` script to generate solution files for <b>Microsoft Visual Studio 2022</b> via <b>Premake5</b> build tool.

### Building the game
Run ```Win-Build-Game``` script to build and copy game executable and content from project directory to separate output build directory. You can choose a project by providing its name, configuration of the build and the target output directory. Default values can be modified inside the script. Proton does not currently support binary asset packing, all files from the `content` directory are be copied directly to the output directory while running the script.

### Build configurations
There are three types of build configuration in Proton2D:
- <b>Debug configuration</b>: This build is intended for development and debugging purposes.
- <b>Release configuration</b> This build offers a more optimized performance than the Debug build, while still retaining the full functionality of the game editor.
- <b>Distribution configuration</b>: This is the deployment-ready build, optimized for end users. It excludes a game editor, ensuring an efficient application.

## The Game Engine Architecture
### Inspiration
The Proton game engine architecture is based on the 
<a href="https://github.com/TheCherno/Hazel">Hazel Game Engine</a> architecture. 
Some modules as 
<b><i>Renderer</i></b>,
<b><i>Event System</i></b>,
<b><i>Debug Utilities</i></b> and implementation for Windows
<b><i>Input</i></b> and application 
<b><i>Window</i></b> were directly copied from the Hazel source code with some slight modifications. Code division into modules (the `src` directory structure) is very similar to that found in Hazel and in other game engines.
Most of the libraries used in Proton are also used in the Hazel engine.
The only difference is the library for <b>Entity Serialization</b>, which Proton happens to use is the <a href="https://github.com/nlohmann/json">nlohmann/json</a> library.

### Game Engine Editor

| Panel | Description |
| ----------- | ---------- |
| Scene Hierarchy |  Hierarchy of entities on the scene. Click RMB to create new entity on the scene root or click on entity to create new child entity. You can drag and drop entities to change hierarchy.  |
| Inspector | Panel where you can edit game objects (entities) by modifing their component values. |
| Scene | Scene simulation Play, Pause and Stop buttons. View of scenes loaded in memory. This will be changed to scene tabs and above editor.  |
| Prefab | List of prefabs that can be spawned or deleted. |
| Misc | General application settings and statistics. |


Proton editor is integrated into the game runtime because engine does not have external langauge scripting or hot reloading implemented yet.


### Entity Scripting Method
Proton at the moment offers only native C++ scripting with enchanced interface via `ScriptFactory` register macro. This approach has the disadvantage of requiring a recompilation and restart of the application for script changes, as it cannot hot reload scripts during execution. Native C++ scripting however, is generally faster in terms of performance than external language scripting. External script engine will be propably added in the future, it was not implemented yet due to the lack of time.

### Entity Component System (ECS)

## Sandbox project
The `sandbox` project is the game project in which the game is developed. It is staticly linked with the engine. It contains an example game made in Proton (more things). 

## The plans
Current plan for the project is to have networking implemented and functional game UI system. Other things that will done in the future is: Scripting Engine, Asset Packing, Audio.

## License
&copy; Licensed under the MIT License.
