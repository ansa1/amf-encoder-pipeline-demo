/*
 * Copyright (c) 2019, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Defs.h"
#include "DDAImpl.h"
#include "Preproc.h"

#include "AMFEncoder.h"

//DXGI
#include <dxgi.h>
#include <d3d11.h>

// AMF
#include "amf/amf/public/common/AMFFactory.h"
#include "amf/amf/public/include/components/VideoEncoderVCE.h"
#include "amf/amf/public/include/components/VideoConverter.h"
#include "amf/amf/public/common/Thread.h"
#include "amf/amf/public/common/AMFSTL.h"
#include "amf/amf/public/common/TraceAdapter.h"
using namespace amf;

// TODO: re-move all parameters to Encoder class
static amf_int32 frameRateIn = 60;
static amf_int64 bitRateIn = 5000000;
static amf_int32 frameCount = 500;
static bool bMaximumSpeed = true;
static amf_int32 outputWidth = 1920;
static amf_int32 outputHeight = 1080;
static const wchar_t* fileNameOut = L"output.h264";



class DemoApplication
{
    /// Demo Application Core class
#define returnIfError(x)\
    if (FAILED(x))\
    {\
        printf(__FUNCTION__": Line %d, File %s Returning error 0x%08x\n", __LINE__, __FILE__, x);\
        return x;\
    }

private:
    /// DDA wrapper object, defined in DDAImpl.h
    DDAImpl *pDDAWrapper = nullptr;
    /// PreProcesor for encoding. Defined in Preproc.h
    /// Preprocessingis required if captured images are of different size than encWidthxencHeight
    /// This application always uses this preprocessor
    RGBToNV12 *pColorConv = nullptr;
    /// NVENCODE API wrapper. Defined in NvEncoderD3D11.h. This class is imported from NVIDIA Video SDK
    //NvEncoderD3D11 *pEnc = nullptr;
    /// D3D11 device context used for the operations demonstrated in this application
    ID3D11Device *pD3DDev = nullptr;
    /// D3D11 device context
    ID3D11DeviceContext *pCtx = nullptr;
    /// D3D11 RGB Texture2D object that recieves the captured image from DDA
    ID3D11Texture2D *pDupTex2D = nullptr;
    /// D3D11 YUV420 Texture2D object that sends the image to NVENC for video encoding
    ID3D11Texture2D *pEncBuf = nullptr;
    /// Output video bitstream file handle
    FILE *fp = nullptr;
    /// Failure count from Capture API
    UINT failCount = 0;
    /// Video output file name
    const char* fileNameOut = "output1.h264";
    FILE* m_file = nullptr;
    //FILE* m_file;
    /*
    AMF_RESULT InitEncoder()
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
    AMF_RESULT InitClassEnc() 
    {
        AMF_RESULT res = g_AMFFactory.Init();
        if (res != AMF_OK)
        {
            //ProcessFailure(nullptr, L"AMF Failed to initialize", L"Error", HRESULT_FROM_AMF(res));
            return res;
        }

        amf_increase_timer_precision();

        g_AMFFactory.GetDebug()->AssertsEnable(true);
        AMFTraceSetGlobalLevel(AMF_TRACE_TRACE);
        AMFTraceEnableWriter(AMF_TRACE_WRITER_DEBUG_OUTPUT, true);

        res = g_AMFFactory.GetFactory()->CreateContext(&m_context);
        AMF_RETURN_IF_FAILED(res, L"CreateContext() failed");

        res = m_context->InitDX11(pD3DDev);
        AMF_RETURN_IF_FAILED(res, L"InitDX11(NULL) failed");

        res = InitConverter();
        AMF_RETURN_IF_FAILED(res, L"InitConverter() failed");

        res = InitEncoder();
        AMF_RETURN_IF_FAILED(res, L"InitEncoder() failed");



        return res;
    }
    AMF_RESULT ProcessFrame()
    {
        AMFSurfacePtr surface;

        AMF_RESULT res = m_context->CreateSurfaceFromDX11Native(pDupTex2D, &surface, nullptr);
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
    */
private:
    /*AMF_RESULT InitConverter()
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
    }*/

    /// Initialize DXGI pipeline
    HRESULT InitDXGI()
    {
        HRESULT hr = S_OK;
        /// Driver types supported
        D3D_DRIVER_TYPE DriverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

        /// Feature levels supported
        D3D_FEATURE_LEVEL FeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_1
        };
        UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
        D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_0;

        /// Create device
        for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
        {
            hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, /*D3D11_CREATE_DEVICE_DEBUG*/0, FeatureLevels, NumFeatureLevels,
                D3D11_SDK_VERSION, &pD3DDev, &FeatureLevel, &pCtx);
            if (SUCCEEDED(hr))
            {
                // Device creation succeeded, no need to loop anymore
                break;
            }
        }
        return hr;
    }

    /// Initialize DDA handler
    HRESULT InitDup()
    {
        HRESULT hr = S_OK;
        if (!pDDAWrapper)
        {
            pDDAWrapper = new DDAImpl(pD3DDev, pCtx);
            hr = pDDAWrapper->Init();
            returnIfError(hr);
        }
        return hr;
    }

    /// Initialize AMF Encoder wrapper
    HRESULT InitEnc()
    {
        assert(pCtx);
        AMFEncoder::GetInstance()->InitEnc(pD3DDev);
        //assert(res);
        return S_OK;
    }

    /// Initialize preprocessor
    HRESULT InitColorConv()
    {
        /*
        if (!pColorConv)
        {
            pColorConv = new RGBToNV12(pD3DDev, pCtx);
            HRESULT hr = pColorConv->Init();
            returnIfError(hr);
        */
        return S_OK;
    }

    /// Initialize Video output file
    HRESULT InitOutFile()
    {
        /*
        if (!fp)
        {
            char fname[64] = { 0 };
            sprintf_s(fname, (const char *)fnameBase, failCount);
            errno_t err = fopen_s(&fp, fname, "wb");
            returnIfError(err);
        }*/
        return S_OK;
    }

    /// Write encoded video output to file
    void WriteEncOutput()
    {
        /*
        int nFrame = 0;
        nFrame = (int)vPacket.size();
        for (std::vector<uint8_t> &packet : vPacket)
        {
            fwrite(packet.data(), packet.size(), 1, fp);
        }
        */
    }

public:
    /// Initialize demo application
    HRESULT Init()
    {
        HRESULT hr = S_OK;

        hr = InitDXGI();
        returnIfError(hr);

        hr = InitDup();
        returnIfError(hr);


        hr = InitEnc();
        returnIfError(hr);

        hr = InitColorConv();
        returnIfError(hr);

        hr = InitOutFile();
        returnIfError(hr);
        return hr;
    }

    /// Capture a frame using DDA
    HRESULT Capture(int wait)
    {
        HRESULT hr = pDDAWrapper->GetCapturedFrame(&pDupTex2D, wait); // Release after preproc
        if (FAILED(hr))
        {
            failCount++;
        }
        return hr;
    }

    /// Preprocess captured frame
    HRESULT Preproc()
    {
        HRESULT hr = S_OK;
        return hr;
    }

    /// Encode the captured frame using AMF
    HRESULT Encode()
    {
        unsigned long netBufferSize = 4 * 1024 * 1024;
        uint8_t* netBuffer = new uint8_t[netBufferSize]; // 2MB
        unsigned long bufferSize = AMFEncoder::GetInstance()->ProcessFrame(&pDupTex2D, netBuffer, netBufferSize);
        if (bufferSize == -1)
            printf("ProcessFrame() failed\n");
        else {
            if (!m_file)
                fopen_s(&m_file, fileNameOut, "wb");
            else
            {
                fwrite(netBuffer, 1, bufferSize, m_file);
            }
        }
        return S_OK;
    }

    HRESULT closeFile() {
        if (m_file)
            fclose(m_file);
        return S_OK;
    }

    /// Release all resources
    void Cleanup(bool bDelete = true)
    {
        
    }
    DemoApplication() {}
    ~DemoApplication()
    {
        Cleanup(true); 
    }
};

/// Demo 60 FPS (approx.) capture
int Grab60FPS(int nFrames)
{
    const int WAIT_BASE = 17;
    DemoApplication Demo;
    HRESULT hr = S_OK;
    int capturedFrames = 0;
    LARGE_INTEGER start = { 0 };
    LARGE_INTEGER end = { 0 };
    LARGE_INTEGER interval = { 0 };
    LARGE_INTEGER freq = { 0 };
    int wait = WAIT_BASE;

    QueryPerformanceFrequency(&freq);

    /// Reset waiting time for the next screen capture attempt
#define RESET_WAIT_TIME(start, end, interval, freq)         \
    QueryPerformanceCounter(&end);                          \
    interval.QuadPart = end.QuadPart - start.QuadPart;      \
    MICROSEC_TIME(interval, freq);                          \
    wait = (int)(WAIT_BASE - (interval.QuadPart * 1000));

    /// Initialize Demo app
    hr = Demo.Init();
    if (FAILED(hr))
    {
        printf("Initialization failed with error 0x%08x\n", hr);
        return -1;
    }

    /// Run capture loop
    do
    {
        /// get start timestamp. 
        /// use this to adjust the waiting period in each capture attempt to approximately attempt 60 captures in a second
        QueryPerformanceCounter(&start);
        /// Get a frame from DDA
        hr = Demo.Capture(wait);
        // after capture() pDup2D texture has new frame - DONE
        if (hr == DXGI_ERROR_WAIT_TIMEOUT) 
        {
            /// retry if there was no new update to the screen during our specific timeout interval
            /// reset our waiting time
            RESET_WAIT_TIME(start, end, interval, freq);
            continue;
        }
        else
        {
            if (FAILED(hr))
            {
                /// Re-try with a new DDA object
                printf("Capture failed with error 0x%08x. Re-create DDA and try again.\n", hr);
                Demo.Cleanup(); // TODO
                hr = Demo.Init();
                if (FAILED(hr))
                {
                    /// Could not initialize DDA, bail out/
                    printf("Failed to Init DDDemo. return error 0x%08x\n", hr);
                    return -1;
                }
                RESET_WAIT_TIME(start, end, interval, freq);
                QueryPerformanceCounter(&start);
                /// Get a frame from DDA
                Demo.Capture(wait);
            }
            RESET_WAIT_TIME(start, end, interval, freq);
            /// Preprocess for encoding
            hr = Demo.Preproc(); 
            // I think we do not need preproc()
            if (FAILED(hr))
            {
                printf("Preproc failed with error 0x%08x\n", hr);
                return -1;
            }
            hr = Demo.Encode();
            // TODO: Encode() should encode pDup2D and write raw .h264 data to buffer
            // Encode() call ProcessFrame() function that returns buffer with .h264 encoded frame
            if (FAILED(hr))
            {
                printf("Encode failed with error 0x%08x\n", hr);
                return -1;
            }
            capturedFrames++;
        }
    } while (capturedFrames <= nFrames);

    Demo.closeFile();
    return 0;
}

void printHelp()
{
    printf(" DXGIOUTPUTDuplication_NVENC_Demo: This application demonstrates using DDA (a.k.a. DesktopDuplication or OutputDuplication or IDXGIOutputDuplication API\n\
                                               to capture desktop and efficiently encode the captured images using NVENCODEAPI.\n\
                                               The output video bitstream will be generated with name DDATest_0.h264 in current working directory.\n\
                                               This application only captures the primary desktop.\n");
    printf(" DXGIOUTPUTDuplication_NVENC_Demo: Commandline help\n");
    printf(" DXGIOUTPUTDuplication_NVENC_Demo: '-?', '?', '-about', 'about', '-h', '-help', '-usage', 'h', 'help', 'usage': Print this Commandline help\n");
    printf(" DXGIOUTPUTDuplication_NVENC_Demo: '-frames <n>': n = No. of frames to capture");
}

int main(int argc, char** argv)
{
    /// The app will try to capture 60 times, by default
    int nFrames = frameCount;
    int ret = 0;
    bool useNvenc = true;

    /// Parse arguments
    try
    {
        if (argc > 1)
        {
            for (int i = 0; i < argc; i++)
            {
                if (!strcmpi("-frames", argv[i]))
                {
                    nFrames = atoi(argv[i + 1]);
                }
                else if (!strcmpi("-frames", argv[i]))
                {
                    useNvenc = true;
                }
                else if ((!strcmpi("-help", argv[i])) ||
                         (!strcmpi("-h", argv[i])) ||
                         (!strcmpi("h", argv[i])) ||
                         (!strcmpi("help", argv[i])) ||
                         (!strcmpi("-usage", argv[i])) ||
                         (!strcmpi("-about", argv[i])) ||
                         (!strcmpi("-?", argv[i])) ||
                         (!strcmpi("?", argv[i])) ||
                         (!strcmpi("about", argv[i])) ||
                         (!strcmpi("usage", argv[i]))
                        )
                {
                    printHelp();
                }
            }
        }
        else
        {
            //printHelp();
            //return 0;
        }
    }
    catch (...)
    {
        printf(" DXGIOUTPUTDuplication_NVENC_Demo: Invalid arguments passed.\n\
                                                   Continuing to grab 60 frames.\n");
        printHelp();
    }
    printf(" DXGIOUTPUTDuplication_NVENC_Demo: Frames to Capture: %d.\n", nFrames);
    Sleep(1000);
    /// Kick off the demo
    ret = Grab60FPS(nFrames);
    return ret;
}