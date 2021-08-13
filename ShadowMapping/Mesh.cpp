#include "stdafx.h"

#include "Mesh.h"

std::unordered_map<std::string, DescriptorDataType> Mesh::type_map{};

void Mesh::loadMeshFromFile(const char* filename)
{
	loadMeshFromFile(filename, std::unordered_map<const char*, const char*>{});
}

void Mesh::loadMeshFromFile(const char* filename, std::unordered_map<const char*, const char*> relabelling)
{
	// Open the file
	std::ifstream fs{ filename };

	if (!fs.is_open()) {
		std::string err = "Failed to open file '" + std::string(filename) + "'.";
		throw std::exception(err.c_str());
	}

	std::string buffer;

	// Skip the first line
	std::getline(fs, buffer);

	// Now read data labels and formats
	std::vector<ElementDescriptor> fileElementDescriptors;

	getline(fs, buffer);

	std::stringstream descriptorString{ buffer };

	while (descriptorString >> buffer) {
		ElementDescriptor e;	
		{
			auto it = relabelling.find(buffer.c_str());
			if (it != relabelling.end()) {
				e.label = it->second;
			}
			else {
				e.label = buffer;
			}
		}

		std::string typeString;
		descriptorString >> typeString;
		auto it = type_map.find(typeString);

		if (it == type_map.end()) {
			throw std::exception((std::string("Unsupported type in vertex data found in file ") + filename + ".").c_str());
		}

		e.type = it->second;

		descriptorString >> e.size;

		if (e.size > 4) {
			throw std::exception((std::string("Unsupported size in vertex data found in file ") + filename + ".").c_str());
		}

		fileElementDescriptors.push_back(e);
	}

	std::vector<char*> vertices;

	UINT index;

	fs >> buffer;
	while (buffer == "v") {
		// Add vertex data to mesh object
		char* vertex = new char[m_vertexSize];
		
		UINT32 offset = 0;

		for (ElementDescriptor file_desc : fileElementDescriptors) {
			bool keep_data = false;
			ElementDescriptor obj_desc;
			for (UINT32 i = 0; i < m_numberOfDescriptors; ++i) {
				if (m_elementDescriptors[i].label == file_desc.label) {					
					obj_desc = m_elementDescriptors[i];
					keep_data = true;
					break;
				}
			}

			if (!keep_data) {
				std::string temp;
				for (int i = 0; i < file_desc.size; ++i) {
					fs >> temp;
				}
				continue;
			}

			offset = obj_desc.offset;
			for (UINT32 i = 0; i < min(file_desc.size, obj_desc.size); ++i) {
				switch (obj_desc.type) {
				case _FLOAT64:
					fs >> *(double*)(vertex + offset); break;
				case _FLOAT32:
					fs >> *(float*)(vertex + offset); break;

				case _INT64:
					fs >> *(INT64*)(vertex + offset); break;
				case _INT32:
					fs >> *(INT32*)(vertex + offset); break;
				case _INT16:
					fs >> *(INT16*)(vertex + offset); break;
				case _INT8:
					fs >> *(INT8*)(vertex + offset); break;

				case _UINT64:
					fs >> *(UINT64*)(vertex + offset); break;
				case _UINT32:
					fs >> *(UINT32*)(vertex + offset); break;
				case _UINT16:
					fs >> *(UINT16*)(vertex + offset); break;
				case _UINT8:
					fs >> *(UINT8*)(vertex + offset); break;
				}

				offset += getDataSize(obj_desc.type);
			}
		}

		vertices.push_back(vertex);

		buffer = "";
		fs >> buffer;
	}

	m_numberOfVertices = vertices.size();
	m_vertices = new char[m_vertexSize * m_numberOfVertices];
	for (UINT32 i = 0; i < m_numberOfVertices; ++i) {
		memcpy(m_vertices + i * m_vertexSize, vertices[i], m_vertexSize);
	}

	(void) vertices.empty();

	std::vector<UINT32> indices;

	while (buffer == "f") {
		// Add face data to mesh object
		UINT32 v1, v2, v3;
		fs >> v1 >> v2 >> v3;

		indices.push_back(v1);
		indices.push_back(v2);
		indices.push_back(v3);

		buffer = "";
		fs >> buffer;
	}

	m_numberOfIndices = indices.size();
	m_indices = new UINT32[m_numberOfIndices];
	memcpy(m_indices, indices.data(), sizeof(UINT32) * indices.size());
}

void Mesh::initAsGrid(UINT width, UINT height, float horizontalSpacing, float verticalSpacing, float horizontalOffset, float verticalOffset)
{
	m_numberOfVertices = width * height;
	m_numberOfIndices = (width - 1) * (height - 1) * 6;

	m_vertices = new char[m_vertexSize * m_numberOfVertices];
	m_indices = new UINT32[m_numberOfIndices];

	ElementDescriptor *positionDescriptor = nullptr, *normalDescriptor = nullptr, *textureDescriptor = nullptr;
	for (UINT i = 0; i < m_numberOfDescriptors; ++i) {
		if (m_elementDescriptors[i].label == "position") {
			positionDescriptor = m_elementDescriptors + i;
		}
		else if(m_elementDescriptors[i].label == "normal"){
			normalDescriptor = m_elementDescriptors + i;
		}
		else if (m_elementDescriptors[i].label == "texture") {
			textureDescriptor = m_elementDescriptors + i;
		}
	}

	for (UINT32 x = 0; x < width; ++x) {
		for (UINT32 y = 0; y < width; ++y) {
			if (positionDescriptor != nullptr) {
				UINT offset = 0;
				char* data = new char[4 * getDataSize(positionDescriptor->type)];
				convertData(positionDescriptor->type, x * horizontalSpacing + horizontalOffset, data + offset);
				offset += getDataSize(positionDescriptor->type);
				convertData(positionDescriptor->type, y * verticalSpacing + verticalOffset, data + offset);
				offset += getDataSize(positionDescriptor->type);
				convertData(positionDescriptor->type, 0.f, data + offset);
				offset += getDataSize(positionDescriptor->type);
				convertData(positionDescriptor->type, 1.f, data + offset);

				memcpy(m_vertices + m_vertexSize * (x + y * width) + positionDescriptor->offset,
					data, positionDescriptor->size * getDataSize(positionDescriptor->type));

				delete[] data;
			}

			if (normalDescriptor != nullptr) {
				UINT offset = 0;
				char* data = new char[4 * getDataSize(normalDescriptor->type)];
				convertData(normalDescriptor->type, 0.f, data + offset);
				offset += getDataSize(normalDescriptor->type);
				convertData(normalDescriptor->type, 0.f, data + offset);
				offset += getDataSize(normalDescriptor->type);
				convertData(normalDescriptor->type, 1.f, data + offset);
				offset += getDataSize(normalDescriptor->type);
				convertData(normalDescriptor->type, 0.f, data + offset);

				memcpy(m_vertices + m_vertexSize * (x + y * width) + normalDescriptor->offset,
					data, normalDescriptor->size * getDataSize(normalDescriptor->type));

				delete[] data;
			}

			if (textureDescriptor != nullptr) {
				UINT offset = 0;
				char* data = new char[2 * getDataSize(textureDescriptor->type)];
				convertData(textureDescriptor->type, (double) x / (width-1), data + offset);
				offset += getDataSize(textureDescriptor->type);
				convertData(textureDescriptor->type, (double) (height-y-1) / (height-1), data + offset);

				memcpy(m_vertices + m_vertexSize * (x + y * width) + textureDescriptor->offset,
					data, textureDescriptor->size * getDataSize(textureDescriptor->type));

				delete[] data;
			}
		}
	}

	UINT32 index = 0;
	for (UINT32 x = 0; x < width - 1; ++x) {
		for (UINT32 y = 0; y < height - 1; ++y) {
			// Top left triangle
			m_indices[index] = x + width * y;
			m_indices[index + 1] = x + width * (y + 1);
			m_indices[index + 2] = (x + 1) + width * y;

			// Bottom right triangle
			m_indices[index + 3] = (x + 1) + width * y;
			m_indices[index + 4] = x + width * (y + 1);
			m_indices[index + 5] = (x + 1) + width * (y + 1);

			index += 6;
		}
	}
}

void Mesh::scheduleUpload(ComPtr<ID3D12Device> m_device, ComPtr<ID3D12GraphicsCommandList> m_commandList) {
	// Create the vertex buffer.
	auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	{
		auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_vertexSize * m_numberOfVertices);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer)));

		ThrowIfFailed(m_device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&vertexBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&vertexBufferUploadHeap)));

		NAME_D3D12_OBJECT(m_vertexBuffer);

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the vertex buffer.
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = m_vertices;
		vertexData.RowPitch = m_vertexSize * m_numberOfVertices;
		vertexData.SlicePitch = vertexData.RowPitch;

		auto transitionDesc = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

		UpdateSubresources<1>(m_commandList.Get(), m_vertexBuffer.Get(), vertexBufferUploadHeap.Get(), 0, 0, 1, &vertexData);
		m_commandList->ResourceBarrier(1, &transitionDesc);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = m_vertexSize;
		m_vertexBufferView.SizeInBytes = m_vertexSize * m_numberOfVertices;
	}

	// Create the index buffer.
	{
		auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_numberOfIndices * sizeof(UINT32));

		ThrowIfFailed(m_device->CreateCommittedResource(
			&defaultHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&indexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&m_indexBuffer)));

		ThrowIfFailed(m_device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&indexBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&indexBufferUploadHeap)));

		NAME_D3D12_OBJECT(m_indexBuffer);

		// Copy data to the intermediate upload heap and then schedule a copy 
		// from the upload heap to the index buffer.
		D3D12_SUBRESOURCE_DATA indexData = {};
		indexData.pData = m_indices;
		indexData.RowPitch = m_numberOfIndices * sizeof(UINT32);
		indexData.SlicePitch = indexData.RowPitch;

		auto transitionDesc = CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

		UpdateSubresources<1>(m_commandList.Get(), m_indexBuffer.Get(), indexBufferUploadHeap.Get(), 0, 0, 1, &indexData);
		m_commandList->ResourceBarrier(1, &transitionDesc);

		// Describe the index buffer view.
		m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
		m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		m_indexBufferView.SizeInBytes = m_numberOfIndices * sizeof(UINT32);
	}
}

void Mesh::draw(ComPtr<ID3D12GraphicsCommandList> commandList) {
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);

	commandList->DrawIndexedInstanced(m_numberOfIndices, 1, 0, 0, 0);
}