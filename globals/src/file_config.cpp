#include "file_config_api.h"

#define CFG_DELIMITER '='
#define CFG_COMMENT '#'

CFileConfig::CFileConfig()
{
   
}

CFileConfig::~CFileConfig()
{

}

int32 CFileConfig::GetFileSize( FILE* pFile )
{
    int32 iSize = 0;

    fseek( pFile, 0L, SEEK_END );
    iSize = (int32)ftell( pFile );
    fseek( pFile, 0L, SEEK_SET );

    return iSize;
}

int32 CFileConfig::SkipComments( const int8 *pBuff, int32 aLen )
{
    int32 index = 0;
    while ( index < aLen && pBuff[index] != '\n' && pBuff[index] != '\r' ) {
        index++;
    };

    return index;
}

int32 CFileConfig::SkipEmptyLines( const int8 *pBuff, int32 aLen )
{
    int32 index = 0;
    while ( (pBuff[index] == '\r' || pBuff[index] == '\n') && index < aLen ) {
        index++;
    };

    return index;
}

int32 CFileConfig::ReadNumber( const int8 *pBuff, int32 aLen, int32 *aSkip )
{
    int32 index = 0;
    while ( pBuff[index] != CFG_DELIMITER && index < aLen ) {
        index++;
    };

    while ( pBuff[index] != '-' && !isdigit(pBuff[index]) && index < aLen ) {
        index++;
    }; 

    int32 iParamLen = 0;
    while ( pBuff[index] != '\r' && pBuff[index] != '\n' && index < aLen ) {

        iParamLen++;
        index++;
    };

    int8 csBuff[128] = {0};
    (*aSkip) = aLen;

    if ( index <= aLen && iParamLen < 128 ) {
        memcpy( csBuff, &pBuff[index - iParamLen], iParamLen );
    } else {
        return -1;
    }

    (*aSkip) = index;

    return atol((char*)csBuff);
}

int32 CFileConfig::ReadString( const int8 *pBuff, int32 aLen, int32 *aSkip, int32 *iParamPos, bool bIsSecureString /*= false*/ )
{
    int32 index = 0;
    while ( (pBuff[index] == ' ' || pBuff[index] == '\t' || pBuff[index] == CFG_DELIMITER) && index < aLen ) {
        index++;
    };

    int32 iParamLen = 0;
    (*iParamPos) = 0;
    while ( pBuff[index] != '\r' && pBuff[index] != '\n' && index < aLen ) {

        if ( bIsSecureString ) {

            while ( pBuff[index] == ' ' || pBuff[index] == '\t' ) {
                index++;
            };
        }

        if ( (*iParamPos) == 0 ) {
            (*iParamPos) = index;
        }

        iParamLen++;
        index++;
    };

    if ( index > aLen ) {
        return -1;
    }

    (*aSkip) = index;
    return iParamLen;
}

int32 CFileConfig::GetStringValue( const int8* pBuff, int32 aLen, const char* pParamName, char*& pParamValue )
{
    int32 len = 0;
    int32 skipLen = 0;

    if ( _strnicmp( (char*)pBuff, pParamName, strlen(pParamName) ) == 0 ) {

        len += (int32)strlen(pParamName);
        int32 iParamPos = 0;
        int32 offset = ReadString( &pBuff[len], (aLen - len), &skipLen, &iParamPos );

        if ( offset < aLen && offset > 0 && iParamPos < aLen ) {

            int32 iSafe = offset;
            if ( pBuff[iParamPos + len + iSafe] == 0 ){
                iSafe--;
            }
            while ( (isspace(pBuff[iParamPos + len + iSafe]) && iSafe > 0)) {
                --iSafe; 
            };
            offset -= (offset - iSafe) - 1;

            safe_delete_array(pParamValue);
            pParamValue = new char [offset + 1];
            memset(pParamValue, 0, (offset + 1) * sizeof(char));
            memcpy( pParamValue, (char*)&pBuff[iParamPos + len], offset );
            len += skipLen;
        }
    }

    return len;
}

int32 CFileConfig::GetNumberValue( const int8* pBuff, int32 aLen, const char* pParamName, int32& iParamValue )
{
    int32 len = 0;
    int32 skipLen = 0;

    if ( _strnicmp( (char*)pBuff, pParamName, strlen(pParamName) ) == 0 ) {

        len += (int32)strlen(pParamName);
        iParamValue = ReadNumber( &pBuff[len], (aLen - len), &skipLen );
        len += skipLen;
    }
    return len;
}

int32 CFileConfig::GetParam( const int8 *pBuff, int32 aLen )
{
    int32 len = 0;

//     len += GetStringValue( pBuff, aLen, <NAME_PARAM_STR>, <VAR_PARAM_STR> );
//     len += GetNumberValue( pBuff, aLen, <NAME_PARAM_STR>, <VAR_PARAM_NUM> );

    return aLen; //Shift over buffer
}

int32 CFileConfig::ParseData( const int8 *pBuff, int32 aLen )
{
    int32 index = 0;
    while ( index < aLen ) {

        index += SkipEmptyLines( &pBuff[index], (aLen - index) );
        if ( pBuff[index] == CFG_COMMENT ) {

            index += SkipComments( &pBuff[index], (aLen - index) );
            continue;
        } else if ( isalpha(pBuff[index]) ) {

            int32 iOffset = GetParam( &pBuff[index], (aLen - index) );
            if ( iOffset > 0 ) {

                index += iOffset;
                continue;
            }
        }

        index++;
    };

    return 0;
}

void CFileConfig::Read( const char* pConfigFile )
{
    FILE* pFile;
    pFile = fopen( (char*)pConfigFile, "rb" );
    if ( pFile ) {

        int32 iFileSize = GetFileSize( pFile );

        if ( iFileSize > 0 ) {

            int8 *pBuff = new int8[iFileSize + 1];
            memset(pBuff, 0, iFileSize + 1);

            fread( pBuff, 1, iFileSize, pFile );
            ParseData( pBuff, iFileSize );

            safe_delete_array(pBuff);
        }

        fclose(pFile);
        pFile = NULL;
    }
}
