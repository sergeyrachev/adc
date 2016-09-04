#pragma once

#define ADC_DLL_IMPORT_NAME "StaticFactory"
#define ADC_DEVICE_INTERFACE "IADCDevice" 

typedef enum _tagADCAPPRESULT{
    AdcOk,
    AdcError
}AdcResult;

// IADCDevice* LoadDriver( const IFactory* pDllFactory, const TCHAR* pInterfaceName)
// {
//     char* pDriverName = NULL;
//     if( pADCDevice->Get(ADC_GET_NAME, &pDriverName) ) {
// 
//         printf("Driver: %s\n", pDriverName);                                  // TODO: Who must delete this buffer?
//         //safe_delete_array(pDriverName);
//     }
// 
//     float fMaxVoltage = 0.0f;
//     if( pADCDevice->Get(ADC_GET_RANGE_BIPOLAR, &fMaxVoltage) ) {
//         printf("Possible input voltage range: %0.3f\n", fMaxVoltage);
//     }
// 
//     int32 iMaxMSC;
//     if(pADCDevice->Get(ADC_GET_MAXAMP, &iMaxMSC)) {
//         printf("Maximum value of amplitude: %d\n", iMaxMSC);
//     }
// 
//     int32 iMinMSC;
//     if(pADCDevice->Get(ADC_GET_MINAMP, &iMinMSC)) {
//         printf("Minimum value of amplitude: %d\n", iMinMSC);
//     }
// 
//     float iMaxFreq;
//     if(pADCDevice->Get(ADC_GET_MAXFREQ, &iMaxFreq)) {
//         printf("Maximum value of timer frequency: %.0f\n", iMaxFreq);
// 
//     }
// 
//     float iMinFreq;
//     if(pADCDevice->Get(ADC_GET_MINFREQ, &iMinFreq)) {
//         printf("Minimum value of timer frequency: %.0f\n", iMinFreq);
// 
//     }
// 
//     int32 iChan;
//     if(pADCDevice->Get(ADC_GET_NCHANNEL, &iChan)) {
//         printf("Number of analog channels: %d\n", iChan);
//     }
// 
//     int32 iDataSize;
//     if(pADCDevice->Get(ADC_GET_DATASIZE, &iDataSize)) {
//         printf("Used byte to write one value: %d\n", iDataSize);
//     }
// 
//     int32 iDataBits;
//     if( pADCDevice->Get( ADC_GET_DATABITS, &iDataBits ) ) {
//         printf("Capacity of ADC data bits: %d\n", iDataBits);  
//     }
// 
//     printf("\n");
//     return pADCDevice;
// }

#include "globals_api.h"
#include "globals_defines.h"

class AdcMemoryBlock
{
public:
    AdcMemoryBlock(int16* p = NULL, int32 i = 0) :  pData(p), iSize(i){}

    const int16* pData;
    const int32  iSize;
};

class IAdcWriter
{
public:
    typedef struct _tagAdcDeviceData
    {
        AdcMemoryBlock* pBlock;
        int32 iNumber;
        int32 iEmitterDiskDegree;
        int32 iCollectorDiskDegree;
        double fHeightResolution;
        int32 iGain0;
        int32 iGain1;
    }AdcDeviceData;

public:
    virtual ~IAdcWriter() {};

    virtual AdcMemoryBlock* RequestBuffer( int32 iSize ) = 0;
    virtual void OnData( AdcDeviceData pPacket ) = 0;

    virtual void UnpackValue(int16 iRawValue, int16& iFlags, int16& iValue)
    {
        // Hardware BigEndian format 0x0001 = 1
        //iRawValue = swap16(iRawValue);  // LA_EMUL LittleEndian format 0x0100 = 1            

        int32 iValueSize = 12;
        int16 iSignBitIndex = 1 << (iValueSize - 1); 
        int16 iValueBitMask = (int16)((uint16)0xFFFF >> (16 - (iValueSize)));

        iFlags = (iRawValue & (int16)(~iValueBitMask) ) >> iValueSize;
        iValue = (( iRawValue & iValueBitMask) ^ iSignBitIndex) - iSignBitIndex;     // Expand sign bit
    }
};
