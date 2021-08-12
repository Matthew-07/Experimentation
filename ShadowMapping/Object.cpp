#include "stdafx.h"
#include "Object.h"

std::unordered_map<std::string, Mesh> Object::m_meshes{};

Object::Object(std::string filename)
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
	}
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
