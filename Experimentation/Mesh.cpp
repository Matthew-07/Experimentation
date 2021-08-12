#include "stdafx.h"

#include "Mesh.h"

//Mesh::Mesh(const char* path)

void Mesh::unload() {
    if (vertices != nullptr) {
        delete[numberOfVertices] vertices;
        vertices = nullptr;
    }

    if (indices != nullptr) {
        delete[numberOfIndices] indices;
        indices = nullptr;
    }
}

void Mesh::loadFromFile(const char* path)
{
    unload();

    std::ifstream fs(path);

    if (!fs.is_open()) {
        throw - 1;
    }

    std::string buffer;

    for (int i = 0; i < 3; i++) {
        std::getline(fs, buffer);
    }

    UINT numberOfPositions = 0;
    UINT numberOfTexCoords = 0;
    UINT numberOfNormals = 0;
    numberOfIndices = 0;

    while (fs >> buffer) {

        if (buffer == "v") {
            numberOfPositions++;
        }
        else if (buffer == "vt") {
            numberOfTexCoords++;
        }
        else if (buffer == "vn") {
            numberOfNormals++;
        }
        else if (buffer == "f") {
            numberOfIndices += 3;
        }

        std::getline(fs, buffer);
    }

    XMFLOAT3* positions = new XMFLOAT3[numberOfPositions];
    XMFLOAT2* texCoords = new XMFLOAT2[numberOfTexCoords];
    XMFLOAT3* normals = new XMFLOAT3[numberOfNormals];

    indices = new UINT32[numberOfIndices];

    fs.clear();
    fs.seekg(0, fs.beg);

    for (int i = 0; i < 3; i++) {
        std::getline(fs, buffer);
    }

    for (int i = 0; i < numberOfPositions; i++) {
        fs >> buffer;
        fs >> positions[i].x >> positions[i].y >> positions[i].z;
    }
    for (int i = 0; i < numberOfTexCoords; i++) {
        fs >> buffer;
        fs >> texCoords[i].x >> texCoords[i].y;
    }
    for (int i = 0; i < numberOfNormals; i++) {
        fs >> buffer;
        fs >> normals[i].x >> normals[i].y >> normals[i].z;
    }

    for (int i = 0; i < 2; i++) {
        std::getline(fs, buffer);
    }

    struct IndexSet {
        UINT pos;
        UINT tex;
        UINT norm;
    };

    std::vector<IndexSet> indexSets;
    std::vector<UVNormVertex> verticesVector;

    UINT index = 0;

    while (fs >> buffer) {
        if (buffer == "f") {
            for (int x = 0; x < 3; x++) {
                IndexSet set;
                char temp;
                fs >> set.pos >> temp >> set.tex >> temp >> set.norm;
                set.pos--; set.tex--; set.norm--;

                bool setExists = false;

                for (int i = 0; i < indexSets.size(); i++) {
                    //if (set.pos == indexSets[i].pos && set.norm == indexSets[i].norm) {
                    if (set.pos == indexSets[i].pos && set.norm == indexSets[i].norm && set.tex == indexSets[i].tex) {
                        setExists = true;
                        indices[index] = i;
                        break;
                    }
                }

                if (!setExists) {
                    indexSets.push_back(set);
                    UVNormVertex v;
                    v.position = positions[set.pos];
                    v.uv = texCoords[set.tex];
                    v.normal = normals[set.norm];
                    indices[index] = verticesVector.size();
                    verticesVector.push_back(v);
                }

                index++;
            }
        }
        else break;
    }

    delete[numberOfPositions] positions;
    delete[numberOfTexCoords] texCoords;
    delete[numberOfNormals] normals;

    numberOfVertices = verticesVector.size();
    vertices = new UVNormVertex[numberOfVertices];

    for (int v = 0; v < numberOfVertices; v++) {
        vertices[v] = verticesVector[v];
    }
}

Mesh Mesh::GenerateGrid(
    const UINT width, const UINT height,
    const float horizontalSpacing, const float verticalSpacing,
    XMVECTOR horizontalAxis,
    XMVECTOR verticalAxis) {

    XMFLOAT3 normal;
    XMStoreFloat3(&normal, XMVector3Cross(horizontalAxis, verticalAxis));

    Mesh mesh;
    mesh.numberOfVertices = width * height;
    mesh.numberOfIndices = (width - 1) * (height - 1) * 2 * 3; // (width-1) * (height-1) squares * 2 triangles per square * 3 vertices per triangle

    mesh.vertices = new UVNormVertex[mesh.numberOfVertices];
    mesh.indices = new UINT32[mesh.numberOfIndices];

    float du = 1.f / (width - 1);
    float dv = 1.f / (height - 1);

    UINT32 index = 0;
    for (UINT32 x = 0; x < width; ++x) {
        for (UINT32 y = 0; y < height; ++y) {
            XMStoreFloat3(&mesh.vertices[index].position, x * horizontalAxis * horizontalSpacing + y * verticalAxis * verticalSpacing);
            mesh.vertices[index].normal = normal;
            mesh.vertices[index].uv = {x * du, y * dv};
            ++index;
        }
    }

    index = 0;
    for (UINT32 x = 0; x < width - 1; ++x) {
        for (UINT32 y = 0; y < height - 1; ++y) {
            // Top left triangle
            mesh.indices[index] = x + width * y;            
            mesh.indices[index + 1] = x + width * (y + 1);
            mesh.indices[index + 2] = (x + 1) + width * y;

            // Bottom right triangle
            mesh.indices[index + 3] = (x + 1) + width * y;                       
            mesh.indices[index + 4] = x + width * (y + 1);
            mesh.indices[index + 5] = (x + 1) + width * (y + 1);

            index += 6;
        }
    }

    return mesh;
}

void Mesh::scheduleUpload(ComPtr<ID3D12Device> m_device, ComPtr<ID3D12GraphicsCommandList> m_commandList) {
    // Create the vertex buffer.
    auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    {
        auto vertexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(UVNormVertex) * numberOfVertices);

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
        vertexData.pData = vertices;
        vertexData.RowPitch = sizeof(UVNormVertex) * numberOfVertices;
        vertexData.SlicePitch = vertexData.RowPitch;

        auto transitionDesc = CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

        UpdateSubresources<1>(m_commandList.Get(), m_vertexBuffer.Get(), vertexBufferUploadHeap.Get(), 0, 0, 1, &vertexData);
        m_commandList->ResourceBarrier(1, &transitionDesc);

        // Initialize the vertex buffer view.
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(UVNormVertex);
        m_vertexBufferView.SizeInBytes = sizeof(UVNormVertex) * numberOfVertices;
    }

    // Create the index buffer.
    {
        auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(numberOfIndices * sizeof(UINT32));

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
        indexData.pData = indices;
        indexData.RowPitch = numberOfIndices * sizeof(UINT32);
        indexData.SlicePitch = indexData.RowPitch;

        auto transitionDesc = CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

        UpdateSubresources<1>(m_commandList.Get(), m_indexBuffer.Get(), indexBufferUploadHeap.Get(), 0, 0, 1, &indexData);
        m_commandList->ResourceBarrier(1, &transitionDesc);

        // Describe the index buffer view.
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
        m_indexBufferView.SizeInBytes = numberOfIndices * sizeof(UINT32);
    }
}

void Mesh::draw(ComPtr<ID3D12GraphicsCommandList> commandList) {
    commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
    commandList->IASetIndexBuffer(&m_indexBufferView);

    commandList->DrawIndexedInstanced(numberOfIndices, 1, 0, 0, 0);
}