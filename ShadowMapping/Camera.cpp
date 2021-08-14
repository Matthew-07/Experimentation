#include "stdafx.h"
#include "Camera.h"

XMMATRIX Camera::getViewMatrix()
{
    return XMMatrixLookToLH(m_position, m_lookDirection, m_upDirection);
}

XMMATRIX Camera::getProjMatrix()
{
    return XMMatrixPerspectiveFovLH(m_fov, m_aspectRatio, m_nearClippingPlane, m_farClippingPlane);
}
