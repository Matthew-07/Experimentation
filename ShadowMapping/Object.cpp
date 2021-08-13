#include "stdafx.h"
#include "Object.h"

std::unordered_map<std::string, Mesh> Object::m_meshes{};

Object::Object(Mesh mesh, std::string meshLabel, Device& device, UINT32 frameCount) : m_device(device)
{
	auto it = m_meshes.find(meshLabel);
	if (it == m_meshes.end()) {
		m_meshes.insert(std::make_pair(meshLabel, std::move(mesh)));

		m_frameCount = frameCount;

		it = m_meshes.find(meshLabel);
		m_mesh = &it->second;
	}
	else {
		m_mesh = &it->second;
	}

	createConstantBuffer();
}

Object::Object(std::string filename, Device& device, UINT32 frameCount) : m_device(device)
{
	auto it = m_meshes.find(filename);
	if (it != m_meshes.end()) {
		m_mesh = &it->second;
	}
	else {
		ElementDescriptor desciptors[3] = {
			{ "position", _FLOAT32, 3, 0 },
			{ "normal", _FLOAT32, 3, 12 },
			{ "tex", _FLOAT32, 2, 24 }
		};
		Mesh mesh{desciptors, 3};

		mesh.loadMeshFromFile(filename.c_str());

		m_meshes.insert(std::make_pair(filename, std::move(mesh)));

		it = m_meshes.find(filename);
		m_mesh = &it->second;
	}

	m_frameCount = frameCount;

	createConstantBuffer();
}

void Object::updateConstantBuffer(UINT frameIndex)
{
	ObjectConstantBuffer objectConstantBufferData = getObjectCB();
	memcpy(m_pObjectCbvDataBegin + frameIndex * sizeof(objectConstantBufferData), &objectConstantBufferData, sizeof(objectConstantBufferData));
}

void Object::draw(ComPtr<ID3D12GraphicsCommandList> m_commandList, UINT rootParameterIndex, UINT frameIndex)
{
	m_commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, m_objectConstantBuffer->GetGPUVirtualAddress() + frameIndex * sizeof(ObjectConstantBuffer));

	m_mesh->draw(m_commandList);
}

ObjectConstantBuffer Object::getObjectCB() const
{
	ObjectConstantBuffer cb{};
	cb.model_matrix = XMMatrixTranslationFromVector(m_position) *
		XMMatrixRotationQuaternion(m_quaternion) *
		XMMatrixScalingFromVector(m_scaling);

	cb.normal_matrix = XMMatrixTranspose(XMMatrixInverse(nullptr, cb.model_matrix));

	cb.k_s = m_ks;
	cb.alpha = m_alpha;

	return cb;
}

void Object::createConstantBuffer()
{
	auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

	// Object Constant Buffer
	{
		auto objectConstantBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ObjectConstantBuffer) * m_frameCount);

		ThrowIfFailed(m_device->CreateCommittedResource(
			&uploadHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&objectConstantBufferResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_objectConstantBuffer)));

		CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
		ThrowIfFailed(m_objectConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pObjectCbvDataBegin)));
		//memcpy(m_pCbvDataBegin, &m_sceneConstantBufferData, sizeof(m_sceneConstantBufferData));

		m_objectConstantBuffer->SetName(L"ObjectConstantBuffer");
	}
}
