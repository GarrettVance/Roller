//          
//          
//  File VertexTangents.cpp   Eric Lengyel's method; 
//  Garrett Vance 20191022
//  Project Roller: 
//  What if somebody built a roller coaster whose track 
//  traced the trajectory of a Lorenz Attractor?
//              
//          

#include "pch.h"
#include "Hvy3DScene.h"
#include "..\Common\DirectXHelper.h"

using namespace HvyDX;
using namespace DirectX;
using namespace Windows::Foundation;






void XModMandelpod::ComputeVertexTangents(
    std::vector<WaveFrontReader<DWORD>::WFR_Vertex>  &   refVertices, 
    std::vector<DWORD> const & refIndices
)
{
    //      
    //  Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary Mesh”. 
    //  Terathon Software, 2001. 
    //  http://terathon.com/code/tangent.html
    //

    uint32_t countOfIndices = (uint32_t)refIndices.size();
    uint32_t countOfTriangles = countOfIndices / 3;
    uint32_t countOfVertices = (uint32_t)refVertices.size(); 

    DirectX::XMVECTOR        *tan1Alloc = new DirectX::XMVECTOR[2 * countOfVertices]; 
    DirectX::XMVECTOR        *tan2Ptr = tan1Alloc + countOfVertices;  // do math on raw pointers;
        
    float w_component = 0.f;

    for (uint32_t a = 0; a < countOfTriangles; a++)
    {
        uint32_t i1 = refIndices.at(0 + 3 * a); 
        uint32_t i2 = refIndices.at(1 + 3 * a); 
        uint32_t i3 = refIndices.at(2 + 3 * a); 

        XMFLOAT3 const & v1 = refVertices.at(i1).position; 
        XMFLOAT3 const & v2 = refVertices.at(i2).position; 
        XMFLOAT3 const & v3 = refVertices.at(i3).position; 

        XMFLOAT2 const & w1 = refVertices.at(i1).textureCoordinate;
        XMFLOAT2 const & w2 = refVertices.at(i2).textureCoordinate;
        XMFLOAT2 const & w3 = refVertices.at(i3).textureCoordinate;

        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;

        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;

        float r = 1.0f / (s1 * t2 - s2 * t1);

        DirectX::XMVECTOR sdir = XMVectorSet(
            (t2 * x1 - t1 * x2) * r, 
            (t2 * y1 - t1 * y2) * r, 
            (t2 * z1 - t1 * z2) * r, 
            w_component
        );  

        DirectX::XMVECTOR tdir = XMVectorSet(
            (s1 * x2 - s2 * x1) * r, 
            (s1 * y2 - s2 * y1) * r,
            (s1 * z2 - s2 * z1) * r, 
            w_component
        );

        tan1Alloc[i1] += sdir;
        tan1Alloc[i2] += sdir;
        tan1Alloc[i3] += sdir;

        tan2Ptr[i1] += tdir;
        tan2Ptr[i2] += tdir;
        tan2Ptr[i3] += tdir;

    }  // Closes "for" loop;


    for (uint32_t a = 0; a < countOfVertices; a++)
    {
        //  Retrieve the already-computed Vertex Normal provided by the WaveFront OBJ file: 

        DirectX::XMVECTOR const & WaveFrontNormal = XMVectorSet( 
            refVertices[a].normal.x, 
            refVertices[a].normal.y, 
            refVertices[a].normal.z, 
            w_component
        );  

        DirectX::XMVECTOR const & urTangent = tan1Alloc[a]; 

        // Gram-Schmidt orthogonalize to get the x, y and z components of the Vertex Tangent: 

        XMVECTOR dotXMV = XMVector3Dot(WaveFrontNormal, urTangent);
        XMFLOAT3 dotF3; 
        XMStoreFloat3(&dotF3, dotXMV); 
        float dotFloat = dotF3.x; 

        XMVECTOR tan3Abnormal = urTangent - XMVectorScale(WaveFrontNormal, dotFloat);
        XMVECTOR tan3Hat = XMVector3Normalize(tan3Abnormal); 

        //  Compute the chirality and store it in the w-component of the Vertex Tangent: 

        XMVECTOR crossXMV = XMVector3Cross(WaveFrontNormal, urTangent); 
        XMVECTOR dotCrossXMV = XMVector3Dot(crossXMV, tan2Ptr[a]); 

        XMFLOAT3 dotCrossF3;
        XMStoreFloat3(&dotCrossF3, dotCrossXMV); 
        float dotCrossFloat = dotCrossF3.x; 

        float w_chirality = (dotCrossFloat < 0.0f) ? -1.0f : +1.0f;

        XMVECTOR finallyTangentXMV = XMVectorSetW(tan3Hat, w_chirality); 

        XMVECTOR BitangentXMV = XMVectorScale(
            XMVector3Cross(WaveFrontNormal, finallyTangentXMV),
            w_chirality
        );

        //   And now convert back to XMFLOAT3: 

        XMFLOAT3 tmpTangent;
        XMStoreFloat3(&tmpTangent, finallyTangentXMV); 
        refVertices[a].tangent = tmpTangent; 

        XMFLOAT3 tmpBitangent;
        XMStoreFloat3(&tmpBitangent, BitangentXMV); 
        refVertices[a].bitangent = tmpBitangent;
    }  //  Closes "for" loop; 

    delete[] tan1Alloc;
}





