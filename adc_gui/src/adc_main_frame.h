#pragma once

#include "adc_engine.h"

#include "file_config_api.h"

#include "wx\wx.h"
#include "wx\spinctrl.h"

#include "adc_plot.h"
#include "adc_settings_window.h"
#include "adc_simple_writer.h"

class CAdcMainWindow : public wxFrame, public CAdcEngine::IAdcInitCallback, public IAdcWriter, public wxThreadHelper
{
    static const int32 MAX_BLOCK_COUNT = 100;
public:
    CAdcMainWindow( );

    void OnClose(wxCloseEvent& event);
    void OnMenuQuit(wxCommandEvent& event);
    void OnMenuLoad(wxCommandEvent& event);
    void OnMenuSave(wxCommandEvent& event);

    void OnStop(wxCommandEvent& event);
    void OnRun(wxCommandEvent& event);

    void OnPacketCountChanged(wxSpinEvent& event);
    void OnEmitterDirectionChange(wxCommandEvent& event);
    void OnCollectorDirectionChange(wxCommandEvent& event);

    void OnSettings(wxCommandEvent& event);
    void OnTimerProgressUpdate(wxTimerEvent& event);

    void OptionsApply( const CAdcSettingsWindow::AdcOptionsInternal& m_Options );
    void OptionsCancel( CAdcSettingsWindow* pWindow );

    virtual void OnDeviceInit( AdcResult aInitResult );

    virtual void OnData( AdcDeviceData pPacket );
    virtual AdcMemoryBlock* RequestBuffer( int32 iSize );

private:
    void InitMenuBar();
    void InitToolBar();
    void InitStatusBar();
    void InitMainPanel();

    virtual wxThread::ExitCode Entry();
private:
    mpWindow* m_pGraph0;
    mpWindow* m_pGraph1;
    wxTextCtrl* m_txtLog;
    wxGauge* m_pProgress;

    wxToolBarToolBase* m_tbStart;
    wxToolBarToolBase* m_tbStartCount;
    wxToolBarToolBase* m_tbStartView;
    wxToolBarToolBase* m_tbStop;
    wxToolBarToolBase* m_tbOptions;
    wxComboBox* m_cbDisc1Combo; 
    wxComboBox* m_cbDisc2Combo;
    wxSpinCtrl* m_scPacketCount;

    CAdcSettingsWindow* m_pSettingsWindow;

    wxTimer* m_pTimerProgress;

private:
    CAdcSettingsWindow::AdcOptionsInternal m_Options;
    CAdcEngine* m_pEngine;
    IAdcWriter* m_pWriter;
    char* m_pDriverPath;

    wxCriticalSection m_csDataGuard;
    std::list<AdcDeviceData> m_lstCollectedData;
    int32 m_iEmitterState;
    int32 m_iCollectorState;
    bool m_bEnableCounter;
    bool m_bIsViewMode;
    int32 m_iPacketCount;
    int32 m_iMaxPacketCount;
    double m_fHeightResolution;
    int32 m_iGain0;
    int32 m_iGain1;
};
                                   