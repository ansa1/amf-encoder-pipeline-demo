#ifndef _ENCODINGMANAGER_H_
#define _ENCODINGMANAGER_H_

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

#include <thread>
#include <mutex>

using namespace amf;

//
// Handles the task of encoding frames
//
class AMFEncoder
{
protected:
    static std::mutex __mutex;
public:
    AMFEncoder();
    ~AMFEncoder();

    AMF_RESULT InitEnc(ID3D11Device* pD3DDev);
    unsigned long ProcessFrame(ID3D11Texture2D** pDupTex2D, uint8_t* buf, unsigned long bufsize);
    unsigned long AMFEncoder::CopyToBuffer(uint8_t* buf, unsigned long bufsize);

    static AMFEncoder* GetInstance() 
    {
        std::lock_guard<std::mutex> lock(__mutex);
        static AMFEncoder* __instance = new AMFEncoder();
        return __instance;
    }

private:
    AMFContextPtr m_context;
    AMFComponentPtr m_encoder;
    AMFComponentPtr m_converter;

    ID3D11DeviceContext* m_deviceContext;

    AMF_RESULT InitEncoder();
    AMF_RESULT InitConverter();
};

#endif