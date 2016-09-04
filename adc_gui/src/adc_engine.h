#pragma once 

#include "wx\wx.h"
#include "globals_api.h"
#include "configuration.h"

#include "ADC_CONST.h"
#include "IADCDevice.h"

struct IADCDevice;
class DllClient;

class CAdcEngine : public IAdcWriter
{
public:
    typedef enum _tagAdcStartMode {

        AdcStartProgram = ADCPARAM_START_PROGRAMM,
        AdcStartExternal = ADCPARAM_START_EXT,
        AdcStartInternal = ADCPARAM_START_COMP
    } AdcStartMode;

    typedef enum _tagAdcSyncEntry
    {
        AdcSyncChannel0 = ADC_CONTROL_SYNC_CH0,
        AdcSyncChannel1 = ADC_CONTROL_SYNC_CH1,
        AdcSyncExternal = ADC_CONTROL_SYNC_TTL
    } AdcSyncEntry;

    typedef enum _tagAdcSyncSide
    {
        AdcSyncFront = ADC_CONTROL_SYNC_FRONT,
        AdcSyncDecline = ADC_CONTROL_SYNC_DECLINE
    } AdcSyncSide;

    typedef enum _tagAdcActiveChannel
    {
        AdcActiveChannel0 = ADC_DATA_MODE_DATACH0,
        AdcActiveChannel1 = ADC_DATA_MODE_DATACH1,
        AdcActiveChannelBoth = ADC_DATA_MODE_DATABOTH
    } AdcActiveChannel;

    typedef struct _tagAdcDeviceCapabilities {

        const int32* pWords;
        int32 iWords;
        const float* pFrequency;
        int32 iFrequency;
        const int32* pGain;
        int32 iGain;

        float fMaxVoltage;
        int32 iMaxTick;
        int32 iBitPerValue;
    } AdcDeviceCapabilities;

    typedef struct _tagAdcDeviceOptions {

        int32 iWords;            
        float fFrequency;            
        AdcActiveChannel iChannels;             
        AdcStartMode iStartMode;
        int32 iGainCh0;
        int32 iGainCh1;
        int32 iSyncLevel;
        AdcSyncEntry iSyncEntry;
        AdcSyncSide  iSyncSide;
    } AdcDeviceOptions;

    class IAdcInitCallback
    {
    public:
        virtual ~IAdcInitCallback(){};
        virtual void OnDeviceInit( AdcResult aInitResult ) = 0;
    };

protected:
    class AdcInitThread : public wxThread
    {
        friend CAdcEngine;
    public:
        AdcInitThread(CAdcEngine* pEngine, IAdcInitCallback* pInitCallback)
            : wxThread(wxTHREAD_DETACHED)
        {
            m_pEngine = pEngine;
            m_pInitCallback = pInitCallback;
        }
        virtual wxThread::ExitCode Entry()
        {
            if ( m_pInitCallback && m_pEngine ) {
                m_pInitCallback->OnDeviceInit(m_pEngine->InternalInit());
            }
            return (wxThread::ExitCode)0;
        }
    protected:
        CAdcEngine* m_pEngine;
        IAdcInitCallback* m_pInitCallback;
    };

    class AdcDataThread : public wxThread
    {
        friend CAdcEngine;
    public:
        AdcDataThread(CAdcEngine* pEngine)
            : wxThread(wxTHREAD_DETACHED)
        {
            m_pEngine = pEngine;

        }

        virtual ~AdcDataThread() {

        }                    

        virtual wxThread::ExitCode Entry()
        {
            while ( !TestDestroy() ) {

                if ( m_pEngine ) {
                    m_pEngine->Entry();
                }
            };

            return (wxThread::ExitCode)0;
        }
    protected:
        CAdcEngine* m_pEngine;
    };

protected:
    CAdcEngine( IAdcWriter* pWriter );
    AdcResult InternalInit();
    void Reset();
    
    int32 GenerateControlFlags();
    int32 GenerateReceiveFlags( AdcActiveChannel iChannel );
    
    void Receive();

    void SendPacket( AdcActiveChannel aChannel );

    bool IsDataReady();
    //ReceiveThread
    void Entry();

    virtual void OnData( AdcDeviceData pPacket );
    virtual AdcMemoryBlock* RequestBuffer( int32 iSize );

public:
    static CAdcEngine* Create(const char* pDllPath, IAdcWriter* pWriter);
    virtual ~CAdcEngine();

    AdcResult Init(const AdcDeviceOptions& aOptions, IAdcInitCallback* pInitCallback);   // Long operation - use separate thread
    AdcResult GetCapabilities( AdcDeviceCapabilities& aCapabilities );
    void GetCurrentOptions( AdcDeviceOptions& aOptions ) const;

    void Start();
    void Stop();
    
private:
    DllClient* m_pDll;
    IADCDevice* m_pDevice;
    AdcDeviceOptions m_Options;
    IAdcWriter* m_pWriter;
    AdcDataThread* m_pDataThread;
};