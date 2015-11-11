#include "UrsinePrecompiled.h"
#include "ModelManager.h"
#include "GfxDefines.h"
#include "VertexDefinitions.h"
#include <d3d11.h>

namespace ursine
{
    namespace graphics
    {
        void ModelManager::Initialize(ID3D11Device *device, ID3D11DeviceContext *context, std::string filePath)
        {
            m_device = device;
            m_deviceContext = context;

            m_modelCount = 0;
            m_currentState = -1;

            //loading all models
            char buffer[ 512 ];
            std::ifstream input;
            std::string fileText = filePath;
            fileText.append("MODELS.8.0.gfx");
            input.open(fileText, std::ios_base::in);

            UAssert(input.is_open(), "Failed to open file for model loading! ('%s')", filePath.c_str());
            while (input.eof() == false)
            {
                //zero it out
                memset(buffer, 0, sizeof(char) * 512);

                //get the line
                input.getline(buffer, 512);

                //if nothing on line, or # comment, continue;
                if (buffer[ 0 ] == '#' || strlen(buffer) == 0)
                    continue;

                //use string, and vector for holding tokens
                std::string data(buffer);
                std::vector<std::string> tokens;

                //deal with data, chop it up by space
                size_t pos = data.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ", 0);
                while (pos != std::string::npos)
                {
                    size_t end = data.find_first_of(" ", pos + 1);
                    if (end - pos > 1)
                        tokens.push_back(data.substr(pos, end - pos + 1));
                    pos = data.find_first_of(" ", end);
                }

                //0 is filename
                tokens[ 0 ].insert(0, filePath);
                tokens[ 1 ].erase(0, 1);
                //1 is name

				//determine what loading format to use
				if (tokens[0].find("8.0.mdl") != std::string::npos)
					LoadModel(tokens[1], tokens[0]);
				else
					LoadModel_Ursine(tokens[1], tokens[0]);
            }

            input.close();

            std::vector<AnimationVertex> temp;
            temp.resize(6);

            temp[ 0 ].vPos = DirectX::XMFLOAT3(-0.5, -0.5, 0.5);
            temp[ 0 ].vTexcoord = DirectX::XMFLOAT2(1, 1);
            temp[ 0 ].vNor = DirectX::XMFLOAT3(0, 0, 0);

            temp[ 1 ].vPos = DirectX::XMFLOAT3(0.5, -0.5, 0.5);
            temp[ 1 ].vTexcoord = DirectX::XMFLOAT2(0, 1);
            temp[ 1 ].vNor = DirectX::XMFLOAT3(0, 0, 0);

            temp[ 2 ].vPos = DirectX::XMFLOAT3(0.5, 0.5, 0.5);
            temp[ 2 ].vTexcoord = DirectX::XMFLOAT2(0, 0);
            temp[ 2 ].vNor = DirectX::XMFLOAT3(0, 0, 0);

            temp[ 3 ].vPos = DirectX::XMFLOAT3(0.5, 0.5, 0.5);
            temp[ 3 ].vTexcoord = DirectX::XMFLOAT2(0, 0);
            temp[ 3 ].vNor = DirectX::XMFLOAT3(0, 0, 0);

            temp[ 4 ].vPos = DirectX::XMFLOAT3(-0.5, 0.5, 0.5);
            temp[ 4 ].vTexcoord = DirectX::XMFLOAT2(1, 0);
            temp[ 4 ].vNor = DirectX::XMFLOAT3(0, 0, 0);

            temp[ 5 ].vPos = DirectX::XMFLOAT3(-0.5, -0.5, 0.5);
            temp[ 5 ].vTexcoord = DirectX::XMFLOAT2(1, 1);
            temp[ 5 ].vNor = DirectX::XMFLOAT3(0, 0, 0);

            std::string name = "internalQuad";

            m_modelArray[ name ] = new ModelResource();

            D3D11_BUFFER_DESC vertexBufferDesc;
            D3D11_SUBRESOURCE_DATA vertexData;
            HRESULT result;

            //Set up the description of the static vertex buffer.
            vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            vertexBufferDesc.ByteWidth = sizeof(AnimationVertex) * 6;
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = 0;
            vertexBufferDesc.MiscFlags = 0;
            vertexBufferDesc.StructureByteStride = 0;

            //Give the subresource structure a pointer to the vertex data.
            vertexData.pSysMem = &temp[ 0 ];
            vertexData.SysMemPitch = 0;
            vertexData.SysMemSlicePitch = 0;

            //Now create the vertex buffer.
            result = m_device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_modelArray[ name ]->Vertices_);
            UAssert(result == S_OK, "Failed to make vertex buffer!");
            m_modelArray[ name ]->VertCount_ = 6;

            m_modelArray[ name ]->IndexCount_ = 6;
            unsigned *indexArray = new unsigned[ m_modelArray[ name ]->IndexCount_ ];

            for (unsigned x = 0; x < 6; ++x)
                indexArray[ x ] = x;

            D3D11_BUFFER_DESC indexBufferDesc;
            D3D11_SUBRESOURCE_DATA indexData;

            //Set up the description of the static index buffer.
            indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            indexBufferDesc.ByteWidth = sizeof(unsigned) * m_modelArray[ name ]->IndexCount_;
            indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            indexBufferDesc.CPUAccessFlags = 0;
            indexBufferDesc.MiscFlags = 0;
            indexBufferDesc.StructureByteStride = 0;

            //Give the subresource structure a pointer to the index data.
            indexData.pSysMem = indexArray;
            indexData.SysMemPitch = 0;
            indexData.SysMemSlicePitch = 0;

            //Create the index buffer.
            result = m_device->CreateBuffer(&indexBufferDesc, &indexData, &m_modelArray[ name ]->Indices_);
            UAssert(result == S_OK, "Failed to make index buffer!");

            m_s2uTable[ name ] = m_modelCount;
            m_u2mTable[ m_modelCount++ ] = m_modelArray[ name ];
        }

        void ModelManager::Uninitialize()
        {
            for (auto &x : m_modelArray)
            {
                if (x.second == nullptr)
                    continue;
                RELEASE_RESOURCE(x.second->Vertices_);
                RELEASE_RESOURCE(x.second->Indices_);

                delete x.second;
            }

            m_device = nullptr;
            m_deviceContext = nullptr;
        }

        void ModelManager::LoadModel(std::string name, std::string fileName)
        {
            UAssert(m_modelArray[ name ] == nullptr, "Model with name '%' has already been loaded (new source file '%s')", name.c_str(), fileName.c_str());

            std::ifstream input;
            std::vector<DiffuseTextureVertex> buffer;

            input.open(fileName, std::ios::in | std::ios::binary);

            UAssert(input.is_open() == true, "Failed to open model '%s'", fileName.c_str());

            if (!input.is_open())
                return;

            /////////////////////////////////////////////////////////////////
            // LOAD FILE INTO MEMORY ////////////////////////////////////////
            char size[ 32 ];
            input.getline(size, 32);
            unsigned vertCount = atoi(size);

            UAssert(vertCount > 0, "Mesh '%s' has 0 vertices!", fileName.c_str());

            buffer.resize(vertCount);
            input.read((char*)&buffer[ 0 ], sizeof(DiffuseTextureVertex) * vertCount);

            /////////////////////////////////////////////////////////////////
            // ALLOCATE MODEL ///////////////////////////////////////////////
            m_modelArray[ name ] = new ModelResource();

            /////////////////////////////////////////////////////////////////
            // CREATE VERTEX BUFFER /////////////////////////////////////////
            D3D11_BUFFER_DESC vertexBufferDesc;
            D3D11_SUBRESOURCE_DATA vertexData;
            HRESULT result;

            //Set up the description of the static vertex buffer.
            vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            vertexBufferDesc.ByteWidth = sizeof(DiffuseTextureVertex) * vertCount;
            vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            vertexBufferDesc.CPUAccessFlags = 0;
            vertexBufferDesc.MiscFlags = 0;
            vertexBufferDesc.StructureByteStride = 0;

            //Give the subresource structure a pointer to the vertex data.
            vertexData.pSysMem = &buffer[ 0 ];
            vertexData.SysMemPitch = 0;
            vertexData.SysMemSlicePitch = 0;

            //Now create the vertex buffer.
            result = m_device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_modelArray[ name ]->Vertices_);
            UAssert(result == S_OK, "Failed to make vertex buffer!");
            m_modelArray[ name ]->VertCount_ = vertCount;

            /////////////////////////////////////////////////////////////////
            // CREATE INDEX BUFFER //////////////////////////////////////////
            m_modelArray[ name ]->IndexCount_ = vertCount;
            unsigned *indexArray = new unsigned[ m_modelArray[ name ]->IndexCount_ ];

            for (unsigned x = 0; x < vertCount; ++x)
                indexArray[ x ] = x;

            D3D11_BUFFER_DESC indexBufferDesc;
            D3D11_SUBRESOURCE_DATA indexData;

            //Set up the description of the static index buffer.
            indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
            indexBufferDesc.ByteWidth = sizeof(unsigned) * m_modelArray[ name ]->IndexCount_;
            indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
            indexBufferDesc.CPUAccessFlags = 0;
            indexBufferDesc.MiscFlags = 0;
            indexBufferDesc.StructureByteStride = 0;

            //Give the subresource structure a pointer to the index data.
            indexData.pSysMem = indexArray;
            indexData.SysMemPitch = 0;
            indexData.SysMemSlicePitch = 0;

            //Create the index buffer.
            result = m_device->CreateBuffer(&indexBufferDesc, &indexData, &m_modelArray[ name ]->Indices_);
            UAssert(result == S_OK, "Failed to make index buffer!");

            m_s2uTable[ name ] = m_modelCount;
            m_u2mTable[ m_modelCount++ ] = m_modelArray[ name ];

            delete[] indexArray;
        }

		void ModelManager::LoadModel_Ursine(std::string name, std::string fileName)
		{
			UAssert(m_modelArray[name] == nullptr, "Model with name '%' has already been loaded (new source file '%s')", name.c_str(), fileName.c_str());

			std::ifstream input;
			std::vector<AnimationVertex> buffer;

			// this is wrong
			HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			ufmt_loader::ModelInfo ufmt_model;
			// Serialize in model ////////////////////////////////////////
			ufmt_model.SerializeIn(hFile);

			/////////////////////////////////////////////////////////////////
			// CREATE VERTEX BUFFER /////////////////////////////////////////
			D3D11_BUFFER_DESC vertexBufferDesc;
			D3D11_SUBRESOURCE_DATA vertexData;
			HRESULT result;

			for (uint mesh_idx = 0; mesh_idx < ufmt_model.mmeshCount; ++mesh_idx)
			{
				/////////////////////////////////////////////////////////////////
				// ALLOCATE MODEL ///////////////////////////////////////////////
				m_modelArray[name] = new ModelResource();

				uint vertCount = ufmt_model.marrMeshes[mesh_idx].vertexCount;
				//Set up the description of the static vertex buffer.
				vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
				vertexBufferDesc.ByteWidth = sizeof(AnimationVertex) * vertCount;
				vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
				vertexBufferDesc.CPUAccessFlags = 0;
				vertexBufferDesc.MiscFlags = 0;
				vertexBufferDesc.StructureByteStride = 0;

				//Give the subresource structure a pointer to the vertex data. - need layout_type to determine if static or skinned
				//can do this with skincount
				buffer.resize(vertCount);
				for (size_t i = 0; i < vertCount; ++i)
				{
					buffer[i].vPos = ufmt_model.marrMeshes[mesh_idx].vertices[i];
					buffer[i].vNor = ufmt_model.marrMeshes[mesh_idx].normals[i];
					buffer[i].vTexcoord = ufmt_model.marrMeshes[mesh_idx].uvs[i];
					if (ufmt_model.marrMeshes[mesh_idx].ctrlPtCount > 0)
					{
						buffer[i].vBWeight.x = ufmt_model.marrMeshes[mesh_idx].ctrlBlendWeights[i][0];
						buffer[i].vBWeight.y = ufmt_model.marrMeshes[mesh_idx].ctrlBlendWeights[i][1];
						buffer[i].vBWeight.z = ufmt_model.marrMeshes[mesh_idx].ctrlBlendWeights[i][2];
						buffer[i].vBWeight.w = ufmt_model.marrMeshes[mesh_idx].ctrlBlendWeights[i][3];
						buffer[i].vBIdx.x = ufmt_model.marrMeshes[mesh_idx].ctrlIndices[i][0];
						buffer[i].vBIdx.y = ufmt_model.marrMeshes[mesh_idx].ctrlIndices[i][1];
						buffer[i].vBIdx.z = ufmt_model.marrMeshes[mesh_idx].ctrlIndices[i][2];
						buffer[i].vBIdx.w = ufmt_model.marrMeshes[mesh_idx].ctrlIndices[i][3];
					}
					else
					{
						buffer[i].vBWeight = DirectX::XMFLOAT4(0, 0, 0, 1);
						buffer[i].vBIdx = DirectX::XMINT4(0, 0, 0, 0);
					}
				}
				 
				//Give the subresource structure a pointer to the vertex data.
				vertexData.pSysMem = &buffer[0];
				vertexData.SysMemPitch = 0;
				vertexData.SysMemSlicePitch = 0;

				//Now create the vertex buffer.
				result = m_device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_modelArray[name]->Vertices_);
				UAssert(result == S_OK, "Failed to make vertex buffer!");
				m_modelArray[name]->VertCount_ = vertCount;

				/////////////////////////////////////////////////////////////////
				// CREATE INDEX BUFFER //////////////////////////////////////////
				m_modelArray[name]->IndexCount_ = ufmt_model.marrMeshes[mesh_idx].indexCount;
				unsigned *indexArray = new unsigned[m_modelArray[name]->IndexCount_];
				for (unsigned x = 0; x < m_modelArray[name]->IndexCount_; ++x)
					indexArray[x] = ufmt_model.marrMeshes[mesh_idx].indices[x];

				D3D11_BUFFER_DESC indexBufferDesc;
				D3D11_SUBRESOURCE_DATA indexData;

				//Set up the description of the static index buffer.
				indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
				indexBufferDesc.ByteWidth = sizeof(unsigned) * m_modelArray[name]->IndexCount_;
				indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
				indexBufferDesc.CPUAccessFlags = 0;
				indexBufferDesc.MiscFlags = 0;
				indexBufferDesc.StructureByteStride = 0;

				//Give the subresource structure a pointer to the index data.
				indexData.pSysMem = indexArray;
				indexData.SysMemPitch = 0;
				indexData.SysMemSlicePitch = 0;

				//Create the index buffer.
				result = m_device->CreateBuffer(&indexBufferDesc, &indexData, &m_modelArray[name]->Indices_);
				UAssert(result == S_OK, "Failed to make index buffer!");

				m_s2uTable[name] = m_modelCount;
				m_u2mTable[m_modelCount++] = m_modelArray[name];

				delete[] indexArray;
			}
		}

        ID3D11Buffer *ModelManager::GetModelVert(std::string name)
        {
            if (m_modelArray[ name ] == nullptr)
                return nullptr;
            return m_modelArray[ name ]->Vertices_;
        }

        unsigned ModelManager::GetModelVertcount(std::string name)
        {
            if (m_modelArray[ name ] == nullptr)
                return -1;
            return m_modelArray[ name ]->VertCount_;
        }

		unsigned ModelManager::GetModelIndexcount(std::string name)
		{
			if (m_modelArray[name] == nullptr)
				return -1;
			return m_modelArray[name]->IndexCount_;
		}

        void ModelManager::BindModel(std::string name)
        {


            ModelResource *model = m_modelArray[ name ];

            UAssert(model != nullptr, "Failed to bind model '%s'", name.c_str());

            //map mesh
            unsigned int strides = sizeof(AnimationVertex);
            unsigned int offset = 0;

            m_deviceContext->IASetVertexBuffers(0, 1, &model->Vertices_, &strides, &offset);
            m_deviceContext->IASetIndexBuffer(model->Indices_, DXGI_FORMAT_R32_UINT, 0);

            m_currentState = m_s2uTable[ name ];
        }

        void ModelManager::BindModel(unsigned ID)
        {


            ModelResource *model = m_u2mTable[ ID ];

            UAssert(model != nullptr, "Failed to bind model ID:%i", ID);

            //map mesh
            unsigned int strides = sizeof(AnimationVertex);
            unsigned int offset = 0;

            m_deviceContext->IASetVertexBuffers(0, 1, &model->Vertices_, &strides, &offset);
            m_deviceContext->IASetIndexBuffer(model->Indices_, DXGI_FORMAT_R32_UINT, 0);

            m_currentState = ID;
        }

        unsigned ModelManager::GetModelIDByName(std::string name)
        {
            return m_s2uTable[ name ];
        }

        ID3D11Buffer *ModelManager::GetModelVertByID(unsigned ID)
        {
            return m_u2mTable[ ID ]->Vertices_;
        }
        unsigned ModelManager::GetModelVertcountByID(unsigned ID)
        {
            return m_u2mTable[ ID ]->VertCount_;
        }

		unsigned ModelManager::GetModelIndexcountByID(unsigned ID)
		{
			return m_u2mTable[ID]->IndexCount_;
		}

        void ModelManager::Invalidate()
        {
            m_currentState = -1;
        }
    }
}