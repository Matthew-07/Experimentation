#pragma once

#include "Mesh.h"

using namespace DirectX;

struct ObjectConstantBuffer {
	XMMATRIX model_matrix{};
	XMMATRIX normal_matrix{};

	float k_s = 0.f;
	float alpha = 0.f;
};

using namespace DirectX;

class Object
{
public:
	Object(std::string mesh);

private:
	Mesh* m_mesh;

	static std::unordered_map<std::string, Mesh> m_meshes;

	XMVECTOR m_position{0.f};
	XMVECTOR m_scaling{1.f};
	XMVECTOR m_quaternion{0.f};

	float m_ks = 0.5f;
	float m_alpha = 10.f;

	ComPtr<ID3D12Resource> m_objectConstantBuffer;
	ObjectConstantBuffer m_objectConstantBufferData;
	UINT8* m_pObjectCbvDataBegin;

	ObjectConstantBuffer getObjectCB() const;
};

