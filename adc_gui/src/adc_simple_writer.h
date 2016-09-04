#pragma once

#include "wx\wx.h"
#include "wx\file.h"
#include "wx/hashmap.h"

#include "configuration.h"
#include "list"
#include "algorithm"

class CAdcSimpleWriter : public IAdcWriter
{
    CAdcSimpleWriter();

public:
    static CAdcSimpleWriter* Create(const char* pFilename);

    virtual ~CAdcSimpleWriter();

    virtual AdcMemoryBlock* RequestBuffer( int32 iSize );
    virtual void OnData( AdcDeviceData pPacket );

protected:
    FILE* m_pFile;

};
