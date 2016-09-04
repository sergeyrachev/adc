#include "adc_settings_window.h"
#include "adc_main_frame.h"

CAdcSettingsWindow::AdcActiveChannelChoice CAdcSettingsWindow::CHANNEL[] = { {"Канал 0", CAdcEngine::AdcActiveChannel0}, {"Канал 1", CAdcEngine::AdcActiveChannel1}, {"Оба", CAdcEngine::AdcActiveChannelBoth} };
CAdcSettingsWindow::AdcSyncEntryChoice CAdcSettingsWindow::SYNC_ENTRY[] = { {"Канал 0", CAdcEngine::AdcSyncChannel0}, {"Канал 1", CAdcEngine::AdcSyncChannel1}, {"Внешний(TTL)", CAdcEngine::AdcSyncExternal }};
CAdcSettingsWindow::AdcStartModeChoice CAdcSettingsWindow::START_MODE[] = { {"Программный", CAdcEngine::AdcStartProgram}, {"Компаратор", CAdcEngine::AdcStartInternal}, {"Внешний", CAdcEngine::AdcStartExternal} };
CAdcSettingsWindow::AdcSyncSideChoice CAdcSettingsWindow::SYNC_SIDE[] = { {"Подъем", CAdcEngine::AdcSyncFront}, {"Спад", CAdcEngine::AdcSyncDecline} };

const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_DRIVER_PATH = "adc_driver_dll_path";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_FREQ_IDX = "adc_frequency_index";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_START_LEVEL = "adc_start_level";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_START_MODE_IDX = "adc_start_mode_idx";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_SYNC_SIDE_IDX = "adc_sync_side_idx";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_SYNC_ENTRY_IDX = "adc_sync_entry_idx";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_DATA_AMOUNT_IDX = "adc_data_amount_idx";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_GAIN_CH0_IDX = "adc_gain_0_idx";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_GAIN_CH1_IDX = "adc_gain_1_idx";
const char* CAdcSettingsWindow::CAdcOptionInternalStorage::ADC_CHANNEL_IDX = "adc_channel_idx";

double CAdcSettingsWindow::LIGHT_SPEED = 300000000.0f;

CAdcSettingsWindow::CAdcSettingsWindow( CAdcMainWindow* pOwner, const AdcOptionsInternal& aOptions, const CAdcEngine::AdcDeviceCapabilities& aCaps )
    : wxFrame(pOwner, wxID_ANY, "Настройки", wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxCLOSE_BOX | wxMINIMIZE_BOX) )
    , m_pOwner(pOwner)
{
    m_chStartMode = NULL;
    //m_chChannel = NULL;
    m_chDataAmount = NULL;
    m_chFrequency = NULL;
    m_chSyncEntry = NULL;
    m_chSyncSide = NULL;
    m_spStartLevel = NULL;
    m_pWords = NULL;
    m_pFrequency = NULL;
    m_chGain0 = NULL;
    m_chGain1 = NULL;

    InitMainPanel();

    m_Options = aOptions;
    UpdateUI(m_Options, aCaps);
}

void CAdcSettingsWindow::InitMainPanel()
{
    wxPanel* pPanel = new wxPanel(this, wxID_ANY);

    wxFlexGridSizer* pMainSizer = new wxFlexGridSizer(1);

    pMainSizer->AddGrowableCol(0);
    pMainSizer->AddGrowableRow(0);
    pPanel->SetSizer(pMainSizer);

    wxStaticBoxSizer* pOptionsSizer = new wxStaticBoxSizer(wxVERTICAL, pPanel, wxT("Параметры") );
    pMainSizer->Add(pOptionsSizer, 1, wxALL | wxEXPAND, 4);

    wxFlexGridSizer* pOptionsElemSizer = new wxFlexGridSizer(2);
    pOptionsSizer->Add( pOptionsElemSizer, 0, wxEXPAND );

    wxSize aSize(120,-1);

    wxStaticText* pStartModeStaticTxt = new wxStaticText(pPanel, wxID_ANY, wxT("Режим запуска:") );
    pOptionsElemSizer->Add(pStartModeStaticTxt, 0, wxALL, 4);
    m_chStartMode = new wxChoice( pPanel, wxID_ANY, wxDefaultPosition, aSize );
    pOptionsElemSizer->Add(m_chStartMode, 0, wxALL, 4);
    Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CAdcSettingsWindow::OnStartModeChange, this, m_chStartMode->GetId());
   
    wxStaticText* pFreqAvailableStaticTxt = new wxStaticText(pPanel, wxID_ANY, wxT("Разрешение:") );
    pOptionsElemSizer->Add(pFreqAvailableStaticTxt, 0, wxALL, 4);
    m_chFrequency = new wxChoice( pPanel, wxID_ANY, wxDefaultPosition, aSize );
    pOptionsElemSizer->Add(m_chFrequency, 0, wxALL, 4);
    Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CAdcSettingsWindow::OnFrequencyChange, this, m_chFrequency->GetId());


    wxStaticText* pAmountStaticTxt = new wxStaticText(pPanel, wxID_ANY, wxT("Высота:") );
    pOptionsElemSizer->Add(pAmountStaticTxt, 0, wxALL, 4);
    m_chDataAmount = new wxChoice( pPanel, wxID_ANY, wxDefaultPosition, aSize );
    pOptionsElemSizer->Add(m_chDataAmount, 0, wxALL, 4);
    Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CAdcSettingsWindow::OnAmountChange, this, m_chDataAmount->GetId());

    wxStaticText* pStartLevelStaticTxt = new wxStaticText(pPanel, wxID_ANY, wxT("Уровень синхр.:"));
    pOptionsElemSizer->Add(pStartLevelStaticTxt, 0, wxALL, 4);
    m_spStartLevel = new wxSpinCtrl(pPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, aSize, wxSP_ARROW_KEYS | wxALIGN_LEFT, -127, 127, 5);
    pOptionsElemSizer->Add(m_spStartLevel, 0, wxALL, 4);
    Bind( wxEVT_COMMAND_SPINCTRL_UPDATED, &CAdcSettingsWindow::OnStartLevelChange, this, m_spStartLevel->GetId());

    wxStaticText* pSyncEntryStaticTxt = new wxStaticText(pPanel, wxID_ANY, wxT("Вход синхр.:"));
    pOptionsElemSizer->Add(pSyncEntryStaticTxt, 0, wxALL, 4);
    m_chSyncEntry = new wxChoice( pPanel, wxID_ANY, wxDefaultPosition, aSize );
    pOptionsElemSizer->Add(m_chSyncEntry, 0, wxALL, 4);
    Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CAdcSettingsWindow::OnSyncEntryChange, this, m_chSyncEntry->GetId());

    wxStaticText* pGain0StaticTxt = new wxStaticText(pPanel, wxID_ANY, wxT("Усил. кан. 0:"));
    pOptionsElemSizer->Add(pGain0StaticTxt, 0, wxALL, 4);
    m_chGain0 = new wxChoice( pPanel, wxID_ANY, wxDefaultPosition, aSize );
    pOptionsElemSizer->Add(m_chGain0, 0, wxALL, 4);
    Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CAdcSettingsWindow::OnGain0Change, this, m_chGain0->GetId());

    wxStaticText* pGain1StaticTxt = new wxStaticText(pPanel, wxID_ANY, wxT("Усил. кан. 1:"));
    pOptionsElemSizer->Add(pGain1StaticTxt, 0, wxALL, 4);
    m_chGain1 = new wxChoice( pPanel, wxID_ANY, wxDefaultPosition, aSize );
    pOptionsElemSizer->Add(m_chGain1, 0, wxALL, 4);
    Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CAdcSettingsWindow::OnGain1Change, this, m_chGain1->GetId());

    wxStaticText* pSyncSideStaticTxt = new wxStaticText(pPanel, wxID_ANY, wxT("Фронт синхр.:"));
    pOptionsElemSizer->Add(pSyncSideStaticTxt, 0, wxALL, 4);
    m_chSyncSide = new wxChoice( pPanel, wxID_ANY, wxDefaultPosition, aSize );
    pOptionsElemSizer->Add(m_chSyncSide, 0, wxALL, 4);
    Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CAdcSettingsWindow::OnSyncSideChange, this, m_chSyncSide->GetId());
 
    wxButton* btnApply = new wxButton(pPanel, wxID_ANY, "Применить");
    pOptionsElemSizer->Add(btnApply, 0, wxALL | wxEXPAND, 4);
    Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CAdcSettingsWindow::OnApply, this, btnApply->GetId());

    wxButton* btnCancel = new wxButton(pPanel, wxID_ANY, "Отмена");
    pOptionsElemSizer->Add(btnCancel, 0, wxALL , 4);
    Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CAdcSettingsWindow::OnCancel, this, btnCancel->GetId());

    pMainSizer->SetSizeHints(this);
    pPanel->Layout();
}

void CAdcSettingsWindow::UpdateUI( const AdcOptionsInternal& aOptions, const CAdcEngine::AdcDeviceCapabilities& aCaps )
{
    m_iWords = aCaps.iWords;
    safe_delete_array(m_pWords);
    m_pWords = new int32[aCaps.iWords];
    memcpy(m_pWords, aCaps.pWords, sizeof(int32) * aCaps.iWords);

    safe_delete_array(m_pFrequency);
    m_pFrequency = new float[aCaps.iFrequency];
    memcpy(m_pFrequency, aCaps.pFrequency, sizeof(float) * aCaps.iFrequency);

    for ( int32 i = 0; i < sizeof(START_MODE) / sizeof(START_MODE[0]); i++ ) {

        m_chStartMode->Append( START_MODE[i].pStr );
    };
    m_chStartMode->SetSelection(aOptions.iStartModeIdx);

    for ( int32 i = 0; i < aCaps.iWords; i++ ) {

        double fHeight = LIGHT_SPEED / 2 / aCaps.pFrequency[m_Options.iFrequencyIdx] * aCaps.pWords[i];

        fHeight = ceil(fHeight / 1000) * 1000; 

        wxString sValue = wxString::Format("%7.0lf м.", fHeight);
        m_chDataAmount->Append(sValue);
    };
    m_chDataAmount->SetSelection(aOptions.iDataAmountIdx);

    for ( int32 i = 0; i < sizeof(SYNC_ENTRY) / sizeof(SYNC_ENTRY[0]); i++ ) {
        m_chSyncEntry->Append( SYNC_ENTRY[i].pStr );
    };
    m_chSyncEntry->SetSelection(aOptions.iSyncEntryIdx);

    for ( int32 i = 0; i < aCaps.iGain; i++ ) {

        wxString sValue = wxString::Format("x%6d.", aCaps.pGain[i]);
        m_chGain0->Append( sValue );
        m_chGain1->Append( sValue );
    };
    m_chGain0->SetSelection(aOptions.iGainCh0Idx);
    m_chGain1->SetSelection(aOptions.iGainCh1Idx);

    for ( int32 i = 0; i < sizeof(SYNC_SIDE) / sizeof(SYNC_SIDE[0]); i++ ) {
        m_chSyncSide->Append( SYNC_SIDE[i].pStr );
    };
    m_chSyncSide->SetSelection(aOptions.iSyncSideIdx);

    for ( int32 i = 0; i < aCaps.iFrequency; i++ ) {

        double fHeightResolution = LIGHT_SPEED / 2 / aCaps.pFrequency[i];
        wxString sValue = wxString::Format("%7.1lf м.", fHeightResolution);
        m_chFrequency->Append(sValue);
    };
    m_chFrequency->SetSelection(aOptions.iFrequencyIdx);

    m_spStartLevel->SetValue(aOptions.iStartLevel);
    UpdateHeight();
}

void CAdcSettingsWindow::OnStartModeChange(wxCommandEvent& event) 
{
    m_Options.iStartModeIdx = event.GetSelection();
}

void CAdcSettingsWindow::OnAmountChange(wxCommandEvent& event) 
{
    m_Options.iDataAmountIdx = event.GetSelection();
}

void CAdcSettingsWindow::OnChannelChange(wxCommandEvent& event) 
{
    m_Options.iChannelIdx = event.GetSelection();
}

void CAdcSettingsWindow::OnFrequencyChange(wxCommandEvent& event)
{
    m_Options.iFrequencyIdx = event.GetSelection();
    UpdateHeight();
}

void CAdcSettingsWindow::OnStartLevelChange(wxSpinEvent& event)
{
    m_Options.iStartLevel = event.GetValue();
}

void CAdcSettingsWindow::OnSyncEntryChange(wxCommandEvent& event)
{
    m_Options.iSyncEntryIdx = event.GetSelection();
}

void CAdcSettingsWindow::OnGain0Change(wxCommandEvent& event)
{
    m_Options.iGainCh0Idx = event.GetSelection();
}

void CAdcSettingsWindow::OnGain1Change(wxCommandEvent& event)
{
    m_Options.iGainCh1Idx = event.GetSelection();
}

void CAdcSettingsWindow::OnSyncSideChange(wxCommandEvent& event)
{
    m_Options.iSyncSideIdx = event.GetSelection();
}

void CAdcSettingsWindow::OnApply( wxCommandEvent& event )
{
    m_pOwner->OptionsApply(m_Options);
    safe_delete_array(m_pWords);
    safe_delete_array(m_pFrequency);
    Close(true);
}

void CAdcSettingsWindow::OnCancel( wxCommandEvent& event )
{
    m_pOwner->OptionsCancel(this);
    safe_delete_array(m_pWords);
    safe_delete_array(m_pFrequency);
    Close(true);
}

void CAdcSettingsWindow::UpdateHeight()
{
    m_chDataAmount->Clear();
    for ( int32 i = 0; i < m_iWords; i++ ) {

        double fHeight = LIGHT_SPEED / 2 / m_pFrequency[m_Options.iFrequencyIdx] * m_pWords[i];

        fHeight = ceil(fHeight / 1000) * 1000; 

        wxString sValue = wxString::Format("%7.0lf м.", fHeight);
        m_chDataAmount->Append(sValue);
    };
    m_chDataAmount->SetSelection(m_Options.iDataAmountIdx);
}
