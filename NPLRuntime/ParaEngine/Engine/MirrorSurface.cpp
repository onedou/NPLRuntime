//-----------------------------------------------------------------------------
// Class:	CMirrorSurface
// Authors:	Li, Xizhi
// Emails:	LiXizhi@yeah.net
// Company: ParaEngine
// Date:	2006.7.2
//-----------------------------------------------------------------------------
#include "ParaEngine.h"
#include "SceneObject.h"
#include "ParaWorldAsset.h"
#include "SortedFaceGroups.h"
#include "AutoCamera.h"

#include "MirrorSurface.h"
#include "memdebug.h"

#if USE_DIRECTX_RENDERER
#include "RenderDeviceD3D9.h"
#endif

using namespace ParaEngine;

/** @def reflection map width in pixels */
#define MAX_REFLECTION_MAP_WIDTH	512
/** @def reflection map height in pixels */
#define MAX_REFLECTION_MAP_HEIGHT	512

/** view angle overdraw for reflection map. 1.0 means the same as the current camera view angle. */
#define REFLECTION_MAP_OVERDRAW		1.0f

#ifndef CHECK_RETURN_CODE
#define CHECK_RETURN_CODE(text, hr) if(FAILED((hr))){OUTPUT_LOG(text);return;}
#endif


CMirrorSurface::CMirrorSurface(void)
:m_vPos(0,0,0),m_reflectionPlane(0,1,0,0), m_reflectionMapOverdraw(REFLECTION_MAP_OVERDRAW),m_bInitialized(false),
m_reflectionTextureWidth(MAX_REFLECTION_MAP_WIDTH), m_reflectionTextureHeight(MAX_REFLECTION_MAP_HEIGHT),
m_pDepthStencilSurface(NULL), m_pReflectionTexture(NULL),m_bEnable(false),m_bDisableFogInReflection(false)
{

}

CMirrorSurface::~CMirrorSurface(void)
{
	Cleanup();
}

IParaEngine::ITexture* CMirrorSurface::GetReflectionTexture()
{
	return m_pReflectionTexture;
}

void CMirrorSurface::Cleanup()
{

}
void CMirrorSurface::SetPlane(const Plane& plane)
{
	m_reflectionPlane = plane;
}

void CMirrorSurface::InitDeviceObjects()
{

}
void CMirrorSurface::RestoreDeviceObjects()
{
	HRESULT hr;

	auto pRenderDevice = CGlobals::GetRenderDevice();

	int deviceWidth = (int)CGlobals::GetRenderDevice()->GetBackbufferRenderTarget()->GetWidth();
	int deviceHeight = (int)CGlobals::GetRenderDevice()->GetBackbufferRenderTarget()->GetHeight();
	int nWidth = min(deviceWidth, m_reflectionTextureWidth);
	int nHeight = min(deviceHeight, m_reflectionTextureHeight);

	m_pReflectionTexture = pRenderDevice->CreateTexture(nWidth, nHeight, EPixelFormat::X8R8G8B8, ETextureUsage::RenderTarget);

	if (!m_pReflectionTexture)
	{
		OUTPUT_LOG("failed createTexture reflection texture");
		return;
	}

	m_pDepthStencilSurface = pRenderDevice->CreateTexture(nWidth, nHeight, EPixelFormat::D16, ETextureUsage::DepthStencil);
	if (!m_pDepthStencilSurface)
	{
		OUTPUT_LOG("failed creating depth stencil buffe");
		return;
	}
	m_bInitialized=true;
}

void CMirrorSurface::InvalidateDeviceObjects()
{
	m_bInitialized=false;
	SAFE_RELEASE(m_pDepthStencilSurface);
	SAFE_RELEASE(m_pReflectionTexture);
}

void CMirrorSurface::DeleteDeviceObjects()
{

}

void CMirrorSurface::SetPosition(Vector3 vPos, bool bUseCloset)
{
	float fEyeY = CGlobals::GetSceneState()->vEye.y;
	if(vPos.y<fEyeY)
	{
		if(IsEnabled() && bUseCloset)
		{
			if(m_vPos.y<fEyeY && m_vPos.y>vPos.y)
			{
				// use old position
			}
			else
				m_vPos = vPos;
		}
		else
			m_vPos = vPos;

		SetEnabled(true);
	}
}
//-----------------------------------------------------------------------------
// 
// Setup up the transforms for fixed function.
// Setup constants for vertex shaders.        
//
//-----------------------------------------------------------------------------
void CMirrorSurface::SetMatrices(bool bPostPushMatrices, bool bPrePopMatrices)
{
	// pop matrices
	if(bPrePopMatrices)
	{
		CGlobals::GetViewMatrixStack().pop();
	}

	auto pRenderDevice = CGlobals::GetRenderDevice();
	Matrix4 worldViewMatrix;
	Matrix4 worldInverseTransposeMatrix;
	Matrix4 viewMatrix;
	Matrix4 viewProjectionMatrix;
	Matrix4 worldViewProjectionMatrix;

	// Reflect the view matrix if asked
	if(m_reflectViewMatrix)
	{
		/*Matrix4 reflectionMatrix;

		Vector3 vRenderOrig =  CGlobals::GetScene()->GetRenderOrigin();

		D3DXMatrixReflect(&reflectionMatrix, &Plane(0.0f, 1.0f, 0.0f, -(m_fGlobalWaterLevel-vRenderOrig.y)));

		viewMatrix = reflectionMatrix * CGlobals::GetViewMatrixStack().SafeGetTop();*/

		// put the camera at the opposite side of the water plane. 
		// TODO: currently it only works when the original camera up axis is (0,1,0)
		// the reflection map should be flipped in x axis in image space.We will do this in shader.
		CAutoCamera* pCamera = ((CAutoCamera*)(CGlobals::GetScene()->GetCurrentCamera()));
		Vector3 vUp(0,-1,0);
		DVector3 vEye = pCamera->GetEyePosition();
		vEye.y = m_vPos.y*2-vEye.y;
		DVector3 vLookAt = pCamera->GetLookAtPosition();
		vLookAt.y = m_vPos.y*2-vLookAt.y;

		pCamera->ComputeViewMatrix( &viewMatrix, &vEye, &vLookAt, &vUp );
	}
	else
	{
		viewMatrix = CGlobals::GetViewMatrixStack().SafeGetTop();
	}

	// push matrices
	if(bPostPushMatrices)
	{
		CGlobals::GetViewMatrixStack().push(viewMatrix);
	}

	CGlobals::GetEffectManager()->UpdateD3DPipelineTransform(true,true, true);
}

void CMirrorSurface::RenderReflectionTexture()
{
	if(!m_bEnable)
		return;
	if(CGlobals::GetSceneState()->vEye.y >= m_vPos.y)
	{
		if(!m_bInitialized)
		{
			InitDeviceObjects();
			RestoreDeviceObjects();
		}
		auto pRenderDevice = CGlobals::GetRenderDevice();
		EffectManager* pEffectManager = CGlobals::GetEffectManager();
		Plane clipPlane;
		Plane transformedClipPlane;
		float     reflectionMapFieldOfView;

		//////////////////////////////////////////////////////////////////////////
		// Render to the reflection map
		auto pOldRenderTarget = pRenderDevice->GetRenderTarget(0);
		pRenderDevice->SetRenderTarget(0, m_pReflectionTexture);

		// set depth surface
		IParaEngine::ITexture* pOldZBuffer = pRenderDevice->GetDepthStencil();
		
		if(!pOldZBuffer)
			return;
		CGlobals::GetRenderDevice()->SetDepthStencil( m_pDepthStencilSurface );

		// Compute the field of view and use it
		CAutoCamera* pCamera = ((CAutoCamera*)(CGlobals::GetScene()->GetCurrentCamera()));
		reflectionMapFieldOfView = 2.0f * atan((tan(pCamera->GetFieldOfView() / 2.0f)) * m_reflectionMapOverdraw);
		ParaMatrixPerspectiveFovLH(&m_reflectionProjectionMatrix, reflectionMapFieldOfView, pCamera->GetAspectRatio(), pCamera->GetNearPlane(), pCamera->GetFarPlane());

		CGlobals::GetProjectionMatrixStack().push(m_reflectionProjectionMatrix);
		m_reflectViewMatrix = true;
		SetMatrices(true, false);
		if(m_bDisableFogInReflection)
			pEffectManager->EnableFog(false);

		pCamera->UpdateFrustum();

		//TODO:  here I assume the camera is always above mirror.
		// Set up the clip plane to draw the correct geometry
		clipPlane = m_reflectionPlane;
		// correct coordinate system
		Vector3 vRenderOrig =  CGlobals::GetScene()->GetRenderOrigin();
		clipPlane.d -= (m_vPos.y - vRenderOrig.y);

		// we will render SKYBOX and TERRAIN using fixed function pipeline. So this is always false.
		// in case we use a programmable pipeline for skybox and terrain, set the transformed clipping plane to clip space.
		// When the programmable pipeline is used the plane equations are assumed to be in the clipping space 
		// transform the plane into view space
		Matrix4 tempMatrix = CGlobals::GetViewMatrixStack().SafeGetTop();
		tempMatrix *= CGlobals::GetProjectionMatrixStack().SafeGetTop();
		tempMatrix = tempMatrix.inverse();
		tempMatrix = tempMatrix.transpose();
		transformedClipPlane = clipPlane.PlaneTransform(tempMatrix);
		
		pEffectManager->SetClipPlane(0, (const float*)&clipPlane, false);
		pEffectManager->SetClipPlane(0, (const float*)&transformedClipPlane, true);
		pEffectManager->EnableClipPlane(true);

		// D3DCLEAR_ZBUFFER is doomed. Hence, reflection must be rendered before the main thing is rendered. 
		GETD3D(CGlobals::GetRenderDevice())->Clear(0L, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, COLOR_RGBA(0, 0, 0, 0), 1.0f, 0L);

		//////////////////////////////////////////////////////////////////////////
		//
		// Render the actual scene to reflection texture: 
		//
		//////////////////////////////////////////////////////////////////////////

		bool bUseReflection = pEffectManager->IsReflectionRenderingEnabled();
		if(bUseReflection)
			pEffectManager->EnableReflectionRendering(false);
		// reverse cull mode, since we made the reflection. This should be done for both fixed function and programmable pipeline.
		//CGlobals::GetSceneState()->m_dwD3D_CULLMODE = RSV_CULL_CW; 
		// TODO: maybe first rendering terrain then rendering sky box will reduce fill rate ?
		//pRenderDevice->SetRenderState(ERenderState::CLIPPLANEENABLE, 0);
		CGlobals::GetScene()->RenderSelection(RENDER_SKY_BOX);
		pEffectManager->EnableClipPlane(true);
		CGlobals::GetScene()->RenderSelection(RENDER_PLAYER);
		CGlobals::GetScene()->RenderSelection(RENDER_MESH_FRONT_TO_BACK);
		// TODO: WHY not render smaller mesh ? CGlobals::GetScene()->RenderSelection(CSceneObject::RENDER_MESH_BACK_TO_FRONT);
		CGlobals::GetScene()->RenderSelection(RENDER_MESH_TRANSPARENT);

		if(!CGlobals::GetSceneState()->GetFaceGroups()->IsEmpty())
		{
			// NOT TESTED: translucent face groups.
			CGlobals::GetScene()->RenderSelection(RENDER_TRANSLUCENT_FACE_GROUPS);
			CGlobals::GetSceneState()->GetFaceGroups()->Clear();
		}

		//////////////////////////////////////////////////////////////////////////
		// Restore state for the normal pipeline
		pEffectManager->EnableClipPlane(false);
		if(m_bDisableFogInReflection)
			pEffectManager->EnableFog(CGlobals::GetScene()->IsFogEnabled());
		CGlobals::GetProjectionMatrixStack().pop();
		m_reflectViewMatrix = false;

		SetMatrices(false, true);
		pCamera->UpdateFrustum();

		// restore old depth surface
		pRenderDevice->SetDepthStencil( pOldZBuffer);
		SAFE_RELEASE(pOldZBuffer);

		//////////////////////////////////////////////////////////////////////////
		// Restore the old render target: i.e. the backbuffer
		pRenderDevice->SetRenderTarget(0, pOldRenderTarget);


		if(bUseReflection)
			pEffectManager->EnableReflectionRendering(true);
	}
	
	// always disable the mirror when the reflection map is updated. call GetReflectionTexture() to enable it.
	SetEnabled(false);
}