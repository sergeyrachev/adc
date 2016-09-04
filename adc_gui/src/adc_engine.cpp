#include "adc_engine.h"

#include "ADC_Errors.h"
#include "DllClient.h"
#include "IADCDevice.h"

#define ADC_DLL_IMPORT_NAME "StaticFactory"
#define ADC_DEVICE_INTERFACE "IADCDevice" 

#include "adc_main_frame.h"

CAdcEngine* CAdcEngine::Create( const char* pDllPath, IAdcWriter* pWriter )
{
    CAdcEngine* pEngine = new CAdcEngine(pWriter);

    IFactory* pDllFactory = NULL; 
    pDllFactory = (IFactory*)pEngine->m_pDll->LoadByPath( _T(pDllPath), ADC_DLL_IMPORT_NAME);
    if ( pDllFactory ) {

        pEngine->m_pDevice = (IADCDevice*)(pDllFactory->Create(_T(ADC_DEVICE_INTERFACE), NULL));
        if ( pEngine->m_pDevice ) {

            int32 iErrCode = pEngine->m_pDevice->Setup(1, 0, 0, NULL);
            if ( iErrCode > 0 ) {
                return pEngine;
            }
        }
    }
    safe_delete(pEngine);
    return NULL;
}

CAdcEngine::~CAdcEngine()
{
    Reset();
    safe_release(m_pDevice);
    m_pDll->Free();
    safe_delete(m_pDll);
}

CAdcEngine::CAdcEngine(IAdcWriter* pWriter)
{
    m_pDll = new DllClient;
    m_pDevice = NULL;

    memset(&m_Options, 0, sizeof(AdcDeviceOptions));

    m_pWriter = pWriter;
    m_pDataThread = NULL;
}

AdcResult CAdcEngine::GetCapabilities( AdcDeviceCapabilities& aCapabilities ) 
{
    m_pDevice->Get( ADC_GET_SIZELIST_SIZE, &aCapabilities.iWords );
    m_pDevice->Get( ADC_GET_SIZELIST, (void*)&aCapabilities.pWords );

    m_pDevice->Get( ADC_GET_FREQLIST_SIZE, &aCapabilities.iFrequency );
    m_pDevice->Get( ADC_GET_FREQLIST, (void*)&aCapabilities.pFrequency );

    m_pDevice->Get( ADC_GET_GAINLIST_SIZE, &aCapabilities.iGain );
    m_pDevice->Get( ADC_GET_GAINLIST, (void*)&aCapabilities.pGain ); 

    m_pDevice->Get(ADC_GET_RANGE_BIPOLAR, &aCapabilities.fMaxVoltage); 
    m_pDevice->Get(ADC_GET_MAXAMP, &aCapabilities.iMaxTick);
    m_pDevice->Get(ADC_GET_DATABITS, &aCapabilities.iBitPerValue);

    return AdcOk;
}

void CAdcEngine::Start( )
{
    if ( m_pDataThread ) {

        m_pDataThread->Resume();        
    } else {

        m_pDataThread = new AdcDataThread( this );
        m_pDevice->Start();
        if ( m_pDataThread->Create() != wxTHREAD_NO_ERROR || m_pDataThread->Run() != wxTHREAD_NO_ERROR ) {

            safe_delete(m_pDataThread);
            return;
        }
    }
}

void CAdcEngine::Stop()
{
    if ( m_pDataThread ) {

        if( m_pDataThread->IsRunning() ) {
            m_pDataThread->Pause();
        }
    }
}

AdcResult CAdcEngine::Init( const AdcDeviceOptions& aOptions, IAdcInitCallback* pInitCallback)
{
    m_Options = aOptions;
    AdcInitThread* pInitThread = new AdcInitThread( this, pInitCallback );
    if ( pInitThread->Create() == wxTHREAD_NO_ERROR ) {

        if ( pInitThread->Run() == wxTHREAD_NO_ERROR ) {
            return AdcOk;
        }
    }
    safe_delete(pInitThread);
    return AdcError;
}       

int32 CAdcEngine::GenerateControlFlags()
{
    int32 iFlags = 0;
    iFlags |= ADC_CONTROL_ESW;      
    iFlags |= m_Options.iSyncSide;                   
    iFlags |= m_Options.iSyncEntry;
    return iFlags;
}


int32 CAdcEngine::GenerateReceiveFlags( AdcActiveChannel iChannel )
{
    int32 iFlags = 0;

    iFlags |= ADC_DATA_MODE_CONVERT2INT16; 
    iFlags |= ADC_DATA_MODE_DATASWAP;
    iFlags |= iChannel;   
    return iFlags;
}


bool CAdcEngine::IsDataReady()
{   
    int32 iReady = 0;
    int32 iErrCode = m_pDevice->Get(ADC_GET_READY, &iReady);
    if ( iErrCode > 0 ) {
        return !!iReady;
    } else {
        return false;
    } 
}

AdcResult CAdcEngine::InternalInit()
{
    Reset();
    int32 iErrCode = -1;      

    ADCParametersMEMORY aDeviceParams;
    aDeviceParams.m_nStartOf =       m_Options.iStartMode;  
    aDeviceParams.m_fFreq =          m_Options.fFrequency;  
    aDeviceParams.m_nBufferSize =    m_Options.iWords;      
    aDeviceParams.m_nBeforeHistory = 0;                     
    aDeviceParams.m_nDelta[0] =      0;                     
    aDeviceParams.m_nGain[0] =       m_Options.iGainCh0;    
    aDeviceParams.m_nDelta[1] =      0;                     
    aDeviceParams.m_nGain[1] =       m_Options.iGainCh1;    
    aDeviceParams.m_nControl =       GenerateControlFlags();
    aDeviceParams.m_nSyncLevel =     m_Options.iSyncLevel;  

    iErrCode = m_pDevice->Init(ADC_INIT_MODE_INIT, &aDeviceParams, NULL);
    if ( iErrCode <= 0 ) {
        return AdcError;
    }

    return AdcOk;
}

void CAdcEngine::Receive()
{
    SendPacket(AdcActiveChannelBoth);
}

AdcMemoryBlock* CAdcEngine::RequestBuffer( int32 iSize )
{
    if ( m_pWriter ) {
        return m_pWriter->RequestBuffer(iSize);
    }
   
    return NULL;
}

void CAdcEngine::OnData( AdcDeviceData pPacket )
{
    if ( m_pWriter ) {
        m_pWriter->OnData(pPacket);
    }
}

void CAdcEngine::Entry()
{
    if ( IsDataReady() ) {

        Receive(); 
        m_pDevice->Start();
    }
}

void CAdcEngine::GetCurrentOptions( AdcDeviceOptions& aOptions ) const
{
    aOptions = m_Options;
}

void CAdcEngine::Reset()
{
    if ( m_pDataThread ) {

        m_pDataThread->Delete();
        m_pDataThread = NULL;
    }
    m_pDevice->Stop();
}

void CAdcEngine::SendPacket( AdcActiveChannel aChannel )
{
    AdcDeviceData aPacket;
    uint32 uiMultiplier = (aChannel == AdcActiveChannelBoth) ? 2 : 1;
    aPacket.pBlock = RequestBuffer(m_Options.iWords * uiMultiplier);

    if ( aPacket.pBlock && aPacket.pBlock->pData ) {

        int32 iGetDataFlags = GenerateReceiveFlags( aChannel );
        int32 iReadedDataWords = m_pDevice->GetData( iGetDataFlags, (void*)aPacket.pBlock->pData, aPacket.pBlock->iSize, 0 );

        if ( aPacket.pBlock->iSize == m_Options.iWords * uiMultiplier ) {
            OnData(aPacket);
        } else {
            DEBUG_OUT("Device returns less data than requested\n");
        }
    }
}
