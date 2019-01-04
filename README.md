# MSH to FBX Converter

This tool converts Meshes, Skeletons and Animations from Pandemics mesh format (MSH2), used in the old Star Wars Battlefront (2004, 2005) series, into Autodesks FBX format which is known by all major Game Engines and 3D Modelling Software.

## How to use
1. Download latest build from [Releases](https://github.com/Ben1138/MSH2FBX/releases)
2. Extract
3. Run ```MSH2FBX.exe --help``` in a Terminal to see all available options.
<br /><br />
Be carefull with the Ignoring Options regarding Points and Bones (Skeleton)! They might brake parentships and result in an useless FBX!<br />
Meshes should be safe to ignore though.
<br /><br />
###Example
```MSH2FBX.exe -m rep_inf_ep3trooper.msh -i Mesh_ShadowVolume```
This will convert rep_inf_ep3trooper.msh (lieing in the same directory as MSH2FBX.exe) to rep_inf_ep3trooper.fbx, ignoring Shadow Volumes.
## Pull and Compile
The folowing process is described for Visual Studio 2017 in x64 release mode. For other build modes, adapt the options respectively.
<br />
1. Download [FBX SDK VS2015](https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2019-0) (if not already)
2. Clone [LibSWBF2](https://github.com/Ben1138/LibSWBF2) repository (if not already)
3. In "Configuration Properties / C/C++ / General" Add path to "Additional Include Directories" pointing at the FBX SDK include directory (default: C:\Program Files\Autodesk\FBX\FBX SDK\2019.0\include)
4. In "Configuration Properties / C/C++ / General" Add path to "Additional Include Directories" pointing at the LibSWBF2.DLL directory (to find in LibSWBF2 repository)
5. In "Configuration Properties / Linker / General" Add path to "Additional Library Directories" pointing at the appropriate FBX SDK library directory (default: C:\Program Files\Autodesk\FBX\FBX SDK\2019.0\lib\vs2015\x64\release)
6. In Configuration "Properties / Linker / General" Add path to "Additional Library Directories" pointing at the appropriate LibSWBF2 library directory (default: LibSWBF2\x64\$(Configuration))
7. Make sure "Configuration Properties / C/C++ / Language / Conformance Mode" is set to NO (FBX headers wont compile otherwise)
7. Make sure "Configuration Properties / C/C++ / Language / C++ Language Standard" is set to ISO C++ 17
7. Make sure "Configuration Properties / Linker / Input / Additional Dependencies" includes libfbxsdk.lib and LibSWBF2.lib
8. Download or compile the LibSWBF2.dll and copy it to the output directory of MSH2FBX (default: MSH2FBX\x64\Release)
9. Make sure a copy of libfbxsdk.dll (default: C:\Program Files\Autodesk\FBX\FBX SDK\2019.0\lib\vs2015\x64\release) exists in the output directory of MSH2FBX (default: MSH2FBX\x64\Release)
9. Compile MSH2FBX
