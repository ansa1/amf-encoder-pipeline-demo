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

using namespace amf;

//
// Handles the task of encoding frames
//
class AMFEncoder
{
public:
    AMFEncoder();
    ~AMFEncoder();

    AMF_RESULT InitEnc(ID3D11DeviceContext* pCtx);
    AMF_RESULT ProcessFrame(ID3D11Texture2D* pDupTex2D);

private:
    AMFContextPtr m_context;
    AMFComponentPtr m_encoder;
    AMFComponentPtr m_converter;

    ID3D11DeviceContext* m_deviceContext;

    AMF_RESULT InitEncoder();
    AMF_RESULT InitConverter();
};

#endif