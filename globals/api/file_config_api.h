#if !defined(_FILE_CONFIG_H_)
#define _FILE_CONFIG_H_

#include "globals_api.h"

class CFileConfig 
{
public:
    CFileConfig();  
    virtual ~CFileConfig();
    void Read(const char* pConfigFile);

protected:
    virtual int32 GetParam( const int8 *pBuff, int32 aLen );
    
    int32 GetStringValue( const int8* pBuff, int32 aLen, const char* pParamName, char*& pParamValue );
    int32 GetNumberValue( const int8* pBuff, int32 aLen, const char* pParamName, int32& uiParamValue );

    int32 SkipComments( const int8 *pBuff, int32 aLen );
    int32 SkipEmptyLines( const int8 *pBuff, int32 aLen );
    int32 ReadNumber( const int8 *pBuff, int32 aLen, int32 *aSkip );
    int32 ReadString( const int8 *pBuff, int32 aLen, int32 *aSkip, int32 *iParamPos, bool bIsAddress = false );
    int32 ParseData( const int8 *pBuff, int32 aLen );
    int32 GetFileSize( FILE* pFile );
};

#endif