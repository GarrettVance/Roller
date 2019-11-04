//                      
//                      
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                      
//                 ghv : Garrett Vance : 20180607
//                      
//                      
//          Implementation of Parallel Transport Frames
//          following Andrew J. Hanson's 1995 paper 
//          "Parallel Transport Approach to Curve Framing".
//                      
//                      
//           DirectX 11 Application for Windows 10 UWP.
//                      
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                      

#include "pch.h"
#include "XModLorenzLoft.h"
#include "..\Common\DirectXHelper.h"

#include <fstream>
#include <string>
#include <sstream>
#include <iterator>

using namespace HvyDX;
using namespace DirectX;
using namespace Windows::Foundation;



using Microsoft::WRL::ComPtr;




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void XModLorenzLoft::FiniteDifferences_Derivative()
{
    double const e_underflow = 0.0000000001; // TODO: too small;


    for (UINT idx_nodes = 0; idx_nodes < loft_axon_arc_density; idx_nodes++)
    {
        //    Obtain an approximate first derivative dr/dt 
        //    (aka tangent vector) via finite difference techniques:

        UINT idx_neung = idx_nodes;
        UINT idx_song = (idx_nodes == (-1 + loft_axon_arc_density)) ? 0 : 1 + idx_nodes;

        double dt =
            loft_axons.at(idx_song).axon_elapsed_time -
            loft_axons.at(idx_neung).axon_elapsed_time; 

        if (abs(dt) < e_underflow)
        {
            if (dt < 0.00) dt = -1.00 * e_underflow;
            else if (dt > 0.00) dt = e_underflow;
        }

        XMVECTOR drdt = (loft_axons.at(idx_song).axon_position_V -
                loft_axons.at(idx_neung).axon_position_V) / (float)dt;

        XMStoreFloat3(
            &loft_axons.at(idx_neung).axon_tangent_drdt,
            drdt
        ); 
    }
}










template<typename T>
std::vector<T> gv_split_n(const std::string& line) {
    std::istringstream is(line);
    return std::vector<T>(std::istream_iterator<T>(is), std::istream_iterator<T>());
}







size_t XModLorenzLoft::ReadLorenzDataFile()
{  
    //  My typical Lorenz Attractor data file has 60 thousand records. 
    
    loft_axons.resize(1 + loft_axon_arc_density); 

#ifdef _DEBUG
    std::fstream gFStr("StrangeAttractor\\lorenz_debug.ghvdata", std::ios_base::in);
#else
    std::fstream gFStr("StrangeAttractor\\lorenz.ghvdata", std::ios_base::in);
#endif

    if (!gFStr.is_open()) { return 0; }

    std::string g_line = ""; 

    XMFLOAT3   tmp_position; 
    float tmp_time; 
    XMFLOAT3   tmp_drdt; 
    XMFLOAT3   tmp_d2rdt2; 
    VHG_Axonodromal_Vertex    tmp_axon;

    uint32_t idxLoop = 0; 
    uint32_t trueLoopCount = 0; 
    while (std::getline(gFStr, g_line))
    {
        //             Fields in data file:
        //      ==================================
        //      position_x, position_y, position_z
        //      elapsed_time
        //      dr/dt__x, dr/dt__y, dr/dt__z
        //      d2r/dt2__x, d2r/dt2__y; d2r/dt2__z
        //      ==================================


        // if (trueLoopCount > 1500)
        if (trueLoopCount > 1515)
        {
            std::istringstream gSS(g_line);
            gSS >> tmp_position.x >> tmp_position.y >> tmp_position.z >> tmp_time >> tmp_drdt.x >> tmp_drdt.y >> tmp_drdt.z >> tmp_d2rdt2.x >> tmp_d2rdt2.y >> tmp_d2rdt2.z;
            XMVECTOR   tmp_position_V = XMLoadFloat3(&tmp_position);
            tmp_axon = { tmp_position, tmp_position_V, tmp_time, tmp_drdt, tmp_d2rdt2 };
            loft_axons.at(idxLoop) = tmp_axon;
            idxLoop++;
        }

        trueLoopCount++;

        if (idxLoop >= loft_axon_arc_density)  // obfuscate;
        {
            break;
        }
    }
    
    gFStr.close(); //           CLOSE THE FILE !!!!! 

    return loft_axons.size();
}  





void XModLorenzLoft::HansonParallelTransportFrame()
{
    // TODO:   merge vectors  unit_tangent and transported_normal into one vector

    uint32_t card_surfpts = loft_axon_arc_density * (1 + loft_tube_facets); 
    std::vector<VHG_Vertex_PosTex>   *surface_points = new std::vector<VHG_Vertex_PosTex>(card_surfpts);


    std::vector<VHG_Vertex_PosTex>   *spoke_lines = new std::vector<VHG_Vertex_PosTex>;
    std::vector<VHG_Vertex_PosTex>   *surface_triangles = new std::vector<VHG_Vertex_PosTex>;

    size_t record_count = ReadLorenzDataFile();

    //          
    //  Bootstrap: Special case when idx_nodes == 0: 
    //  ========================================
    //  Construct what Andrew J. Hanson terms the "initial normal vector V0": 
    //   

    XMVECTOR axon_delta_r = loft_axons.at(1).axon_position_V - loft_axons.at(0).axon_position_V; 

    XMVECTOR g_zero = XMVector3Cross(loft_axons.at(0).axon_position_V, axon_delta_r);  

    XMVECTOR h_normalized_normal = XMVector3Normalize(XMVector3Cross(g_zero, axon_delta_r));

    loft_axons.at(0).transported_normal = h_normalized_normal;

    XMStoreFloat3(& (loft_axons.at(0).axon_normal), h_normalized_normal); // ghv: added 20190313; 

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //              End of bootstrap; 
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    XMVECTOR nut_curr; 
    XMVECTOR nut_next = XMVector3Normalize(XMLoadFloat3(&(loft_axons.at(0).axon_tangent_drdt)));
    const float epsilon_length = 0.0001f;

    for (UINT idx_frame = 0; idx_frame < loft_axon_arc_density; idx_frame++)
    {
        if (idx_frame < -1 + loft_axon_arc_density)
        {
            nut_curr = nut_next;
            nut_next = XMVector3Normalize(XMLoadFloat3(&(loft_axons.at(1 + idx_frame).axon_tangent_drdt)));

            XMVECTOR B_vector = XMVector3Cross(nut_curr, nut_next);

            float B_length = XMVectorGetX(XMVector3Length(B_vector));
            XMVECTOR next_transported_normal;

            if (B_length < epsilon_length)
            {
                next_transported_normal = loft_axons.at(idx_frame).transported_normal; 
            }
            else
            {
                XMVECTOR B_unit_vector = XMVector3Normalize(B_vector);
                XMVECTOR angle_theta_vector = XMVector3AngleBetweenNormals(nut_curr, nut_next);
                float angle_theta = XMVectorGetX(angle_theta_vector);
                XMMATRIX gv_rotation_matrix = XMMatrixRotationNormal(B_unit_vector, angle_theta);

                next_transported_normal = XMVector3Transform(
                    loft_axons.at(idx_frame).transported_normal,
                    gv_rotation_matrix
                );
            }

            loft_axons.at(1 + idx_frame).transported_normal = next_transported_normal; 

            XMStoreFloat3(&(loft_axons.at(1 + idx_frame).axon_normal), next_transported_normal); // ghv: added 20190313; 
        }

        XMVECTOR unit_normal = XMVector3Normalize(loft_axons.at(idx_frame).transported_normal);

        //     In order to orient and position the cross-section, 
        //     need another normal vector in addition to the transported_normal.
        //     I have chosen to use vector B = T cross N.

        XMVECTOR binormal = XMVector3Cross(
            XMVector3Normalize(XMLoadFloat3(&(loft_axons.at(idx_frame).axon_tangent_drdt))), 
            unit_normal
        );

        XMStoreFloat3(& (loft_axons.at(idx_frame).axon_binormal), binormal); // ghv: added 20190315; 

        //   TODO: confirm that the binormal doesn't need explicit normalization;

        for (uint32_t k = 0; k <= loft_tube_facets; ++k)  //  HAZARD : loop upper limit is <= not < !!!!
        {
            //     Poloidal loop: 

            float t_fraction = k / (float)loft_tube_facets;

            float angle_phi = t_fraction * 2 * XM_PI;

            float C_x = loft_tube_radius * cosf(angle_phi);
            float C_y = loft_tube_radius * sinf(angle_phi);

            XMVECTOR vectorP = loft_axons.at(idx_frame).axon_position_V + C_x * unit_normal + C_y * binormal; 

            XMFLOAT3 tmp_f3;
            XMStoreFloat3(&tmp_f3, vectorP); 
            VHG_Vertex_PosTex tmp_surface_point;  //  or can use  = { tmp_f3, XMFLOAT2(0.f, 0.f) };
            tmp_surface_point.e_pos = tmp_f3; 
            tmp_surface_point.e_texco = XMFLOAT2(0.f, 0.f);

            uint32_t ii = idx_frame * (1 + loft_tube_facets) + k; 

            surface_points->at(ii).e_pos = tmp_f3; 
            surface_points->at(ii).e_texco = XMFLOAT2(0.f, 0.f);

            //  std::vector just for LINELIST topology to show radial segments
            //  from space curve out to points on the tube surface:

            VHG_Vertex_PosTex tmp_spoke;
            tmp_spoke.e_pos = (loft_axons.at(idx_frame)).axon_position_r;
            tmp_spoke.e_texco = XMFLOAT2(0.f, 0.f);
            spoke_lines->push_back(tmp_spoke);
            spoke_lines->push_back(tmp_surface_point);
        }
        // for each cross-section facet;
    }

    loft_vertex_buffer_1_count = (uint32_t)spoke_lines->size();
    
    Create_Vertex_Buffer(
        spoke_lines,
        loft_vertex_buffer_1_buffer.ReleaseAndGetAddressOf() 
    );  // use topology = LINELIST;
    

    VHG_Vertex_PosTex quad_top_left;
    VHG_Vertex_PosTex quad_top_right;
    VHG_Vertex_PosTex quad_bottom_right;
    VHG_Vertex_PosTex quad_bottom_left;

    for (uint32_t i_axial = 0; i_axial < -1 + loft_axon_arc_density; i_axial++)
    {
        for (uint32_t i_poloidal = 0; i_poloidal < loft_tube_facets; i_poloidal++)
        {
            //   set the texture coordinates: 

            uint32_t ii = i_poloidal + (loft_tube_facets + 1) * i_axial;

            quad_top_left = { surface_points->at(0 + ii).e_pos,                             XMFLOAT2(0.f, 1.f), i_axial };
            quad_top_right = { surface_points->at((loft_tube_facets + 1) + ii).e_pos,        XMFLOAT2(0.f, 0.f), i_axial };
            quad_bottom_right = { surface_points->at((loft_tube_facets + 2) + ii).e_pos,     XMFLOAT2(1.f, 0.f), i_axial };
            quad_bottom_left = { surface_points->at(1 + ii).e_pos,                          XMFLOAT2(1.f, 1.f), i_axial };

            //      Synthesize the 1st of the 2 triangles:
            surface_triangles->push_back(quad_top_left);
            surface_triangles->push_back(quad_top_right);
            surface_triangles->push_back(quad_bottom_right);

            //      Synthesize the 2nd triangle:
            surface_triangles->push_back(quad_bottom_right);
            surface_triangles->push_back(quad_bottom_left);
            surface_triangles->push_back(quad_top_left);
        }
    }


    loft_vertex_buffer_2_count = (uint32_t)surface_triangles->size(); 

    Create_Vertex_Buffer(
        surface_triangles,
        loft_vertex_buffer_2_buffer.ReleaseAndGetAddressOf() 
    );  // use topology = TRIANGLELIST;


#if 3 == 4

    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //  
    //  Use DirectXMesh ComputeNormals() method 
    //  to generate vertex normals for each triangle: 
    //      
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    XMFLOAT2 nul2 = XMFLOAT2(0.f, 0.f);
    XMFLOAT3 nul3 = XMFLOAT3(0.f, 0.f, 0.f);
    XMVECTOR nulXMV = XMVectorSet(0.f, 0.f, 0.f, 0.f);

    int32_t countOfVertices = (int32_t)surface_triangles->size(); 

    int32_t countOfTriangles = countOfVertices / 3; 
    DWORD *arrTriangles = new DWORD[countOfVertices]{ 0 }; 

    int32_t idxCWS = 0; 
    for (idxCWS = 0; idxCWS < countOfVertices; idxCWS++)
    {
        arrTriangles[idxCWS] = idxCWS;
    }

    XMFLOAT3 *arrVertices = new XMFLOAT3[countOfVertices]{ nul3 }; 
    XMFLOAT2 *arrTexco = new XMFLOAT2[countOfVertices]{ nul2 }; 
    int32_t idxVertx = 0; 

    std::vector<VHG_Vertex_PosTex>::iterator itVertx; 
    for (itVertx = surface_triangles->begin(); itVertx != surface_triangles->end(); ++itVertx) 
    {
        arrVertices[idxVertx] = (*itVertx).e_pos;
        arrTexco[idxVertx] = (*itVertx).e_texco;
        idxVertx++;
    }

    XMFLOAT3 *normals = new XMFLOAT3[countOfVertices]{nul3}; 

    DirectX::ComputeNormals(
        (uint32_t*)arrTriangles, 
        countOfTriangles, 
        arrVertices, 
        countOfVertices, 
        CNORM_DEFAULT,
        normals
    ); 

    //  Move the newly generated normals data into the e_master_lofts vector: 

    idxVertx = 0; 
    for (itVertx = surface_triangles->begin(); itVertx != surface_triangles->end(); ++itVertx)
    {
        (*itVertx).e_normal = normals[idxVertx]; 
        idxVertx++;
    }


    Platform::String^ plat_local_folder =
        Windows::Storage::ApplicationData::Current->LocalFolder->Path;
    std::wstring local_folder_w(plat_local_folder->Begin());
    std::string loclal_folder_a(local_folder_w.begin(), local_folder_w.end());
    char fully_qualified_path_a[512];

    sprintf_s(
        fully_qualified_path_a, 512,
        "%s\\ghv_lorenz_data.txt",
        (const char*)loclal_folder_a.c_str()
    );

#endif 



#if 3 == 4
    std::ofstream gv_ofstream(fully_qualified_path_a, std::ios::trunc); 
    if (!gv_ofstream.is_open()) 
    { 
        DX::ThrowIfFailed(E_FAIL); 
    }

    for (uint32_t idxFile = 0; idxFile < countOfVertices; idxFile++) 
    {
        char file_record[256];
        sprintf_s(
            file_record, 256,
            "%10.6f \t %10.6f \t %10.6f \t %10.6f \t %10.6f \t %10.6f \t %10.6f \t %10.6f \n",  
            surface_triangles->at(idxFile).e_pos.x,  
            surface_triangles->at(idxFile).e_pos.y,  
            surface_triangles->at(idxFile).e_pos.z,  
            surface_triangles->at(idxFile).e_normal.x, 
            surface_triangles->at(idxFile).e_normal.y,  
            surface_triangles->at(idxFile).e_normal.z,  
            surface_triangles->at(idxFile).e_texco.x, 
            surface_triangles->at(idxFile).e_texco.y
        );
        gv_ofstream << file_record;
    }
    gv_ofstream.close(); 
#endif

    delete surface_triangles; surface_triangles = nullptr;
    delete spoke_lines; spoke_lines = nullptr;
    delete surface_points; surface_points = nullptr;
}  






