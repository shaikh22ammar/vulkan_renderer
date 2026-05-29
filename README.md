# Vulkan renderer

This renderer is my attempt at Khornos' updated Vulkan tutorial. 
The tutorial and other Vulkan tutorials like vkGuide largely tell you what to code instead of telling how to code.
I have tried to remedy that by extensively referring to the spec and adding annotations as comments in the critical parts.

While it is based on the Vulkan tutorial, it heavy diverges in many place especially in regards to performance.
I don't see the point of teaching Vulkan if you do not teach the benefits of Vulkan.
For example, I use push constants for MVP matrix.
The tutorial sends all the 3 matrices separately whichs exceed the guaranteed limits of push constants.
I instead multiply them in the CPU. Since I use cglm instead of glm, the multiplication is faster due
to SIMD exploitation.
I also put in some effort in suballocating memory: packing multiple data in a single buffer whilst taking care of alignment.

I also create wrappers around cglm to use vulkan coordinates instead of resorting to hacks in arbitrary places.


# Build instructions

GLFW, Vulkan SDK, and pkg-config are required dependencies.
Non-POSIX systems are not supprted.
1. Use `make isMacOS = false` for devices that natively support Vulkan, otherwise `make` on MacOS, for a debug build. 
2. Use `make [isMacOS = ...] isDebug = false` for optimised build without validation layers.
3. Run using `./run.sh`

# Examples 

![Square](recordings/square.gif)
![Viking room](recordings/viking_room.gif)
