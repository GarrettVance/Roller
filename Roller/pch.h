#pragma once

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d11_3.h>
#include <d2d1_3.h>
#include <d2d1effects_2.h>
#include <dwrite_3.h>
#include <wincodec.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <agile.h>
#include <concrt.h>

#include <complex>
#include <vector>       //  ghv 20190211 gotta have std::vector...
#include <sstream>      //  ghv 20190217 for std::wstringstream; 
#include <iomanip>      //  ghv 20181222 for "setprecision" used in stringstreams;
#include <string>

#include <Keyboard.h>
#include <Mouse.h>
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>

#pragma comment(lib, "DirectXTK")

#include "DirectXMesh.h"
#pragma comment(lib, "DirectXMesh")



#undef GHV_OPTION_DRAW_SPOKES

