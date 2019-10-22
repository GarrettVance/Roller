# Roller
DirectX D3D11 first-person roller coaster on the Lorenz Attractor. A brief video is online at https://youtu.be/v94JQgMI0G0.

Multisample anti-aliasing (MSAA) has recently been incorporated into Roller, resulting in significantly improved render quality for the Lorenz Attractor in the 3rd-person view. Use the keyboard F7 and F8 keys to respectively disable or enable MSAA. MSAA is enabled by default. Both a 3rd-person and a first-person perspective are rendered in multiple viewports. See the Screenshots directory for examples. Refer to the Multisample_AntiAliasing.cpp file in this repository for details of the MSAA mechanism.  

This is a complete solution for Visual Studio 2017 which builds a DirectX D3D11 application for Windows 10 UWP. You can build for either x64 debug or x64 release. 

Dependencies: The Roller project depends on two external libraries, namely Microsoft DirectXTK and DirectXMesh. As of October 22, 2019, these dependencies are satisfied through the nuget functionality in VisualStudio: namely Tools -> NuGet Package Manager -> Package Manager Console. See the "packages.config" file in this repository. If nuget is unavailable, download these libraries from Microsoft's github page (both DirectXTK and DirectXMesh distributions provide Visual Studio solution files targeting Windows 10 UWP). Build these libraries for x64, after which you can build the Roller project.  
