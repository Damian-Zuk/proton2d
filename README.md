# Proton2D

## Getting started
At the moment, Proton engine is compatible with Windows only. Linux support is planned to be added in the near future.

#### Cloning the repository
Proton uses git submodules, therefore cloning should be done with the following command:
```
git clone --recursive https://github.com/Proton2D/proton
```

If you happen to clone this repository non-recursively, use `git submodule update --init ` to clone the necessary submodules.

## Building
Build configuration tool: <a href="https://premake.github.io/">Premake 5</a>

Supported operating systems:
- Windows

### Building projects
Use ```Win-Build-VS22``` script to generate project files for <b><i>Microsoft Visual Studio 2022</i></b> using <b><i>Premake5</i></b>.

### Building the game
Use ```Win-Build-Game``` script to build and copy game executable and content from project directory to separate output build directory. You can choose project you want to build, configuration of the build and the target output directory.

### Build configurations
Proton2D has three types of build configuration:
- <b>Debug</b>: debug build with editor included.
- <b>Release</b>: release build with editor included.
- <b>Distribution</b>: release build without editor.

## The Game Engine Architecture
### Inspiration
The Proton Game Engine architecture was deeply inspired from the 
<a href="https://github.com/TheCherno/Hazel">Hazel Game Engine</a>. 
Some modules as 
<b><i>Renderer</i></b>,
<b><i>Event System</i></b>,
<b><i>Debug Utilities</i></b>, parts of 
<b><i>Core Module</i></b> and implementation for 
<b><i>Windows Input</i></b> and application 
<b><i>Window</i></b> are direct copy of Hazel source code with some slight modifications here and there. 

#### The key differences

Proton2D uses almost the same set of libraries as Hazel Engine.
The only difference is the library used for object serialization, which Proton happens to use is the <i>nlohmann/json</i> parser.

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam pretium porta mauris eu accumsan. Nunc pulvinar feugiat ex, in rutrum orci venenatis eu. Etiam magna quam, porttitor eu sollicitudin sit amet, rutrum at orci. Nam at sagittis urna. Praesent iaculis felis nisl, eget lobortis nisi aliquam id. Etiam finibus non quam at lacinia.

## Sandbox project

## The plans
Lorem ipsum dolor sit amet, consectetur adipiscing elit. In libero risus, luctus ut lorem vel, ultricies suscipit felis. Vestibulum at laoreet ex. Curabitur efficitur sem nec elit pellentesque sollicitudin. Praesent id diam tellus. Nam nec dictum neque. Integer interdum quam lorem, at scelerisque ex auctor a. Cras laoreet libero eleifend dui condimentum malesuada.

## License
&copy; MIT License
