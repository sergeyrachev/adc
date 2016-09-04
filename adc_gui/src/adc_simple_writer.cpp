
#include "adc_simple_writer.h"

CAdcSimpleWriter::CAdcSimpleWriter()
{
    m_pFile = NULL;
}

CAdcSimpleWriter* CAdcSimpleWriter::Create( const char* pFilename )
{
    CAdcSimpleWriter* pWriter = new CAdcSimpleWriter;

    wxString sFilename(pFilename);

    if ( pWriter->m_pFile = fopen(pFilename, "wb") ) {
        return pWriter;
    }
    safe_delete(pWriter); 
    return NULL;
}

CAdcSimpleWriter::~CAdcSimpleWriter()
{
    fclose(m_pFile);
    m_pFile = NULL;
}

void CAdcSimpleWriter::OnData( AdcDeviceData aData )
{
    if ( m_pFile ) {

        if ( aData.pBlock->pData ) {

            int16 iFlags0 = aData.iEmitterDiskDegree;
            int16 iFlags1 = aData.iCollectorDiskDegree;

            char pOutput[4096];

            sprintf(pOutput, "n;%0d;h;%lf;x;%d;x;%d;", aData.iNumber, aData.fHeightResolution, aData.iGain0, aData.iGain1);
            
            for ( int16 j = 15 - 12;  j >= 0; j-- ) {
                sprintf(pOutput+strlen(pOutput), "%-1d", (iFlags0>>j) & 1 );
            };
            sprintf(pOutput+strlen(pOutput), ";"  );

            for ( int16 j = 15 - 12;  j >= 0; j-- ) {
                sprintf(pOutput+strlen(pOutput), "%-1d", (iFlags1>>j) & 1 );
            };
            sprintf(pOutput+strlen(pOutput), ";\r\n"  );

            fwrite(pOutput, 1, strlen(pOutput), m_pFile);

            for ( int32 i = 0, iCount=0; i < aData.pBlock->iSize; i+=2, iCount++ ) {

                sprintf(pOutput, "#;%9.4lf", iCount * aData.fHeightResolution);

                int16 iRawValue0 = aData.pBlock->pData[i];  
                int16 iRawValue1 = aData.pBlock->pData[i+1]; 
                int16 iFlags0 = 0;
                int16 iFlags1 = 0;
                int16 iValue0 = 0;
                int16 iValue1 = 0;

                UnpackValue(iRawValue0, iFlags0, iValue0);
                UnpackValue(iRawValue1, iFlags1, iValue1);

                sprintf(pOutput+strlen(pOutput), ";%5.1d", abs(iValue0) );
                sprintf(pOutput+strlen(pOutput), ";%5.1d", abs(iValue1) );

                sprintf(pOutput+strlen(pOutput), ";\r\n"  );

                fwrite(pOutput, strlen(pOutput), 1, m_pFile);
            };

            fflush(m_pFile);
        }
    }
}

AdcMemoryBlock* CAdcSimpleWriter::RequestBuffer( int32 iSize )
{
    return NULL;
}
