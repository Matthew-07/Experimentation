#pragma once

#include "Mesh.h"
#include "Device.h"

using namespace DirectX;

struct ObjectConstantBuffer {
	XMMATRIX model_matrix{}; // 16
	XMMATRIX normal_matrix{}; // 16

	float k_s = 0.f; // 1
	float alpha = 0.f; // 1

	float padding[64 - 16 - 16 - 1 - 1];
};

using namespace DirectX;

class Object
{
public:
	Object(Mesh mesh, std::string meshLabel, Device& device, UINT32 frameCount);
	Object(std::string mesh, Device& device, UINT32 frameCount);

	void updateConstantBuffer(UINT frameIndex);

	void setPosition(XMVECTOR position) { m_position = position; }
	void setScaling(XMVECTOR scaling) { m_scaling = scaling; }
	void setQuaternion(XMVECTOR quaternion) { m_quaternion = quaternion; }

	XMVECTOR getPosition() { return m_position; }
	XMVECTOR getScaling() { return m_scaling; }
	XMVECTOR getQuaternion() { return m_quaternion; }

	void translate(XMVECTOR offset) { m_position += offset; }
	void scale(XMVECTOR ratios) { m_scaling *= ratios; }
	void rotate(XMVECTOR quaternion) { m_quaternion = XMQuaternionMultiply(quaternion, m_quaternion); }

	static void uploadMeshes(ComPtr<ID3D12Device> m_device, ComPtr<ID3D12GraphicsCommandList> m_commandList) {
		for (std::pair<const std::string, Mesh>& pair : m_meshes) {
			pair.second.scheduleUpload(m_device, m_commandList);
		}
	}

	static void finaliseMeshes() {
		for (std::pair<const std::string, Mesh>& pair : m_meshes) {
			pair.second.finaliseUpload();
		}
	}

	void draw(ComPtr<ID3D12GraphicsCommandList> m_commandList, UINT rootParameterIndex, UINT frameIndex);

private:
	Device& m_device;
	UINT m_frameCount = 2;

	Mesh* m_mesh;

	static std::unordered_map<std::string, Mesh> m_meshes;

	XMVECTOR m_position{0.f};
	XMVECTOR m_scaling{1.f};
	XMVECTOR m_quaternion{0.f};

	float m_ks = 0.5f;
	float m_alpha = 10.f;

	ComPtr<ID3D12Resource> m_objectConstantBuffer;
	UINT8* m_pObjectCbvDataBegin;

	ObjectConstantBuffer getObjectCB() const;

	void createConstantBuffer();
};

