# Roller
DirectX D3D11 first-person roller coaster on the Lorenz Attractor. Multisample antialiasing (MSAA) has just recently been incorporated into Roller resulting in significantly improved render quality for the Lorenz Attractor. 

This is a complete solution for Visual Studio 2017 which builds a DirectX D3D11 application for Windows 10 UWP. You can build for either x64 debug or x64 release. 

While running the application, use the keyboard F5 and F6 keys to toggle the view between 3rd-person and first-person perspective. See the Screenshots directory for examples. 

Dependencies: The Roller project depends on two external libraries, namely Microsoft DirectXTK and DirectXMesh. As of October 22, 2019, these dependencies are satisfied through the nuget functionality in VisualStudio: namely Tools -> NuGet Package Manager -> Package Manager Console. See the "packages.config" file in this repository. If nuget is unavailable, download these libraries from Microsoft's github page (both DirectXTK and DirectXMesh distributions provide Visual Studio solution files targeting Windows 10 UWP). Build these libraries for x64, after which you can build the Roller project.  
