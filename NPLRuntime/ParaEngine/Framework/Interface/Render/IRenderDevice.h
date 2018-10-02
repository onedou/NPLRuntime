#pragma once

#include <memory>
#include <stdint.h>
#include "Framework/RenderSystem/RenderTypes.h"
#include "Framework/Common/Math/Rect.h"
#include "Framework/Common/Math/Color4f.h"
#include "Framework/Interface/Render/ITexture.h"
#include "Framework/Common/Image.hpp"
#include "Core/PEtypes.h"
#include "IEffect.h"
#include "math/ParaViewport.h"




#if defined(DEBUG)
	#define PE_CHECK_GL_ERROR_DEBUG() IRenderDevice::CheckRenderError(__FILE__, __FUNCTION__, __LINE__);
#else
	#define PE_CHECK_GL_ERROR_DEBUG() ((void)0)
#endif



namespace IParaEngine
{
	class ITexture;
}

namespace ParaEngine
{
	class CVertexDeclaration;
	class ParaViewport;
	class IRenderContext;

	struct RenderDeviceCaps
	{
		bool DynamicTextures;
		bool MRT;
		bool NPOT; // supports the use of 2D textures with dimensions that are not powers of two
		uint32_t MaxSimultaneousTextures;
		bool ScissorTest;
		bool Stencil;
		uint32_t NumSimultaneousRTs;
		bool SupportS3TC;
		bool Texture_R32F;
		bool Texture_RGBA16F;
        bool BorderClamp;
		RenderDeviceCaps()
			:DynamicTextures(false)
			,MRT(false)
			,NPOT(false)
			,MaxSimultaneousTextures(0)
			,ScissorTest(false)
			,Stencil(false)
			,NumSimultaneousRTs(0)
			,SupportS3TC(false)
			,Texture_R32F(false)
			,Texture_RGBA16F(false)
            ,BorderClamp(false)
		{

		}
	};


	class IRenderDevice
	{
	public:
		IRenderDevice() = default;
		virtual ~IRenderDevice() = default;

		virtual uint32_t GetRenderState(const ERenderState& State) = 0;
		virtual bool SetRenderState(const ERenderState State, const uint32_t Value) = 0;
		virtual bool SetClipPlane(uint32_t Index, const float* pPlane) = 0;
		virtual bool ReadPixels(int nLeft, int nTop, int nWidth, int nHeight, void* pDataOut, uint32_t nDataFormat = 0, uint32_t nDataType = 0) = 0;
		virtual int GetMaxSimultaneousTextures() = 0;
		virtual bool DrawPrimitive(EPrimitiveType PrimitiveType, uint32_t StartVertex, uint32_t PrimitiveCount) = 0;
		virtual bool DrawIndexedPrimitive(EPrimitiveType Type, int BaseVertexIndex, uint32_t MinIndex, uint32_t NumVertices, uint32_t indexStart, uint32_t PrimitiveCount) = 0;
		virtual bool DrawPrimitiveUP(EPrimitiveType PrimitiveType, uint32_t PrimitiveCount,const void* pVertexStreamZeroData, uint32_t VertexStreamZeroStride) = 0;
		virtual bool DrawIndexedPrimitiveUP(
			EPrimitiveType PrimitiveType,
			uint32_t MinVertexIndex,
			uint32_t NumVertices,
			uint32_t PrimitiveCount,
			const void * pIndexData,
			EPixelFormat IndexDataFormat,
			const void* pVertexStreamZeroData,
			uint32_t VertexStreamZeroStride) = 0;

		virtual bool SetTransform(ETransformsStateType State, DeviceMatrix_ptr pMatrix) = 0;
		virtual bool SetFVF(uint32_t FVF) = 0;
		virtual void SetCursorPosition(int X, int Y, uint32_t Flags) = 0;

		virtual bool SetSamplerState(uint32_t stage, ESamplerStateType type, uint32_t value) = 0;
		virtual bool GetSamplerState(uint32_t stage, ESamplerStateType type, uint32_t* value) = 0;

		virtual bool SetVertexDeclaration(VertexDeclarationPtr pVertexDeclaration) = 0;
		virtual bool CreateVertexDeclaration(VertexElement* pVertexElements, VertexDeclarationPtr* ppDecl) = 0;
		virtual bool SetIndices(IndexBufferDevicePtr_type pIndexData) = 0;
		virtual bool SetStreamSource(uint32_t StreamNumber, VertexBufferDevicePtr_type pStreamData, uint32_t OffsetInBytes, uint32_t Stride) = 0;
		virtual ParaViewport GetViewport() = 0;
		virtual bool SetViewport(const ParaViewport& viewport) = 0;

		virtual bool SetClearColor(const Color4f& color) = 0;
		virtual bool SetClearDepth(const float depth) = 0;
		virtual bool SetClearStencil(const int stencil) = 0;

		virtual bool Clear(bool color,bool depth,bool stencil) = 0;

		virtual bool SetScissorRect(RECT* pRect) = 0;
		virtual bool GetScissorRect(RECT* pRect) = 0;


		virtual bool BeginScene() = 0;
		virtual bool EndScene() = 0;

		virtual bool Present() = 0;
		virtual bool StretchRect(IParaEngine::ITexture* source, IParaEngine::ITexture* dest, RECT* srcRect, RECT* destRect) = 0;
		virtual bool SetTexture(uint32_t slot, IParaEngine::ITexture* texture) = 0;

		/** check render error and print to log. only call this in debug mode, since it breaks parallelism between gpu and cpu. */
		static bool CheckRenderError(const char* filename = NULL, const char* func = NULL, int nLine = 0);

		virtual std::shared_ptr<IParaEngine::IEffect> CreateEffect(const void* pSrcData,uint32_t srcDataLen, IParaEngine::IEffectInclude* include,std::string& error) = 0;
		
		virtual IParaEngine::ITexture* CreateTexture(uint32_t width, uint32_t height, EPixelFormat format,ETextureUsage usage) = 0;
		virtual IParaEngine::ITexture* CreateTexture(const ImagePtr& image) = 0;

		virtual bool SetRenderTarget(uint32_t index, IParaEngine::ITexture* target) = 0;
		virtual bool SetDepthStencil(IParaEngine::ITexture* target) = 0;

		virtual const RenderDeviceCaps& GetCaps() = 0;

		virtual IParaEngine::ITexture* GetRenderTarget(uint32_t index) = 0;
		virtual IParaEngine::ITexture* GetDepthStencil() = 0;

		virtual  IParaEngine::ITexture* GetBackbufferRenderTarget() = 0;
		virtual  IParaEngine::ITexture* GetBackbufferDepthStencil() = 0;
	};

	using IRenderDevicePtr = std::shared_ptr<IRenderDevice>;
}

