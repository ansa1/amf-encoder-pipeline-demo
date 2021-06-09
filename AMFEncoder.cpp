#include "AMFEncoder.h"

#define FACILITY_AMF 0xA3F
HRESULT HRESULT_FROM_AMF(unsigned long x) { return x == AMF_OK ? S_OK : (HRESULT)(((x) & 0x0000FFFF) | (FACILITY_AMF << 16) | 0x80000000); }

static amf_int32 frameRateIn = 30;
static amf_int64 bitRateIn = 5000000;
static amf_int32 rectSize = 50;
static amf_int32 frameCount = 500;
static bool bMaximumSpeed = true;
static amf_int32 outputWidth = 1280;
static amf_int32 outputHeight = 720;
static const wchar_t* fileNameOut = L"output.h264";

AMFEncoder::AMFEncoder()
{
	AMF_RESULT res = g_AMFFactory.Init();
	if (res != AMF_OK)
	{
		//ProcessFailure(nullptr, L"AMF Failed to initialize", L"Error", HRESULT_FROM_AMF(res));
		return;
	}

	amf_increase_timer_precision();

	g_AMFFactory.GetDebug()->AssertsEnable(true);
	AMFTraceSetGlobalLevel(AMF_TRACE_TRACE);
	AMFTraceEnableWriter(AMF_TRACE_WRITER_DEBUG_OUTPUT, true);

	//_wfopen_s(&m_file, fileNameOut, L"wb");
}

AMFEncoder::~AMFEncoder()
{
	if (m_deviceContext)
	{
		m_deviceContext->Release();
	}
}

AMF_RESULT AMFEncoder::InitConverter()
{
	assert(m_context);

	AMF_RESULT res = g_AMFFactory.GetFactory()->CreateComponent(m_context, AMFVideoConverter, &m_converter);
	AMF_RETURN_IF_FAILED(res, L"CreateComponent(%s) failed", AMFVideoConverter);

	res = m_converter->SetProperty(AMF_VIDEO_CONVERTER_KEEP_ASPECT_RATIO, true);
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_CONVERTER_KEEP_ASPECT_RATIO, true) failed");

	res = m_converter->SetProperty(AMF_VIDEO_CONVERTER_MEMORY_TYPE, AMF_MEMORY_DX11);
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_CONVERTER_MEMORY_TYPE, AMF_MEMORY_DX11) failed");

	res = m_converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_FORMAT, AMF_SURFACE_NV12);
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_FORMAT, AMF_SURFACE_NV12) failed");

	AMFSize size;
	size.width = outputWidth;
	size.height = outputHeight;
	res = m_converter->SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_SIZE, size);
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_CONVERTER_OUTPUT_SIZE, %dx%d) failed", size.width, size.height);

	amf_int32 width = 1920;
	amf_int32 height = 1080;

	res = m_converter->Init(AMF_SURFACE_BGRA, width, height);
	AMF_RETURN_IF_FAILED(res, L"converter->Init() failed");
	return res;
}

AMF_RESULT AMFEncoder::InitEncoder()
{
	assert(m_context);

	AMF_RESULT res = g_AMFFactory.GetFactory()->CreateComponent(m_context, AMFVideoEncoderVCE_AVC, &m_encoder);
	AMF_RETURN_IF_FAILED(res, L"CreateComponent(%s) failed", AMFVideoEncoderVCE_AVC);

	res = m_encoder->SetProperty(AMF_VIDEO_ENCODER_USAGE, AMF_VIDEO_ENCODER_USAGE_TRANSCONDING);
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_ENCODER_USAGE, AMF_VIDEO_ENCODER_USAGE_TRANSCONDING) failed");

	if (bMaximumSpeed)
	{
		res = m_encoder->SetProperty(AMF_VIDEO_ENCODER_B_PIC_PATTERN, 0);
		// do not check error for AMF_VIDEO_ENCODER_B_PIC_PATTERN - can be not supported - check Capability Manager sample
		res = m_encoder->SetProperty(AMF_VIDEO_ENCODER_QUALITY_PRESET, AMF_VIDEO_ENCODER_QUALITY_PRESET_SPEED);
		AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_ENCODER_QUALITY_PRESET, AMF_VIDEO_ENCODER_QUALITY_PRESET_SPEED) failed");
	}

	res = m_encoder->SetProperty(AMF_VIDEO_ENCODER_TARGET_BITRATE, bitRateIn);
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_ENCODER_TARGET_BITRATE, %" LPRId64 L") failed", bitRateIn);

	res = m_encoder->SetProperty(AMF_VIDEO_ENCODER_FRAMESIZE, ::AMFConstructSize(outputWidth, outputHeight));
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_ENCODER_FRAMESIZE, %dx%d) failed", outputWidth, outputHeight);

	res = m_encoder->SetProperty(AMF_VIDEO_ENCODER_FRAMERATE, ::AMFConstructRate(frameRateIn, 1));
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_ENCODER_FRAMERATE, %dx%d) failed", frameRateIn, 1);

	res = m_encoder->Init(AMF_SURFACE_NV12, outputWidth, outputHeight);
	AMF_RETURN_IF_FAILED(res, L"encoder->Init() failed");
	return res;
}

AMF_RESULT AMFEncoder::InitEnc(/*DX_RESOURCES* Res, DXGI_OUTPUT_DESC* Desc*/ID3D11DeviceContext* pCtx)
{
	//assert(Res);
	//assert(Desc);
	AMF_RESULT res = g_AMFFactory.GetFactory()->CreateContext(&m_context);
	AMF_RETURN_IF_FAILED(res, L"CreateContext() failed");

	HRESULT hr = pCtx->QueryInterface(&m_deviceContext);
	if (FAILED(hr))
	{
		AMF_RETURN_IF_FAILED(AMF_FAIL, L"QueryInterface() failed");
	}

	res = m_context->InitDX11(NULL);
	AMF_RETURN_IF_FAILED(res, L"InitDX11(NULL) failed");

	res = InitConverter();
	AMF_RETURN_IF_FAILED(res, L"InitConverter() failed");

	res = InitEncoder();
	AMF_RETURN_IF_FAILED(res, L"InitEncoder() failed");
	return res;
}

AMF_RESULT AMFEncoder::ProcessFrame(ID3D11Texture2D* pDupTex2D)
{
	ID3D11Texture2D* SharedSurf = pDupTex2D;
	//assert(Data);
	//assert(SharedSurf);
	//assert(DeskDesc);
	AMFSurfacePtr surface;

	m_deviceContext->CopySubresourceRegion(SharedSurf, 0, 0, 0, 0, pDupTex2D, 0, NULL);

	AMF_RESULT res = m_context->CreateSurfaceFromDX11Native(SharedSurf, &surface, nullptr);
	AMF_RETURN_IF_FAILED(res, L"CreateSurfaceFromDX11Native() failed");

	res = m_converter->SubmitInput(surface);
	AMF_RETURN_IF_FAILED(res, L"converter->SubmitInput() failed");

	AMFDataPtr converted;
	res = m_converter->QueryOutput(&converted);
	AMF_RETURN_IF_FAILED(res, L"converter->QueryOutput() failed");

	res = converted->SetProperty(AMF_VIDEO_ENCODER_INSERT_SPS, true);
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_ENCODER_INSERT_SPS, true) failed");

	res = converted->SetProperty(AMF_VIDEO_ENCODER_INSERT_PPS, true);
	AMF_RETURN_IF_FAILED(res, L"SetProperty(AMF_VIDEO_ENCODER_INSERT_PPS, true) failed");

	res = m_encoder->SubmitInput(converted);
	AMF_RETURN_IF_FAILED(res, L"encoder->SubmitInput() failed");

	AMFDataPtr encoded;
	int count = 0;
	do
	{
		AMF_RESULT res = m_encoder->QueryOutput(&encoded);
		if (res == AMF_EOF || res == AMF_OK)
			break;

		count++;
		if (res == AMF_REPEAT)
			continue;

		AMF_RETURN_IF_FAILED(res, L"encoder->QueryOutput() failed");
	} while (true);
	/*
	wchar_t text[100];
	_snwprintf(text, ARRAYSIZE(text), L"encode %i", count);
	OutputDebugString(text);
	*/
	AMFBufferPtr buffer;
	res = encoded->QueryInterface(AMFBuffer::IID(), (void**)&buffer);
	AMF_RETURN_IF_FAILED(res, L"encoded->QueryInterface() failed");

	// play the file with VLC:
	// vlc output.h264 --demux h264
	static const char* fileNameOut = "output.h264";
	static FILE* m_file = nullptr;
	if (!m_file)
		fopen_s(&m_file, fileNameOut, "wb");
	else
	{
		fwrite(buffer->GetNative(), 1, buffer->GetSize(), m_file);
	}
	
	return AMF_OK;
}
