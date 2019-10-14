//--------------------------------------------------------------------------------------
// File: WaveFrontReader.h
//
// Code for loading basic mesh data from a WaveFront OBJ file
//
// http://en.wikipedia.org/wiki/Wavefront_.obj_file
//
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
//
// http://go.microsoft.com/fwlink/?LinkID=324981
//--------------------------------------------------------------------------------------

#pragma once

#pragma warning(push)
#pragma warning(disable : 4005)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOGDI
#define NOMCX
#define NOSERVICE
#define NOHELP
#pragma warning(pop)

#include <windows.h>

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>

#include <stdint.h>

#include <directxmath.h>
#include <directxcollision.h>






#ifndef DIRECTX_NOEXCEPT
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define DIRECTX_NOEXCEPT
#else
#define DIRECTX_NOEXCEPT noexcept
#endif
#endif









template<class index_t>
class WaveFrontReader
{
public:


    typedef index_t index_t;




    struct WFR_Vertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT3 normal;
        DirectX::XMFLOAT2 textureCoordinate;
        DirectX::XMFLOAT3 tangent;   // ghv added 20190204;
        DirectX::XMFLOAT3 bitangent;   // ghv added 20190204;
    };







    WaveFrontReader() DIRECTX_NOEXCEPT : hasNormals(false), hasTexcoords(false) {}   //  Class ctor for the WaveFrontReader class;







    HRESULT Load(_In_z_ const wchar_t* p_dot_obj_filename, bool ccw = true)
    {
        Clear();

        static const size_t MAX_POLY = 64;

        using namespace DirectX;



        std::wifstream InFile(p_dot_obj_filename);


        if (!InFile)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        wchar_t fname[_MAX_FNAME] = {};

        _wsplitpath_s(p_dot_obj_filename, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, nullptr, 0);

        name = fname;

        std::vector<XMFLOAT3>   positions;
        std::vector<XMFLOAT3>   normals;
        std::vector<XMFLOAT2>   texCoords;

        VertexCache  vertexCache;






        CWS_Material defmat;

        wcscpy_s(defmat.strName, L"default");

        plector_materials.emplace_back(defmat);





        uint32_t curSubset = 0;




        wchar_t external_dot_mtl_filename[MAX_PATH] = {};



        for (;; )
        {
            std::wstring strCommand;
            InFile >> strCommand;
            if (!InFile)
                break;

            if (*strCommand.c_str() == L'#')
            {
                // Comment
            }
            else if (0 == wcscmp(strCommand.c_str(), L"o"))
            {
                // Object name ignored
            }
            else if (0 == wcscmp(strCommand.c_str(), L"g"))
            {
                // Group name ignored
            }
            else if (0 == wcscmp(strCommand.c_str(), L"s"))
            {
                // Smoothing group ignored
            }
            else if (0 == wcscmp(strCommand.c_str(), L"v"))
            {
                // Vertex Position
                float x, y, z;
                InFile >> x >> y >> z;
                positions.emplace_back(XMFLOAT3(x, y, z));
            }
            else if (0 == wcscmp(strCommand.c_str(), L"vt"))
            {
                // Vertex TexCoord
                float u, v;
                InFile >> u >> v;
                texCoords.emplace_back(XMFLOAT2(u, v));

                hasTexcoords = true;
            }
            else if (0 == wcscmp(strCommand.c_str(), L"vn"))
            {
                // Vertex Normal
                float x, y, z;
                InFile >> x >> y >> z;
                normals.emplace_back(XMFLOAT3(x, y, z));

                hasNormals = true;
            }
            else if (0 == wcscmp(strCommand.c_str(), L"f"))
            {
                // Face
                INT iPosition, iTexCoord, iNormal;
                WFR_Vertex vertex;

                DWORD faceIndex[MAX_POLY];
                size_t iFace = 0;
                for (;;)
                {
                    if (iFace >= MAX_POLY)
                    {
                        // Too many polygon verts for the reader
                        return E_FAIL;
                    }

                    memset(&vertex, 0, sizeof(vertex));

                    InFile >> iPosition;

                    UINT vertexIndex = 0;
                    if (!iPosition)
                    {
                        // 0 is not allowed for index
                        return E_UNEXPECTED;
                    }
                    else if (iPosition < 0)
                    {
                        // Negative values are relative indices
                        vertexIndex = UINT(positions.size() + iPosition);
                    }
                    else
                    {
                        // OBJ format uses 1-based arrays
                        vertexIndex = iPosition - 1;
                    }

                    if (vertexIndex >= positions.size())
                        return E_FAIL;

                    vertex.position = positions[vertexIndex];

                    if ('/' == InFile.peek())
                    {
                        InFile.ignore();

                        if ('/' != InFile.peek())
                        {
                            // Optional texture coordinate
                            InFile >> iTexCoord;

                            UINT coordIndex = 0;
                            if (!iTexCoord)
                            {
                                // 0 is not allowed for index
                                return E_UNEXPECTED;
                            }
                            else if (iTexCoord < 0)
                            {
                                // Negative values are relative indices
                                coordIndex = UINT(texCoords.size() + iTexCoord);
                            }
                            else
                            {
                                // OBJ format uses 1-based arrays
                                coordIndex = iTexCoord - 1;
                            }

                            if (coordIndex >= texCoords.size())
                                return E_FAIL;

                            vertex.textureCoordinate = texCoords[coordIndex];
                        }

                        if ('/' == InFile.peek())
                        {
                            InFile.ignore();

                            // Optional vertex normal
                            InFile >> iNormal;

                            UINT normIndex = 0;
                            if (!iNormal)
                            {
                                // 0 is not allowed for index
                                return E_UNEXPECTED;
                            }
                            else if (iNormal < 0)
                            {
                                // Negative values are relative indices
                                normIndex = UINT(normals.size() + iNormal);
                            }
                            else
                            {
                                // OBJ format uses 1-based arrays
                                normIndex = iNormal - 1;
                            }

                            if (normIndex >= normals.size())
                                return E_FAIL;

                            vertex.normal = normals[normIndex];
                        }
                    }

                    // If a duplicate vertex doesn't exist, add this vertex to the Vertices
                    // list. Store the index in the Indices array. 
                    //                  
                    //              
                    //  ghv : 
                    //  ghv : The Vertices and Indices lists will eventually become 
                    //  ghv : the Vertex Buffer and Index Buffer for the mesh. 
                    //  ghv :       


                    DWORD index = AddVertex(vertexIndex, &vertex, vertexCache);
                    if (index == (DWORD)-1)
                        return E_OUTOFMEMORY;


#pragma warning( suppress : 4127 )
                    if (sizeof(index_t) == 2 && (index >= 0xFFFF))
                    {
                        // Too many indices for 16-bit IB!
                        return E_FAIL;
                    }
#pragma warning( suppress : 4127 )
                    else if (sizeof(index_t) == 4 && (index >= 0xFFFFFFFF))
                    {
                        // Too many indices for 32-bit IB!
                        return E_FAIL;
                    }

                    faceIndex[iFace] = index;
                    ++iFace;


                    // Check for more face data or end of the face statement
                    bool faceEnd = false;
                    for (;;)
                    {
                        wchar_t p = InFile.peek();

                        if ('\n' == p || !InFile)
                        {
                            faceEnd = true;
                            break;
                        }
                        else if (isdigit(p) || p == '-' || p == '+')
                            break;

                        InFile.ignore();
                    }

                    if (faceEnd)
                        break;
                }

                if (iFace < 3)
                {
                    // Need at least 3 points to form a triangle
                    return E_FAIL;
                }

                // Convert polygons to triangles
                DWORD i0 = faceIndex[0];
                DWORD i1 = faceIndex[1];

                for (size_t j = 2; j < iFace; ++j)
                {
                    DWORD index = faceIndex[j];
                    plector_indices.emplace_back(static_cast<index_t>(i0));
                    if (ccw)
                    {
                        plector_indices.emplace_back(static_cast<index_t>(i1));
                        plector_indices.emplace_back(static_cast<index_t>(index));
                    }
                    else
                    {
                        plector_indices.emplace_back(static_cast<index_t>(index));
                        plector_indices.emplace_back(static_cast<index_t>(i1));
                    }

                    plector_attributes.emplace_back(curSubset);

                    i1 = index;
                }

                assert(plector_attributes.size() * 3 == plector_indices.size());
            }
            else if (0 == wcscmp(strCommand.c_str(), L"mtllib"))
            {
                //  ghv : The WaveFront OBJ file contains a "mtllib" directive 
                //  ghv : to identify an external dot.mtl file, e.g. 
                //  ghv : 
                //  ghv : mtllib Trapezohedron_Blender_Exported.mtl  
                //  ghv :   
                //  ghv : The "mtllib" directive's argument is the non-qualified 
                //  ghv : file name of the external dot.mtl file. 
                //          

                InFile >> external_dot_mtl_filename;
            }
            else if (0 == wcscmp(strCommand.c_str(), L"usemtl"))
            {
                //  ghv : The WaveFront OBJ file contains "usemtl" directives, e.g. 
                //  ghv :     
                //  ghv : usemtl None 
                //  ghv : usemtl None_JuliaSetOne.bmp
                //  ghv : usemtl None_Mandelbrot.png ...etc...
                //  ghv :     
                //  ghv : Each "usemtl" directive has an argument giving 
                //  ghv : a kind of logical name to each CWS_Material. 
                //  ghv : The logical CWS_Material name is then found inside the dot.mtl file. 
                //    

                wchar_t usemtl_material_nickname[MAX_PATH] = {};

                InFile >> usemtl_material_nickname;

                bool bFound = false;
                uint32_t count = 0;
                for (auto it = plector_materials.cbegin(); it != plector_materials.cend(); ++it, ++count)
                {
                    //   ghv : traverse the whole std::vector of materials 

                    if (0 == wcscmp(it->strName, usemtl_material_nickname))
                    {
                        bFound = true;
                        curSubset = count;
                        break;
                    }
                }

                if (!bFound)
                {
                    CWS_Material mat;   // ghv : No match of usemtl_material_nickname so must instantiate new CWS_Material object; 

                    curSubset = static_cast<uint32_t>(plector_materials.size());

                    wcscpy_s(mat.strName, MAX_PATH - 1, usemtl_material_nickname);

                    plector_materials.emplace_back(mat);
                }
            }
            else
            {
#ifdef _DEBUG
                // Unimplemented or unrecognized command
                OutputDebugStringW(strCommand.c_str());
#endif
            }

            InFile.ignore(1000, '\n');
        }

        if (positions.empty())
            return E_FAIL;

        // Cleanup
        InFile.close();

        BoundingBox::CreateFromPoints(bounds, positions.size(), positions.data(), sizeof(XMFLOAT3));

        // If an associated material file was found, read that in as well.


        if (*external_dot_mtl_filename)
        {
            //  ghv : Get the extension .xyz from "external_dot_mtl_filename". Expect it to be "mtl": 

            wchar_t mater_extension[_MAX_EXT] = {};

            _wsplitpath_s(external_dot_mtl_filename, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, mater_extension, _MAX_EXT);



            //  ghv : Get the disk drive and the directory from "p_dot_obj_filename": 

            wchar_t mater_drive[_MAX_DRIVE] = {};
            wchar_t mater_dir[_MAX_DIR] = {};

            _wsplitpath_s(p_dot_obj_filename, mater_drive, _MAX_DRIVE, mater_dir, _MAX_DIR, nullptr, 0, nullptr, 0);



            //  ghv : Combine what has been discovered to synthesize a fully-qualified path: 
            //  ghv : mater_drive + mater_dir + mater_FNAME + mater_extension = fully-qualified filename. 

            wchar_t szPath[MAX_PATH] = {};

            _wmakepath_s(szPath, MAX_PATH, mater_drive, mater_dir, fname, mater_extension);




            HRESULT hr = LoadMTL(szPath);
            if (FAILED(hr))
                return hr;
        }

        return S_OK;
    }
    //  Closes Load();  
















    HRESULT LoadMTL(_In_z_ const wchar_t* p_dot_mtl_file)
    {
        using namespace DirectX;


        //  ghv :   
        //  ghv : Assumes MTL is in CWD along with OBJ !!! 
        //  ghv : Assumes MTL is in CWD along with OBJ !!! 
        //  ghv : Assumes MTL is in CWD along with OBJ !!! 
        //  ghv :   


        std::wifstream InFile(p_dot_mtl_file);

        if (!InFile)
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        auto curMaterial = plector_materials.end();

        for (;; )
        {
            std::wstring strCommand;
            InFile >> strCommand;
            if (!InFile)
                break;

            if (0 == wcscmp(strCommand.c_str(), L"newmtl"))
            {
                // Switching active materials
                wchar_t strName[MAX_PATH] = {};
                InFile >> strName;

                curMaterial = plector_materials.end();
                for (auto it = plector_materials.begin(); it != plector_materials.end(); ++it)
                {
                    if (0 == wcscmp(it->strName, strName))
                    {
                        curMaterial = it;
                        break;
                    }
                }
            }


            // The rest of the commands rely on an active material

            if (curMaterial == plector_materials.end())
                continue;

            if (0 == wcscmp(strCommand.c_str(), L"#"))
            {
                // Comment
            }
            else if (0 == wcscmp(strCommand.c_str(), L"Ka"))
            {
                // Ambient color
                float r, g, b;
                InFile >> r >> g >> b;
                curMaterial->vAmbient = XMFLOAT3(r, g, b);
            }
            else if (0 == wcscmp(strCommand.c_str(), L"Kd"))
            {
                // Diffuse color
                float r, g, b;
                InFile >> r >> g >> b;
                curMaterial->vDiffuse = XMFLOAT3(r, g, b);
            }
            else if (0 == wcscmp(strCommand.c_str(), L"Ks"))
            {
                // Specular color
                float r, g, b;
                InFile >> r >> g >> b;
                curMaterial->vSpecular = XMFLOAT3(r, g, b);
            }
            else if (0 == wcscmp(strCommand.c_str(), L"d") ||
                0 == wcscmp(strCommand.c_str(), L"Tr"))
            {
                // Alpha
                InFile >> curMaterial->fAlpha;
            }
            else if (0 == wcscmp(strCommand.c_str(), L"Ns"))
            {
                // Shininess
                int nShininess;
                InFile >> nShininess;
                curMaterial->nShininess = nShininess;
            }
            else if (0 == wcscmp(strCommand.c_str(), L"illum"))
            {
                // Specular on/off
                int illumination;
                InFile >> illumination;
                curMaterial->bSpecular = (illumination == 2);
            }
            else if (0 == wcscmp(strCommand.c_str(), L"map_Kd"))
            {
                // Texture
                InFile >> curMaterial->strTexture;
            }
            else
            {
                // Unimplemented or unrecognized command
            }

            InFile.ignore(1000, L'\n');
        }

        InFile.close();

        return S_OK;
    }
    //  Closes LoadMTL();  










    void Clear()
    {
        plector_vertices.clear();
        plector_indices.clear();
        plector_attributes.clear();
        plector_materials.clear();
        name.clear();
        hasNormals = false;
        hasTexcoords = false;

        bounds.Center.x = bounds.Center.y = bounds.Center.z = 0.f;
        bounds.Extents.x = bounds.Extents.y = bounds.Extents.z = 0.f;
    }













    HRESULT LoadVBO(_In_z_ const wchar_t* p_vbo_file)
    {
        Clear();

        wchar_t fname[_MAX_FNAME] = {};

        _wsplitpath_s(p_vbo_file, nullptr, 0, nullptr, 0, fname, _MAX_FNAME, nullptr, 0);

        name = fname;



        CWS_Material defmat;
        wcscpy_s(defmat.strName, L"default");
        materials.emplace_back(defmat);



        std::ifstream vboFile(p_vbo_file, std::ifstream::in | std::ifstream::binary);
        if (!vboFile.is_open())
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

        hasNormals = hasTexcoords = true;

        uint32_t numVertices = 0;
        uint32_t numIndices = 0;

        vboFile.read(reinterpret_cast<char*>(&numVertices), sizeof(uint32_t));
        if (!numVertices)
            return E_FAIL;

        vboFile.read(reinterpret_cast<char*>(&numIndices), sizeof(uint32_t));
        if (!numIndices)
            return E_FAIL;


        plector_vertices.resize(numVertices);

        vboFile.read(reinterpret_cast<char*>(plector_vertices.data()), sizeof(WFR_Vertex) * numVertices);


#pragma warning( suppress : 4127 )
        if (sizeof(index_t) == 2)
        {
            plector_indices.resize(numIndices);
            vboFile.read(reinterpret_cast<char*>(plector_indices.data()), sizeof(uint16_t) * numIndices);
        }
        else
        {
            std::vector<uint16_t> tmp;
            tmp.resize(numIndices);
            vboFile.read(reinterpret_cast<char*>(tmp.data()), sizeof(uint16_t) * numIndices);

            plector_indices.reserve(numIndices);
            for (auto it = tmp.cbegin(); it != tmp.cend(); ++it)
            {
                plector_indices.emplace_back(*it);
            }
        }

        BoundingBox::CreateFromPoints(bounds, plector_vertices.size(), reinterpret_cast<const XMFLOAT3*>(plector_vertices.data()), sizeof(WFR_Vertex));

        vboFile.close();

        return S_OK;
    }
    //  Closes LoadVBO();  













    struct CWS_Material
    {
        DirectX::XMFLOAT3 vAmbient;

        DirectX::XMFLOAT3 vDiffuse;

        DirectX::XMFLOAT3 vSpecular;

        uint32_t nShininess;

        float fAlpha;

        bool bSpecular;

        wchar_t strName[MAX_PATH];

        wchar_t strTexture[MAX_PATH];



        CWS_Material() DIRECTX_NOEXCEPT :      //  Class ctor for "CWS_Material" class; 
            vAmbient(0.2f, 0.2f, 0.2f),
            vDiffuse(0.8f, 0.8f, 0.8f),
            vSpecular(1.0f, 1.0f, 1.0f),
            nShininess(0),
            fAlpha(1.f),
            bSpecular(false)
        {
            memset(strName, 0, sizeof(strName)); memset(strTexture, 0, sizeof(strTexture));
        }
    };









    std::vector<WFR_Vertex>         plector_vertices;    //  ghv : plector = "public not private" + "std::vector"; 
    std::vector<index_t>            plector_indices;
    std::vector<uint32_t>           plector_attributes;
    std::vector<CWS_Material>           plector_materials;



    std::wstring            name;
    bool                    hasNormals;
    bool                    hasTexcoords;

    DirectX::BoundingBox    bounds;

private:
    typedef std::unordered_multimap<UINT, UINT> VertexCache;

    DWORD AddVertex(UINT hash, WFR_Vertex* pVertex, VertexCache& cache)
    {
        auto f = cache.equal_range(hash);

        for (auto it = f.first; it != f.second; ++it)
        {
            auto& tv = plector_vertices[it->second];

            if (0 == memcmp(pVertex, &tv, sizeof(WFR_Vertex)))
            {
                return it->second;
            }
        }

        DWORD index = static_cast<UINT>(plector_vertices.size());
        plector_vertices.emplace_back(*pVertex);

        VertexCache::value_type entry(hash, index);
        cache.insert(entry);
        return index;
    }
    //  Closes AddVertex(); 


};
//  Closes class WaveFrontReader; 




