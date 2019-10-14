//                      
//                      
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                      
//                 ghv : Garrett Vance : 20180607
//                      
//                      
//          Implementation of Parallel Transport Frames
//		    following Andrew J. Hanson's 1995 paper 
//          "Parallel Transport Approach to Curve Framing".
//                      
//                      
//           DirectX 11 Application for Windows 10 UWP.
//                      
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                      

#include "pch.h"
#include "ParallelTransportFrame.h"
#include "..\Common\DirectXHelper.h"

#include <fstream>
#include <string>
#include <sstream>
#include <iterator>

using namespace HvyDX;
using namespace DirectX;
using namespace Windows::Foundation;




ParallelTransportFrame::ParallelTransportFrame(const std::shared_ptr<DX::DeviceResources>& deviceResources) 
    :
    m_deviceResources(deviceResources),  
    ptf_loadingComplete(false),
    ptf_rasterizer_fill_mode(D3D11_FILL_WIREFRAME)
{  
    //   
    //  My typical Lorenz Attractor data file has 60 thousand records. 
    //  

#ifdef _DEBUG

    this->ptf_axon_arc_density = 4096;   // Use 4096 to speed launch of debug builds;

#else

    this->ptf_axon_arc_density = 16384;   //  

#endif

    //      Configure the tubular loft:

    this->ptf_tube_radius = 0.05f;      // Spoke length and tube radius > 0.01. 

    this->ptf_tube_facets = 6;
}  





void ParallelTransportFrame::Update(
    XMMATRIX        const&           p_ParallelTransportFrameWorldMatrix,
    XMMATRIX        const&           p_parentSceneViewMatrix,
    XMMATRIX        const&           p_parentSceneProjectionMatrix
)
{
    DirectX::XMStoreFloat4x4(
        &ptf_WVP_Data.model,
        XMMatrixTranspose(
            p_ParallelTransportFrameWorldMatrix
        )
    );

    DirectX::XMStoreFloat4x4(
        &ptf_WVP_Data.view,
        XMMatrixTranspose(
            p_parentSceneViewMatrix
        )
    );

    DirectX::XMStoreFloat4x4(
        &ptf_WVP_Data.projection,
        XMMatrixTranspose(
            p_parentSceneProjectionMatrix
        )
    );
}






void ParallelTransportFrame::Render()  
{ 
    if (!ptf_loadingComplete)
    {
        return;
    }

    auto context = m_deviceResources->GetD3DDeviceContext();

    // Send WVP data to constant buffer

    context->UpdateSubresource1( ptf_WVP_Buffer.Get(),   0,  NULL, &ptf_WVP_Data,    0,   0,  0 );


    context->VSSetConstantBuffers1( 0,   1,   ptf_WVP_Buffer.GetAddressOf(), nullptr,  nullptr );


    Create_Rasterizer_State();  // TODO: remove this call, as rasterizer state never changes for ptf loft;


    context->RSSetState(ptf_rasterizer_state.Get());



    context->IASetInputLayout(ptf_inputLayout.Get());


    context->VSSetShader(ptf_vertexShader.Get(), nullptr, 0);
  

    context->PSSetShader(ptf_pixelShader.Get(), nullptr, 0);
    // unused context->PSSetShaderResources(0, 1, ptf_loft_texture_srv.GetAddressOf());
    // unused context->PSSetSamplers(0, 1, ptf_loft_texture_sampler_state.GetAddressOf());




#ifdef GHV_OPTION_DRAW_SPOKES
    UINT vertex_buffer_1_stride = sizeof(VHG_Vertex_PosTex);
    UINT vertex_buffer_1_offset = 0;
    context->IASetVertexBuffers(
        0,
        1,
        ptf_vertex_buffer_1_buffer.GetAddressOf(),
        &vertex_buffer_1_stride,
        &vertex_buffer_1_offset
    );
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    context->Draw(this->ptf_vertex_buffer_1_count, 0);
#endif





    UINT vertex_buffer_2_stride = sizeof(VHG_Vertex_PosTex);
    UINT vertex_buffer_2_offset = 0;
    context->IASetVertexBuffers(
        0,
        1,
        ptf_vertex_buffer_2_buffer.GetAddressOf(),
        &vertex_buffer_2_stride,
        &vertex_buffer_2_offset
    );

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->Draw( ptf_vertex_buffer_2_count, 0 );
}  
//  Closes ParallelTransportFrame::Render;  

























//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void ParallelTransportFrame::Create_Vertex_Buffer(
    std::vector<HvyDX::VHG_Vertex_PosTex>  *p_vect_vertices,
    ID3D11Buffer** p_buffer_object
)
{ 
    size_t   bytes_required_allocation =
        sizeof(VHG_Vertex_PosTex) * p_vect_vertices->size();


    const VHG_Vertex_PosTex   *const_data_ptr = &(*p_vect_vertices)[0];


    D3D11_SUBRESOURCE_DATA vertexBufferData = { 0 };
    ZeroMemory(&vertexBufferData, sizeof(vertexBufferData));


    vertexBufferData.pSysMem = const_data_ptr;
    vertexBufferData.SysMemPitch = 0;
    vertexBufferData.SysMemSlicePitch = 0;


    CD3D11_BUFFER_DESC vertexBufferDesc(
        (UINT)bytes_required_allocation,   // the total required allocation;  
        D3D11_BIND_VERTEX_BUFFER
    );

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            p_buffer_object
        )
    );
}  
//  Closes ParallelTransportFrame::Create_Vertex_Buffer; 







//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void ParallelTransportFrame::gv_finite_differences()
{
	double const e_underflow = 0.0000000001;


	for (UINT idx_nodes = 0; idx_nodes < ptf_axon_arc_density; idx_nodes++)
	{

		//   
        //    Obtain an approximate first derivative dr/dt 
		//    (aka tangent vector) via finite difference techniques:
		//               

		UINT idx_neung = idx_nodes;

		UINT idx_song = (idx_nodes == (-1 + ptf_axon_arc_density)) ? 0 : 1 + idx_nodes;

		//  assert the space curve is a closed loop (ouroboros); 


		double dt =
			ptf_curve_derivatives.at(idx_song).axon_elapsed_time -
			ptf_curve_derivatives.at(idx_neung).axon_elapsed_time; 

		if (abs(dt) < e_underflow)
		{
			if (dt < 0.00) dt = -1.00 * e_underflow;
			else if (dt > 0.00) dt = e_underflow;
		}


		double tangent_drdt_x =
			(ptf_curve_derivatives.at(idx_song).axon_position_r.x -
			 ptf_curve_derivatives.at(idx_neung).axon_position_r.x) / dt;

		double tangent_drdt_y =
			(ptf_curve_derivatives.at(idx_song).axon_position_r.y -
			 ptf_curve_derivatives.at(idx_neung).axon_position_r.y) / dt;

		double tangent_drdt_z =
			(ptf_curve_derivatives.at(idx_song).axon_position_r.z -
			 ptf_curve_derivatives.at(idx_neung).axon_position_r.z) / dt;

			
		ptf_curve_derivatives.at(idx_neung).axon_tangent_drdt = DirectX::XMFLOAT3(
			(float)tangent_drdt_x,
			(float)tangent_drdt_y,
			(float)tangent_drdt_z
		);
	}
}










template<typename T>
std::vector<T> gv_split_n(const std::string& line) {
	std::istringstream is(line);
	return std::vector<T>(std::istream_iterator<T>(is), std::istream_iterator<T>());
}







size_t ParallelTransportFrame::gv_read_lorenz_data_file()
{  
    //  My typical Lorenz Attractor data file has 60 thousand records. 
    
    ptf_curve_derivatives.resize(1 + ptf_axon_arc_density); 

#ifdef _DEBUG
	std::fstream gFStr("StrangeAttractor\\lorenz_debug.ghvdata", std::ios_base::in);
#else
	std::fstream gFStr("StrangeAttractor\\lorenz.ghvdata", std::ios_base::in);
#endif

	if (!gFStr.is_open()) { return 0; }

	std::string g_line = ""; 

	VHG_Axonodromal_Vertex    tmp_curve;

    uint32_t idxLoop = 0; 


	while (std::getline(gFStr, g_line))
	{
        //             Fields in data file:
        //      ==================================
        //      position_x, position_y, position_z
        //      elapsed_time
        //      dr/dt__x, dr/dt__y, dr/dt__z
        //      d2r/dt2__x, d2r/dt2__y; d2r/dt2__z
        //      ==================================


#if 3 == 4

        std::vector<double> fields_split = gv_split_n<double>(g_line);

		XMFLOAT3   tmp_position = XMFLOAT3( 
                static_cast<float>(fields_split[0]), 
                static_cast<float>(fields_split[1]), 
                static_cast<float>(fields_split[2])
        );

        float tmp_time = static_cast<float>(fields_split[3]);

        XMFLOAT3   tmp_drdt = XMFLOAT3( 
                static_cast<float>(fields_split[4]), 
                static_cast<float>(fields_split[5]), 
                static_cast<float>(fields_split[6])
        );

        XMFLOAT3   tmp_d2rdt2 = XMFLOAT3( 
                static_cast<float>(fields_split[7]), 
                static_cast<float>(fields_split[8]), 
                static_cast<float>(fields_split[9])
        );

		tmp_curve = { tmp_position, tmp_time, tmp_drdt, tmp_d2rdt2 };
#else

        XMFLOAT3   tmp_position; 
        float tmp_time; 
        XMFLOAT3   tmp_drdt; 
        XMFLOAT3   tmp_d2rdt2; 

        std::istringstream gSS(g_line); 

        gSS >> tmp_position.x >> tmp_position.y >> tmp_position.z >> tmp_time >> tmp_drdt.x >> tmp_drdt.y >> tmp_drdt.z >> tmp_d2rdt2.x >> tmp_d2rdt2.y >> tmp_d2rdt2.z;

		tmp_curve = { tmp_position, tmp_time, tmp_drdt, tmp_d2rdt2 };

#endif


        ptf_curve_derivatives.at(idxLoop) = tmp_curve;


        idxLoop++;

        if (idxLoop >= ptf_axon_arc_density)
        {
            break;
        }
	}
	
	gFStr.close(); //           CLOSE THE FILE !!!!! 

    return ptf_curve_derivatives.size();
}  




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++





void ParallelTransportFrame::HansonParallelTransportFrame()
{

    // TODO:   merge vectors  unit_tangent and transported_normal into one vector

    //  TODO: bug: this loop takes no account of the card of ptf_curve_derivatives vector...


    std::vector<XMVECTOR> *transported_normal = new std::vector<XMVECTOR>();


    std::vector<VHG_Vertex_PosTex>   *surface_points = new std::vector<VHG_Vertex_PosTex>;
    std::vector<VHG_Vertex_PosTex>   *spoke_lines = new std::vector<VHG_Vertex_PosTex>;
    std::vector<VHG_Vertex_PosTex>   *surface_triangles = new std::vector<VHG_Vertex_PosTex>;


    size_t record_count = gv_read_lorenz_data_file();


    //          
    //  Bootstrap: Special case when idx_nodes == 0: 
    //  ========================================
    //  Construct what Andrew J. Hanson terms the "initial normal vector V0": 
    //         
    //  Does Hanson's algorithm require the initial normal vector to be normalized???
    //   

    XMVECTOR axon_tangent_zero = XMLoadFloat3( &(ptf_curve_derivatives.at(0).axon_tangent_drdt) );

    XMVECTOR axon_r_position_zero = XMLoadFloat3( &(ptf_curve_derivatives.at(0).axon_position_r) );
    XMVECTOR axon_r_position_one  = XMLoadFloat3( &(ptf_curve_derivatives.at(1).axon_position_r) );

    XMVECTOR axon_delta_r = axon_r_position_one - axon_r_position_zero;

    //  TODO: ???   Regarding the vector cross product below: 
    //  TODO: ???   should i use the finite difference derivative "axon_tangent_zero" 
    //  TODO: ???   rather than the simple delta "axon_delta_r" ???

    XMVECTOR g_zero = XMVector3Cross(axon_r_position_zero, axon_delta_r);  

    XMVECTOR h_normalized_normal = XMVector3Normalize(XMVector3Cross(g_zero, axon_delta_r));

    transported_normal->push_back(h_normalized_normal);

    XMStoreFloat3(& (ptf_curve_derivatives.at(0).axon_normal), h_normalized_normal); // ghv: added 20190313; 


    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    //              End of bootstrap; 
    //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


    XMVECTOR nut_curr; 
    XMVECTOR nut_next = XMVector3Normalize(XMLoadFloat3(&(ptf_curve_derivatives.at(0).axon_tangent_drdt)));
    const float epsilon_length = 0.0001f;


    for (UINT idx_frame = 0; idx_frame < ptf_axon_arc_density; idx_frame++)
    {

        if (idx_frame < -1 + ptf_axon_arc_density)
        {
            nut_curr = nut_next;
            nut_next = XMVector3Normalize(XMLoadFloat3(&(ptf_curve_derivatives.at(1 + idx_frame).axon_tangent_drdt)));

            XMVECTOR B_vector = XMVector3Cross(nut_curr, nut_next);

            float B_length = XMVectorGetX(XMVector3Length(B_vector));
            XMVECTOR next_transported_normal;

            if (B_length < epsilon_length)
            {
                next_transported_normal = transported_normal->at(idx_frame);
            }
            else
            {
                XMVECTOR B_unit_vector = XMVector3Normalize(B_vector);
                XMVECTOR angle_theta_vector = XMVector3AngleBetweenNormals(nut_curr, nut_next);
                float angle_theta = XMVectorGetX(angle_theta_vector);
                XMMATRIX gv_rotation_matrix = XMMatrixRotationNormal(B_unit_vector, angle_theta);

                next_transported_normal = XMVector3Transform(
                    transported_normal->at(idx_frame),
                    gv_rotation_matrix
                );
            }
            transported_normal->push_back(next_transported_normal);

            XMStoreFloat3(&(ptf_curve_derivatives.at(1 + idx_frame).axon_normal), next_transported_normal); // ghv: added 20190313; 
        }



    // }
    // for (UINT idx_frame = 0; idx_frame < ptf_axon_arc_density; idx_frame++)
    // {



        XMVECTOR unit_normal = XMVector3Normalize(
            transported_normal->at(idx_frame)
        );

        //     In order to orient and position the cross-section, 
        //     need another normal vector in addition to the transported_normal.
        //     I have chosen to use vector B = T cross N.

        XMVECTOR binormal = XMVector3Cross(
            XMVector3Normalize(XMLoadFloat3(&(ptf_curve_derivatives.at(idx_frame).axon_tangent_drdt))), 
            unit_normal
        );

        XMStoreFloat3(& (ptf_curve_derivatives.at(idx_frame).axon_binormal), binormal); // ghv: added 20190315; 

        //   I expect that binormal doesn't need explicit normalization;
        //           
        //   The fact that node_unit_tangent and unit_normal are perpendicular
        //   implies angle = 90 degrees, thus sin(angle) = 1,
        //   thus magnitude of cross product is product of magnitudes,
        //   and each of those is known to be 1.

        for (uint32_t k = 0; k <= ptf_tube_facets; ++k)  //  HAZARD : loop upper limit is <= not < !!!!
        {
            //     Poloidal loop: 

            float t_fraction = k / (float)ptf_tube_facets;

            float angle_phi = t_fraction * 2 * XM_PI;

            float C_x = ptf_tube_radius * cosf(angle_phi);
            float C_y = ptf_tube_radius * sinf(angle_phi);

            float N_x = XMVectorGetX(unit_normal);
            float N_y = XMVectorGetY(unit_normal);
            float N_z = XMVectorGetZ(unit_normal);

            float B_x = XMVectorGetX(binormal);
            float B_y = XMVectorGetY(binormal);
            float B_z = XMVectorGetZ(binormal);

            float P_x = (ptf_curve_derivatives.at(idx_frame)).axon_position_r.x + C_x * N_x + C_y * B_x;
            float P_y = (ptf_curve_derivatives.at(idx_frame)).axon_position_r.y + C_x * N_y + C_y * B_y;
            float P_z = (ptf_curve_derivatives.at(idx_frame)).axon_position_r.z + C_x * N_z + C_y * B_z;

            VHG_Vertex_PosTex tmp_surface_point = { XMFLOAT3(P_x, P_y, P_z), XMFLOAT2(0.f, 0.f) };
            surface_points->push_back(tmp_surface_point);

            //  std::vector just for LINELIST topology to show radial segments
            //  from space curve out to points on the tube surface:

            VHG_Vertex_PosTex tmp_spoke;
            tmp_spoke.e_pos = (ptf_curve_derivatives.at(idx_frame)).axon_position_r;
            tmp_spoke.e_texco = XMFLOAT2(0.f, 0.f);
            spoke_lines->push_back(tmp_spoke);
            spoke_lines->push_back(tmp_surface_point);
        }
        // for each cross-section facet;
    }






    ptf_vertex_buffer_1_count = (uint32_t)spoke_lines->size();
    
    Create_Vertex_Buffer(
        spoke_lines,
        ptf_vertex_buffer_1_buffer.ReleaseAndGetAddressOf() 
    );  // use topology = LINELIST;
    
    //     Expected cardinality of std::vector surface_points = ptf_axon_arc_density * (1 + tube_facets).

    VHG_Vertex_PosTex quad_top_left;
    VHG_Vertex_PosTex quad_top_right;
    VHG_Vertex_PosTex quad_bottom_right;
    VHG_Vertex_PosTex quad_bottom_left;

    for (uint32_t i_axial = 0; i_axial < -1 + ptf_axon_arc_density; i_axial++)
    {
        for (uint32_t i_poloidal = 0; i_poloidal < ptf_tube_facets; i_poloidal++)
        {
            //   set the texture coordinates: 

            uint32_t ii = i_poloidal + (ptf_tube_facets + 1) * i_axial;

            quad_top_left = { surface_points->at(0 + ii).e_pos, XMFLOAT2(0.f, 1.f) };
            quad_top_right = { surface_points->at((ptf_tube_facets + 1) + ii).e_pos, XMFLOAT2(0.f, 0.f) };
            quad_bottom_right = { surface_points->at((ptf_tube_facets + 2) + ii).e_pos, XMFLOAT2(1.f, 0.f) };
            quad_bottom_left = { surface_points->at(1 + ii).e_pos, XMFLOAT2(1.f, 1.f) };

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


    ptf_vertex_buffer_2_count = (uint32_t)surface_triangles->size(); 

    Create_Vertex_Buffer(
        surface_triangles,
        ptf_vertex_buffer_2_buffer.ReleaseAndGetAddressOf() 
    );  // use topology = TRIANGLELIST;




    //   TODO:   need an Index Buffer


    delete surface_triangles; surface_triangles = nullptr;
    delete spoke_lines; spoke_lines = nullptr;
    delete surface_points; surface_points = nullptr;
    delete transported_normal; transported_normal = nullptr;
}  







void ParallelTransportFrame::Create_Rasterizer_State(void)
{
    D3D11_RASTERIZER_DESC raster_desc;
    ZeroMemory(&raster_desc, sizeof(raster_desc));

    raster_desc.FillMode = this->ptf_rasterizer_fill_mode;
    raster_desc.CullMode = D3D11_CULL_NONE;
    raster_desc.FrontCounterClockwise = FALSE;

    raster_desc.DepthBias = 0;
    raster_desc.SlopeScaledDepthBias = 0.0f;
    raster_desc.DepthBiasClamp = 0.0f;

    raster_desc.DepthClipEnable = TRUE;
    raster_desc.ScissorEnable = FALSE;

    raster_desc.MultisampleEnable = FALSE;
    raster_desc.AntialiasedLineEnable = FALSE;

    DX::ThrowIfFailed(
        this->m_deviceResources->GetD3DDevice()->CreateRasterizerState(
            &raster_desc,
            ptf_rasterizer_state.ReleaseAndGetAddressOf()
        )
    );
}  
//  Closes ParallelTransportFrame::Create_Rasterizer_State; 







void ParallelTransportFrame::Create_Input_Layout(const std::vector<byte>& p_byte_vector)
{
    static const D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateInputLayout(
            vertexDesc,
            ARRAYSIZE(vertexDesc),
            &p_byte_vector[0], 
            p_byte_vector.size(), 
            &ptf_inputLayout
        )
    );
}  
// Closes ParallelTransportFrame::Create_Input_Layout(); 







void ParallelTransportFrame::CreateDeviceDependentResources()
{ 
    //      
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //                 Load Textures from Image Files          
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //  

    // unused this->Load_Texture_Triax();


    //      
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //                     Create a SamplerState 
    //               to be used inside the PTF Loft Pixel Shader           
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //  

    /*
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(sampDesc));
    sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; // undo: revert to wrap;
    sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

    DX::ThrowIfFailed(
        m_deviceResources->GetD3DDevice()->CreateSamplerState(
                &sampDesc,
                ptf_loft_texture_sampler_state.ReleaseAndGetAddressOf()
        )
    );
    */
    

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  


    Create_Rasterizer_State();


    //      
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //                Load shaders asynchronously 
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //  

    auto loadVSTask = DX::ReadDataAsync(L"PTF_VertexShader.cso");
    auto loadPSTask = DX::ReadDataAsync(L"PTF_PixelShader.cso");

    //      
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //                   Create VS Vertex Shader 
    //                   Create Input Layout Object              
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //  

    auto createVSTask = loadVSTask.then([this](const std::vector<byte>& fileData)
    {
            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreateVertexShader(
                    &fileData[0],
                    fileData.size(),
                    nullptr,
                    &ptf_vertexShader
                )
            );

            this->Create_Input_Layout(fileData);
    });


    //      
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //                        Create PS Pixel Shader 
    //                   Create the WVP Constant Buffer               
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++  
    //  

    auto createPSTask = loadPSTask.then([this](const std::vector<byte>& fileData)
    {
            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreatePixelShader(
                    &fileData[0],
                    fileData.size(),
                    nullptr,
                    &ptf_pixelShader )
            );

            CD3D11_BUFFER_DESC constantBufferDesc(
                sizeof(VHG_ConBuf_MVP_Struct),
                D3D11_BIND_CONSTANT_BUFFER
            );

            static_assert(
                (sizeof(VHG_ConBuf_MVP_Struct) % 16) == 0,
                "Constant Buffer struct must be 16-byte aligned"
                );

            DX::ThrowIfFailed(
                m_deviceResources->GetD3DDevice()->CreateBuffer(
                    &constantBufferDesc,
                    nullptr,
                    &ptf_WVP_Buffer )
            );
    });


    auto createCubeTask = (createPSTask && createVSTask).then([this]()
    {
            this->HansonParallelTransportFrame();
    });


    createCubeTask.then([this]()
    {
            this->ptf_loadingComplete = true;
    });

}  
//  Closes ParallelTransportFrame::CreateDeviceDependentResources()




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


void ParallelTransportFrame::ReleaseDeviceDependentResources()
{
    ptf_loadingComplete = false; 

    ptf_WVP_Buffer.Reset(); 

}  
//  Closes  ParallelTransportFrame::ReleaseDeviceDependentResources; 




//                 ...file ends... 

