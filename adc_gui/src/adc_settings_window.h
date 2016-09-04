#pragma once

#include "wx\wx.h"
#include "adc_engine.h"
#include "file_config_api.h"

class CAdcMainWindow;
class CAdcSettingsWindow : public wxFrame
{
    friend CAdcMainWindow;

    typedef struct _tagAdcActiveChannelChoice {
        const char* pStr;
        CAdcEngine::AdcActiveChannel iChannel;
    } AdcActiveChannelChoice;

    typedef struct _tagAdcSyncEntryChoice {
        const char* pStr;
        CAdcEngine::AdcSyncEntry iSyncEntry;
    } AdcSyncEntryChoice;

    typedef struct _tagAdcStartModeChoice {
        const char* pStr;
        CAdcEngine::AdcStartMode iStartMode;
    } AdcStartModeChoice;

    typedef struct _tagAdcSyncSideChoice {
        const char* pStr;
        CAdcEngine::AdcSyncSide iSyncSide;
    } AdcSyncSideChoice;

    static AdcActiveChannelChoice CHANNEL[];
    static AdcSyncEntryChoice SYNC_ENTRY[];
    static AdcStartModeChoice START_MODE[]; 
    static AdcSyncSideChoice SYNC_SIDE[];

    typedef struct _tagAdcOptionsInternal
    {
        int32 iStartModeIdx;
        int32 iChannelIdx;
        int32 iDataAmountIdx;
        int32 iFrequencyIdx;
        int32 iSyncEntryIdx;
        int32 iSyncSideIdx;
        int32 iGainCh0Idx;
        int32 iGainCh1Idx;
        int32 iStartLevel;
    } AdcOptionsInternal;

    class CAdcOptionInternalStorage;
    static double LIGHT_SPEED;

public:
    CAdcSettingsWindow( CAdcMainWindow* pOwner, const AdcOptionsInternal& aOptions, const CAdcEngine::AdcDeviceCapabilities& aCaps );

protected:
    void InitMainPanel();

    void OnStartModeChange(wxCommandEvent& event);
    void OnAmountChange(wxCommandEvent& event);
    void OnChannelChange(wxCommandEvent& event);
    void OnFrequencyChange(wxCommandEvent& event);
    void OnStartLevelChange(wxSpinEvent& event);
    void OnSyncEntryChange(wxCommandEvent& event);
    void OnSyncSideChange(wxCommandEvent& event);
    void OnGain0Change(wxCommandEvent& event);
    void OnGain1Change(wxCommandEvent& event);

    void OnApply(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    void UpdateUI(const AdcOptionsInternal& aOptions, const CAdcEngine::AdcDeviceCapabilities& aCaps);
    void UpdateHeight();
    
private:
    CAdcMainWindow* m_pOwner;
    AdcOptionsInternal m_Options;

    int32* m_pWords;
    int32 m_iWords;
    float* m_pFrequency;
    int32 m_iFrequency;

    wxChoice* m_chStartMode;
    wxChoice* m_chDataAmount;
    wxChoice* m_chFrequency;
    wxSpinCtrl* m_spStartLevel;
    wxChoice* m_chSyncEntry;
    wxChoice* m_chSyncSide;
    wxChoice* m_chGain0;
    wxChoice* m_chGain1;     
};

class CAdcSettingsWindow::CAdcOptionInternalStorage : public CFileConfig
{
    static const char* ADC_DRIVER_PATH;
    static const char* ADC_FREQ_IDX;
    static const char* ADC_START_LEVEL;
    static const char* ADC_SYNC_SIDE_IDX;
    static const char* ADC_SYNC_ENTRY_IDX;
    static const char* ADC_DATA_AMOUNT_IDX;
    static const char* ADC_CHANNEL_IDX;
    static const char* ADC_START_MODE_IDX;
    static const char* ADC_GAIN_CH0_IDX;
    static const char* ADC_GAIN_CH1_IDX;

public:
    CAdcOptionInternalStorage( const char* pOptionFile )
        : CFileConfig()
    {
        m_pDriverPath = NULL;
        m_pOptionFile = NULL;

        memset(&m_Options, 0, sizeof(CAdcSettingsWindow::AdcOptionsInternal));

        safe_copy_string(m_pOptionFile, pOptionFile);
    }

    ~CAdcOptionInternalStorage()
    {
        safe_delete_array(m_pOptionFile);
        safe_delete_array(m_pDriverPath);
    }

    bool Save( const char* pDriverPath, const CAdcSettingsWindow::AdcOptionsInternal& aOption)
    {
        FILE* pOptionFile = fopen(m_pOptionFile, "wb");
        if ( pOptionFile ) {

            safe_copy_string(m_pDriverPath, pDriverPath);
            m_Options = aOption;

            if ( m_pDriverPath ) {
                fprintf( pOptionFile, "%s = %s\n", ADC_DRIVER_PATH, m_pDriverPath );
            }

            fprintf( pOptionFile, "%s = %d\n", ADC_START_MODE_IDX, m_Options.iStartModeIdx );
            fprintf( pOptionFile, "%s = %d\n", ADC_FREQ_IDX, m_Options.iFrequencyIdx );
            fprintf( pOptionFile, "%s = %d\n", ADC_START_LEVEL, m_Options.iStartLevel );
            fprintf( pOptionFile, "%s = %d\n", ADC_SYNC_SIDE_IDX, m_Options.iSyncSideIdx );
            fprintf( pOptionFile, "%s = %d\n", ADC_SYNC_ENTRY_IDX, m_Options.iSyncEntryIdx );
            fprintf( pOptionFile, "%s = %d\n", ADC_DATA_AMOUNT_IDX, m_Options.iDataAmountIdx );
            fprintf( pOptionFile, "%s = %d\n", ADC_CHANNEL_IDX, m_Options.iChannelIdx );
            fprintf( pOptionFile, "%s = %d\n", ADC_GAIN_CH0_IDX, m_Options.iGainCh0Idx );
            fprintf( pOptionFile, "%s = %d\n", ADC_GAIN_CH1_IDX, m_Options.iGainCh1Idx );

            fclose(pOptionFile);
            return true;
        }

        return false;
    }

    bool Load( const char*& ppDriverPath, CAdcSettingsWindow::AdcOptionsInternal& aOption )
    {
        safe_delete_array(m_pDriverPath);
        Read(m_pOptionFile);

        if ( m_pDriverPath ) {
            ppDriverPath = m_pDriverPath;
            aOption = m_Options;
            return true;
        }

        return false;
    }

private:
    virtual int32 GetParam( const int8 *pBuff, int32 aLen )
    {
        int32 len = 0;
        len += GetStringValue( pBuff, aLen, ADC_DRIVER_PATH, m_pDriverPath );
        len += GetNumberValue( pBuff, aLen, ADC_START_MODE_IDX, m_Options.iStartModeIdx );
        len += GetNumberValue( pBuff, aLen, ADC_FREQ_IDX, m_Options.iFrequencyIdx );
        len += GetNumberValue( pBuff, aLen, ADC_START_LEVEL, m_Options.iStartLevel ); 
        len += GetNumberValue( pBuff, aLen, ADC_SYNC_SIDE_IDX, m_Options.iSyncSideIdx );
        len += GetNumberValue( pBuff, aLen, ADC_SYNC_ENTRY_IDX, m_Options.iSyncEntryIdx );
        len += GetNumberValue( pBuff, aLen, ADC_DATA_AMOUNT_IDX, m_Options.iDataAmountIdx );
        len += GetNumberValue( pBuff, aLen, ADC_CHANNEL_IDX, m_Options.iChannelIdx );
        len += GetNumberValue( pBuff, aLen, ADC_GAIN_CH0_IDX, m_Options.iGainCh0Idx );
        len += GetNumberValue( pBuff, aLen, ADC_GAIN_CH1_IDX, m_Options.iGainCh1Idx );

        return len;
    }

private:
    char* m_pOptionFile;
    char* m_pDriverPath;
    CAdcSettingsWindow::AdcOptionsInternal m_Options;
    int32 m_iWords;
};