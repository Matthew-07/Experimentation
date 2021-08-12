#pragma once

using namespace DirectX;

class Camera
{
public:
	void init(XMVECTOR position, XMVECTOR lookDirection, XMVECTOR upDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f),
		float fov = XMConvertToRadians(70.f), float aspectRatio = 16.f / 9.f, float nearClippingPlane = 0.1f, float farClippingPlane = 100.f) {
		m_position = position;
		m_lookDirection = XMVector3Normalize(lookDirection);
		m_upDirection = XMVector3Normalize(upDirection);
		m_leftDirection = XMVector3Cross(lookDirection, upDirection);

		m_fov = fov;
		m_aspectRatio = aspectRatio;
		m_nearClippingPlane = nearClippingPlane;
		m_farClippingPlane = farClippingPlane;
	}

	void setPosition(XMVECTOR position) {
		m_position = position;
	}
	void setLookDirection(XMVECTOR direction) {
		m_lookDirection = XMVector3Normalize(direction);
		m_leftDirection = XMVector3Cross(m_lookDirection, m_upDirection);
	}
	void setFocusPoint(XMVECTOR position) {
		m_lookDirection = XMVector3Normalize(position - m_position);
		m_leftDirection = XMVector3Cross(m_lookDirection, m_upDirection);
	}

	XMVECTOR getPosition() { return m_position; }
	XMVECTOR getLookDirection() { return m_lookDirection; }

	void pitch(float angle) {
		/*if (angle > 0 && XMVectorGetX(XMVector3Dot(m_lookDirection, m_upDirection)) < 0.9f ||
			angle < 0 && XMVectorGetX(XMVector3Dot(m_lookDirection, m_upDirection)) > -0.9f) {
			m_lookDirection = XMVector3Transform(m_lookDirection, XMMatrixRotationAxis(m_leftDirection, -angle));
		}*/

		XMVECTOR newLookDirection = XMVector3Transform(m_lookDirection, XMMatrixRotationAxis(m_leftDirection, -angle));
		
		//if (XMVectorGetX(XMVector3Dot(XMVector3Cross(newLookDirection, m_leftDirection), m_upDirection)) < 0) {
		if (XMVectorGetX(XMVector3Dot(XMVector3Cross(newLookDirection, m_leftDirection), m_upDirection)) < -0.1) {
			m_lookDirection = newLookDirection;
		}

		//m_leftDirection = XMVector3Cross(m_lookDirection, m_upDirection);
	}
	void yawRight(float angle) {
		m_lookDirection = XMVector3Transform(m_lookDirection, XMMatrixRotationAxis(m_upDirection, angle));
		m_leftDirection = XMVector3Transform(m_leftDirection, XMMatrixRotationAxis(m_upDirection, angle));
		//m_leftDirection = XMVector3Cross(m_lookDirection, m_upDirection);
	}

	void forward(float distance) {
		m_position += m_lookDirection * distance;
	}
	void backward(float distance) {
		m_position -= m_lookDirection * distance;
	}

	void left(float distance) {
		m_position += m_leftDirection * distance;
	}
	void right(float distance) {
		m_position -= m_leftDirection * distance;
	}

	void up(float distance) {
		m_position += m_upDirection * distance;
	}
	void down(float distance) {
		m_position -= m_upDirection * distance;
	}

	XMMATRIX getViewMatrix();
	XMMATRIX getProjMatrix();

	void setFov(float fov) { m_fov = fov; }
	void setAspectRatio(float aspectRatio) {
		m_aspectRatio = aspectRatio;
	}
	void setNearClippingPlane(float distance) {
		m_nearClippingPlane = distance;
	}
	void setFarClippingPlane(float distance) {
		m_farClippingPlane = distance;
	}

private:
	XMVECTOR m_position;
	XMVECTOR m_lookDirection;
	XMVECTOR m_leftDirection;

	// Camera orientation
	XMVECTOR m_upDirection;

	float m_fov = XMConvertToRadians(70.f);
	float m_aspectRatio = 16.f / 9.f;
	float m_nearClippingPlane = 0.1f, m_farClippingPlane = 100.f;
};

