#include "globals_api.h"

#include "adc_main_frame.h"

#include "wx/sizer.h"
#include "wx/wfstream.h"

#include "icons.xpm"
#include <vector>

CAdcMainWindow::CAdcMainWindow()
    : wxFrame(NULL, wxID_ANY, "ADC Viewer")
{
    memset(&m_Options, 0, sizeof(CAdcSettingsWindow::AdcOptionsInternal )) ;

    m_iEmitterState = 0;
    m_iCollectorState = 0;

    m_pDriverPath = NULL;
    m_pEngine = NULL;
    m_pWriter = NULL;
    m_pSettingsWindow = NULL;
    m_pProgress = NULL;
    m_txtLog = NULL;
    m_pGraph0 = NULL;
    m_pGraph1 = NULL;

    m_bEnableCounter = false;
    m_bIsViewMode = false;
    m_iPacketCount = 0;
    m_iMaxPacketCount = 100;
    m_fHeightResolution = 0.0;

    InitMenuBar();
    InitToolBar();
    InitStatusBar();
    InitMainPanel();

    m_pTimerProgress = new wxTimer(this);
    Bind(wxEVT_TIMER, &CAdcMainWindow::OnTimerProgressUpdate, this);
    m_pTimerProgress->Start(100);

    Bind(wxEVT_CLOSE_WINDOW, &CAdcMainWindow::OnClose, this );

    const char* pDriverPath = NULL;
    CAdcSettingsWindow::CAdcOptionInternalStorage* pOptionStorage = new CAdcSettingsWindow::CAdcOptionInternalStorage("settings.ini");
    if ( pOptionStorage->Load(pDriverPath, m_Options) ) {

        safe_copy_string(m_pDriverPath, pDriverPath);

        m_pEngine = CAdcEngine::Create(m_pDriverPath, this);
        OptionsApply(m_Options);
        GetToolBar()->EnableTool(m_tbOptions->GetId(), true);
    } 
    safe_delete(pOptionStorage);

    if ( CreateThread() == wxTHREAD_NO_ERROR ) {
        GetThread()->Run();
    }

    Show();
}                

void CAdcMainWindow::OnClose( wxCloseEvent& e )
{
    if ( GetThread() ) {
        GetThread()->Delete();
    }

    safe_delete(m_pTimerProgress);
    safe_delete(m_pEngine);
    safe_delete(m_pWriter);
    safe_delete_array(m_pDriverPath);
    
    e.Skip();
}

                    
void CAdcMainWindow::OnMenuQuit( wxCommandEvent& event )
{
    Close(true);
}

void CAdcMainWindow::InitMenuBar()
{
    wxMenu *pMenu = new wxMenu;
    wxMenuItem* pItem = NULL;

    pItem = pMenu->Append(wxID_ANY, wxT("&Загрузить драйвер..."));
    Bind( wxEVT_COMMAND_MENU_SELECTED, &CAdcMainWindow::OnMenuLoad, this, pItem->GetId());
    pMenu->AppendSeparator();

    pItem = pMenu->Append(wxID_ANY, wxT("&Сменить файл результатов..."));
    Bind( wxEVT_COMMAND_MENU_SELECTED, &CAdcMainWindow::OnMenuSave, this, pItem->GetId());
    pMenu->AppendSeparator();

    pItem = pMenu->Append(wxID_EXIT, wxT("&Выход"));
    Bind(wxEVT_COMMAND_MENU_SELECTED, &CAdcMainWindow::OnMenuQuit, this, pItem->GetId());

    wxMenuBar *pMenuBar = new wxMenuBar;
    pMenuBar->Append(pMenu, "&Главное");
    SetMenuBar(pMenuBar);
}

void CAdcMainWindow::InitToolBar()
{
    int32 iTbStyle =  wxTB_HORIZONTAL;
    wxToolBar* pToolBar = CreateToolBar( iTbStyle );

    wxToolBarToolBase* pItem = NULL;
    
    pItem = pToolBar->AddTool(wxID_ANY, wxT("Запуск!"), wxBitmap(pxpm_run_program), wxBitmap(pxpm_run_program_disable), wxITEM_NORMAL, wxT("Запуск преобразования..."), wxT("Запуск преобразования...") );
    Bind( wxEVT_COMMAND_TOOL_CLICKED, &CAdcMainWindow::OnRun, this, pItem->GetId());
    m_tbStart = pItem;
    m_tbStart->Enable(false);

    pItem = pToolBar->AddTool(wxID_ANY, wxT("Запуск с отсчётом пакетов!"), wxBitmap(pxpm_run_counter), wxBitmap(pxpm_run_counter_disable), wxITEM_NORMAL, wxT("Запуск преобразования с отсчётом пакетов..."), wxT("Запуск преобразования с отсчётом пакетов...") );
    Bind( wxEVT_COMMAND_TOOL_CLICKED, &CAdcMainWindow::OnRun, this, pItem->GetId());
    m_tbStartCount = pItem;
    m_tbStartCount->Enable(false);
    
    pItem = pToolBar->AddTool(wxID_ANY, wxT("Запуск в режиме осциллографа!"), wxBitmap(pxpm_run_view), wxBitmap(pxpm_run_view_disable), wxITEM_NORMAL, wxT("Запуск осциллографа..."), wxT("Запуск осциллографа...") );
    Bind( wxEVT_COMMAND_TOOL_CLICKED, &CAdcMainWindow::OnRun, this, pItem->GetId());
    m_tbStartView = pItem;
    m_tbStartView->Enable(false);
        
    pToolBar->AddSeparator();

    pItem = pToolBar->AddTool(wxID_ANY, wxT("Остановка!"), wxBitmap(pxpm_stop), wxBitmap(pxpm_stop_disable), wxITEM_NORMAL, wxT("Остановить"), wxT("Остановить...") );
    Bind( wxEVT_COMMAND_TOOL_CLICKED, &CAdcMainWindow::OnStop, this, pItem->GetId());
    m_tbStop = pItem;
    m_tbStop->Enable(false);

    pToolBar->AddStretchableSpace();

    wxSize labelSize(100, -1);
    wxStaticText* pDisk1Desc = new wxStaticText( pToolBar, wxID_ANY, wxT("Диск излучателя:"), wxDefaultPosition, labelSize);
    pToolBar->AddControl(pDisk1Desc);
    
    m_cbDisc1Combo = new wxComboBox(pToolBar, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(50, -1), 0, 0, wxCB_DROPDOWN | wxCB_READONLY );
    m_cbDisc1Combo->Append(wxT("0°"));
    m_cbDisc1Combo->Append(wxT("90°"));
    m_cbDisc1Combo->Append(wxT("180°"));
    m_cbDisc1Combo->Append(wxT("270°"));
    m_cbDisc1Combo->SetSelection(m_iEmitterState);
    pItem = pToolBar->AddControl(m_cbDisc1Combo);
    Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CAdcMainWindow::OnEmitterDirectionChange, this, m_cbDisc1Combo->GetId());

    pToolBar->AddSeparator();

    wxStaticText* pPacketCountDesc = new wxStaticText( pToolBar, wxID_ANY, wxT("Кол-во пакетов:"), wxDefaultPosition, labelSize);
    pToolBar->AddControl(pPacketCountDesc);

    m_scPacketCount = new wxSpinCtrl(pToolBar, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS | wxTE_PROCESS_ENTER | wxTE_PROCESS_TAB, 1, 1000, m_iMaxPacketCount);
    pItem = pToolBar->AddControl(m_scPacketCount);
    Bind( wxEVT_COMMAND_SPINCTRL_UPDATED, &CAdcMainWindow::OnPacketCountChanged, this, m_scPacketCount->GetId());

    pToolBar->AddSeparator();

    wxStaticText* pDisk2Desc = new wxStaticText( pToolBar, wxID_ANY, wxT("Диск приёмника:"), wxDefaultPosition, labelSize);
    pToolBar->AddControl(pDisk2Desc);

    m_cbDisc2Combo = new wxComboBox(pToolBar, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(50, -1), 0, 0, wxCB_DROPDOWN | wxCB_READONLY );
    m_cbDisc2Combo->Append(wxT("0°"));
    m_cbDisc2Combo->Append(wxT("90°"));
    m_cbDisc2Combo->Append(wxT("180°"));
    m_cbDisc2Combo->Append(wxT("270°"));
    m_cbDisc2Combo->SetSelection(m_iCollectorState);
    pItem = pToolBar->AddControl(m_cbDisc2Combo);
    Bind( wxEVT_COMMAND_COMBOBOX_SELECTED, &CAdcMainWindow::OnCollectorDirectionChange, this, m_cbDisc2Combo->GetId());

    pToolBar->AddStretchableSpace();

    pItem = pToolBar->AddTool(wxID_ANY, wxT("Параметры"), wxBitmap(pxpm_options), wxBitmap(pxpm_options), wxITEM_NORMAL, wxT("Параметры преобразования..."), wxT("Дополнительные настройки") );
    Bind( wxEVT_COMMAND_TOOL_CLICKED, &CAdcMainWindow::OnSettings, this, pItem->GetId());
    m_tbOptions = pItem;
    m_tbOptions->Enable(false);

    pToolBar->Realize();
}

void CAdcMainWindow::InitStatusBar()
{
    wxStatusBar* pStatusBar = CreateStatusBar(3);

    int32 iFieldsSize[] = {-1, -2, 120};
    pStatusBar->SetStatusWidths(3, iFieldsSize);

    wxRect aBarField;
    pStatusBar->GetFieldRect(2, aBarField);

    m_pProgress = new wxGauge(pStatusBar, wxID_ANY, MAX_BLOCK_COUNT, aBarField.GetPosition() );
    m_pProgress->Show(true);
   
    pStatusBar->SetStatusText("Загрузите библиотеку драйвера устройства через главное меню...");
}

void CAdcMainWindow::InitMainPanel()
{
    wxPanel* pPanel = new wxPanel(this, wxID_ANY);

    wxFlexGridSizer* pMainSizer = new wxFlexGridSizer(1);

    pMainSizer->AddGrowableCol(0);
    pMainSizer->AddGrowableRow(0);
    pPanel->SetSizer(pMainSizer);

    wxStaticBoxSizer* pGraph0Sizer = new wxStaticBoxSizer(wxVERTICAL, pPanel, wxT("Канал 0"));
    wxStaticBoxSizer* pGraph1Sizer = new wxStaticBoxSizer(wxVERTICAL, pPanel, wxT("Канал 1"));

    wxFlexGridSizer* pGraphSizer = new wxFlexGridSizer(2);
    pGraphSizer->AddGrowableRow(0);
    pGraphSizer->AddGrowableCol(0);
    pGraphSizer->AddGrowableCol(1);

    pGraphSizer->Add(pGraph0Sizer, 1, wxALL | wxEXPAND, 4);
    pGraphSizer->Add(pGraph1Sizer, 1, wxALL | wxEXPAND, 4);

    pMainSizer->Add(pGraphSizer, 1, wxALL | wxEXPAND, 4);

    wxStaticBoxSizer* pLogSizer = new wxStaticBoxSizer(wxVERTICAL, pPanel, wxT("Журнал событий"));
    pMainSizer->Add(pLogSizer, 1, wxALL | wxEXPAND, 4);

    m_txtLog = new wxTextCtrl(pPanel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(720, 180), wxTE_MULTILINE);
    pLogSizer->Add( m_txtLog, 1, wxEXPAND );

    m_pGraph0 = new mpWindow( pPanel, wxID_ANY ); 
    m_pGraph1 = new mpWindow( pPanel, wxID_ANY ); 
    pGraph0Sizer->Add( m_pGraph0, 1, wxALL | wxEXPAND );
    pGraph1Sizer->Add( m_pGraph1, 1, wxALL | wxEXPAND );

    mpFXYVector* pSignalLayer0 = new mpFXYVector("signal0");
    pSignalLayer0->SetContinuity(false);

    wxPen aPen0(*wxBLUE);
    pSignalLayer0->SetPen(aPen0);
    pSignalLayer0->SetDrawOutsideMargins(false);

    mpFXYVector* pSignalLayer1 = new mpFXYVector("signal1");

    pSignalLayer1->SetContinuity(false);
    wxPen aPen1(*wxRED);
    pSignalLayer1->SetPen(aPen1);
    pSignalLayer1->SetDrawOutsideMargins(false);

    wxFont aFont(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    mpScaleX* pAxisX0 = new mpScaleX("Уровень сигнала, отсч.", mpALIGN_BOTTOM, true, mpX_NORMAL);
    mpScaleY* pAxisY0 = new mpScaleY("Высота, м", mpALIGN_LEFT, true);
    mpScaleX* pAxisX1 = new mpScaleX("Уровень сигнала, отсч.", mpALIGN_BOTTOM, true, mpX_NORMAL);
    mpScaleY* pAxisY1 = new mpScaleY("Высота, м", mpALIGN_LEFT, true);

    pAxisX0->SetFont(aFont);
    pAxisY0->SetFont(aFont);
    pAxisX0->SetDrawOutsideMargins(false);
    pAxisY0->SetDrawOutsideMargins(false);
    pAxisX1->SetFont(aFont);
    pAxisY1->SetFont(aFont);
    pAxisX1->SetDrawOutsideMargins(false);
    pAxisY1->SetDrawOutsideMargins(false);
    
    m_pGraph0->SetMargins(30, 30, 50, 100);
    m_pGraph1->SetMargins(30, 30, 50, 100);

    m_pGraph0->AddLayer(pAxisX0);
    m_pGraph0->AddLayer(pAxisY0);
    m_pGraph1->AddLayer(pAxisX1);
    m_pGraph1->AddLayer(pAxisY1);
    m_pGraph0->AddLayer(pSignalLayer0);
    m_pGraph1->AddLayer(pSignalLayer1);

    m_pGraph0->EnableDoubleBuffer(true);
    m_pGraph1->EnableDoubleBuffer(true);
    m_pGraph0->SetMPScrollbars(false);
    m_pGraph1->SetMPScrollbars(false);

    pMainSizer->SetSizeHints(this);
    pPanel->Layout();
}
 
void CAdcMainWindow::OnStop(wxCommandEvent& event)
{
    m_cbDisc1Combo->Enable(true);
    m_scPacketCount->Enable(true);
    m_cbDisc2Combo->Enable(true);

    GetToolBar()->EnableTool(m_tbStart->GetId(), true);
    GetToolBar()->EnableTool(m_tbStartCount->GetId(), true);
    GetToolBar()->EnableTool(m_tbStartView->GetId(), true);
    GetToolBar()->EnableTool(m_tbStop->GetId(), false);
    GetToolBar()->EnableTool(m_tbOptions->GetId(), true); 
              
    if ( m_pEngine ) {
        m_pEngine->Stop();
    }
}

void CAdcMainWindow::OnRun(wxCommandEvent& event)
{
    m_cbDisc1Combo->Enable(false);
    m_scPacketCount->Enable(false);
    m_cbDisc2Combo->Enable(false);

    GetToolBar()->EnableTool(m_tbStart->GetId(), false);
    GetToolBar()->EnableTool(m_tbStartCount->GetId(), false);
    GetToolBar()->EnableTool(m_tbStartView->GetId(), false);
    GetToolBar()->EnableTool(m_tbStop->GetId(), true);
    GetToolBar()->EnableTool(m_tbOptions->GetId(), false);

    m_bIsViewMode = event.GetId() == m_tbStartView->GetId();
    m_bEnableCounter = event.GetId() == m_tbStartCount->GetId();
    if (!m_bEnableCounter)
    {
        m_iEmitterState = 0;
        m_iCollectorState = 0;

        m_cbDisc1Combo->SetSelection(m_iEmitterState);
        m_cbDisc2Combo->SetSelection(m_iCollectorState);
    }

    m_iPacketCount = 0;
    if ( m_pEngine ) {
        m_pEngine->Start();
    }
}                     

void CAdcMainWindow::OnMenuLoad( wxCommandEvent& event )
{
    safe_delete(m_pEngine);

    do {

        wxFileDialog openFileDialog( this, wxT("Загрузка драйвера устройства"), "", "", "DLL драйвера устройства (*.dll)|*.dll", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if ( openFileDialog.ShowModal() == wxID_CANCEL ) {
            return;
        }   
        
        safe_copy_string(m_pDriverPath, openFileDialog.GetPath().c_str().AsChar()); 
        m_pEngine = CAdcEngine::Create(m_pDriverPath, this);

    } while(!m_pEngine);
    
    GetToolBar()->EnableTool(m_tbOptions->GetId(), true);
    GetStatusBar()->SetStatusText(wxT("Driver load successfull"), 0);
}

void CAdcMainWindow::OnMenuSave( wxCommandEvent& event )
{
    wxFileDialog saveFileDialog(this, wxT("Сохранение результатов преобразования"), "", "", "Текстовые файлы результатов (*.txt)|*.txt", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if ( saveFileDialog.ShowModal() == wxID_CANCEL ) {
        return;
    }

    safe_delete(m_pWriter);
    m_pWriter = CAdcSimpleWriter::Create(saveFileDialog.GetPath().c_str().AsChar());

    if ( !m_pWriter ) {
        DEBUG_OUT("File %s inaccessible\n", saveFileDialog.GetPath().c_str().AsChar());
    }
}

void CAdcMainWindow::OnTimerProgressUpdate( wxTimerEvent& event )
{
    int32 iBufferOccurancy = 0;
    {
        wxCriticalSectionLocker lck(m_csDataGuard);
        iBufferOccurancy = m_lstCollectedData.size();
    }
    m_pProgress->SetValue( iBufferOccurancy );
}

void CAdcMainWindow::OnSettings( wxCommandEvent& event )
{
    bool bIsEmpty = false;
    {
        wxCriticalSectionLocker lck(m_csDataGuard);
        bIsEmpty = m_lstCollectedData.empty();
    }

    if ( m_pEngine && bIsEmpty ) {

        CAdcEngine::AdcDeviceCapabilities aDeviceCaps;
        m_pEngine->GetCapabilities(aDeviceCaps);

        if ( !m_pSettingsWindow ) {
            m_pSettingsWindow = new CAdcSettingsWindow(this, m_Options, aDeviceCaps);
        }
        
        m_pSettingsWindow->Show(true);
        m_pSettingsWindow->SetFocus();
    }
}

void CAdcMainWindow::OptionsApply( const CAdcSettingsWindow::AdcOptionsInternal& aOptions )
{
    int32 iWords = 0;
    int32 iValue = 0;
    m_Options = aOptions;
    if ( m_pEngine ) {

        CAdcEngine::AdcDeviceOptions aDeviceOptions;
        m_pEngine->GetCurrentOptions(aDeviceOptions);

        CAdcEngine::AdcDeviceCapabilities aDeviceCaps;
        m_pEngine->GetCapabilities(aDeviceCaps);

        aDeviceOptions.iStartMode = CAdcSettingsWindow::START_MODE[aOptions.iStartModeIdx].iStartMode;
        aDeviceOptions.iWords = aDeviceCaps.pWords[aOptions.iDataAmountIdx];
        aDeviceOptions.iChannels = CAdcSettingsWindow::CHANNEL[aOptions.iChannelIdx].iChannel;    
        aDeviceOptions.fFrequency = aDeviceCaps.pFrequency[aOptions.iFrequencyIdx];
        aDeviceOptions.iSyncLevel = aOptions.iStartLevel;
        aDeviceOptions.iSyncEntry = CAdcSettingsWindow::SYNC_ENTRY[aOptions.iSyncEntryIdx].iSyncEntry;
        aDeviceOptions.iSyncSide = CAdcSettingsWindow::SYNC_SIDE[aOptions.iSyncSideIdx].iSyncSide;
        aDeviceOptions.iGainCh0 = m_iGain0 = aDeviceCaps.pGain[aOptions.iGainCh0Idx];
        aDeviceOptions.iGainCh1 = m_iGain1 = aDeviceCaps.pGain[aOptions.iGainCh1Idx];
        m_pEngine->Init(aDeviceOptions, this);

        m_fHeightResolution =  CAdcSettingsWindow::LIGHT_SPEED / 2 / aDeviceCaps.pFrequency[aOptions.iFrequencyIdx];

        safe_delete(m_pWriter);
        wxString strFilename = wxDateTime::Now().Format("%Y-%m-%dT%H-%M-%S") + wxString(".txt");
        m_pWriter = CAdcSimpleWriter::Create( strFilename.c_str().AsChar() );
        GetStatusBar()->SetStatusText(wxT("Устройство готово..."), 0);

        iWords = aDeviceCaps.pWords[aOptions.iDataAmountIdx];
        iValue = aDeviceCaps.iMaxTick;
    }

    std::vector<double> vecHeight;
    std::vector<double> vecValues(iWords, iValue);
    vecValues[0] = 0;

    for ( int32 i = 0; i < iWords; i++ ) {
        vecHeight.push_back(i * m_fHeightResolution);
    };

    mpFXYVector* pGraph0 = (mpFXYVector*)m_pGraph0->GetLayerByName("signal0");
    pGraph0->Clear();
    pGraph0->SetData(vecValues, vecHeight);
    m_pGraph0->Fit();
    m_pGraph0->UpdateAll();

    mpFXYVector* pGraph1 = (mpFXYVector*)m_pGraph1->GetLayerByName("signal1");
    pGraph1->Clear();
    pGraph1->SetData(vecValues, vecHeight);
    m_pGraph1->Fit();
    m_pGraph1->UpdateAll();

    m_pSettingsWindow = NULL;
    CAdcSettingsWindow::CAdcOptionInternalStorage* pOptionStorage = new CAdcSettingsWindow::CAdcOptionInternalStorage("settings.ini");
    pOptionStorage->Save(m_pDriverPath, m_Options);

    safe_delete(pOptionStorage);
}

void CAdcMainWindow::OnDeviceInit( AdcResult aInitResult )
{
    m_txtLog->AppendText( wxString::Format("Инициализация: %s %d\n", aInitResult ? "Неудача, код ошибки" : "Успех", aInitResult));
    GetToolBar()->EnableTool(m_tbStart->GetId(), true);
    GetToolBar()->EnableTool(m_tbStartCount->GetId(), true);
    GetToolBar()->EnableTool(m_tbStartView->GetId(), true);
}

AdcMemoryBlock* CAdcMainWindow::RequestBuffer( int32 iSize )
{
    AdcMemoryBlock* pData = new AdcMemoryBlock(new int16[iSize], iSize);
    return pData;
}

void CAdcMainWindow::OnData( AdcDeviceData pPacket )
{
    pPacket.iEmitterDiskDegree = m_iEmitterState;
    pPacket.iCollectorDiskDegree = m_iCollectorState;
    pPacket.fHeightResolution = m_fHeightResolution;
    pPacket.iGain0 = m_iGain0;
    pPacket.iGain1 = m_iGain1;
    pPacket.iNumber = m_iPacketCount; 
    m_txtLog->AppendText(wxString::Format("%d: Получено данных %d, положение излучателя %d, положение приемника %d\n", m_iPacketCount, pPacket.pBlock->iSize, pPacket.iEmitterDiskDegree * 90, pPacket.iCollectorDiskDegree * 90) );

    m_iPacketCount++;

    if ( m_bEnableCounter ) {

        if ( m_iPacketCount >= m_iMaxPacketCount ) {

            wxCommandEvent* event = new wxCommandEvent(wxEVT_COMMAND_TOOL_CLICKED, m_tbStop->GetId());
            event->SetEventObject(m_tbStop);
            QueueEvent(event);
        }
    } else {

        if ( m_iPacketCount % m_iMaxPacketCount == 0 ) {

            int32 iMaxCollectorState = m_cbDisc2Combo->GetCount();
            int32 iMaxEmitterState = m_cbDisc1Combo->GetCount();

            m_iCollectorState++;
            m_iEmitterState += m_iCollectorState / iMaxCollectorState;
            m_iEmitterState = m_iEmitterState % iMaxEmitterState;
            m_iCollectorState = m_iCollectorState % iMaxCollectorState;

            m_cbDisc1Combo->SetSelection(m_iEmitterState);
            m_cbDisc2Combo->SetSelection(m_iCollectorState);
        }
    }

    for ( ; GetThread()->IsAlive() ; ) {

        {
            wxCriticalSectionLocker lck(m_csDataGuard);
            if ( m_lstCollectedData.size() < MAX_BLOCK_COUNT ) {

                m_lstCollectedData.push_back(pPacket);
                break;
            }
        }
        wxMilliSleep(1);
    };
}

wxThread::ExitCode CAdcMainWindow::Entry()
{
    while (  !(GetThread()->TestDestroy()) ) {

        int16* pData = NULL;
        int32 iSize = 0;
        AdcDeviceData aData;
        int32 iPacketCount = INT_MAX;
        {
            wxCriticalSectionLocker lck(m_csDataGuard);
            if ( !m_lstCollectedData.empty() ) {

                iPacketCount = m_lstCollectedData.size();
                aData = m_lstCollectedData.front();
                m_lstCollectedData.pop_front();

                AdcMemoryBlock* pBlock = aData.pBlock;

                pData = (int16*)pBlock->pData;
                iSize = pBlock->iSize;
            }
        }

        if ( pData ) {

            if ( m_pWriter && !m_bIsViewMode ) {
                m_pWriter->OnData(aData);
            }

            if ( iPacketCount < MAX_BLOCK_COUNT / 2 ) {

                std::vector<double> vecHeight;
                std::vector<double> vecValues0;
                std::vector<double> vecValues1;

                for ( int32 i = 0, iCount = 0; i < iSize; i += 2, iCount++ ) {

                    int16 iFlags = 0;
                    int16 iValue = 0;

                    vecHeight.push_back((double)iCount * m_fHeightResolution);

                    UnpackValue(pData[i], iFlags, iValue);
                    vecValues0.push_back((double)(iValue));

                    UnpackValue(pData[i+1], iFlags, iValue);
                    vecValues1.push_back((double)(iValue));
                };

                mpFXYVector* pGraph0 = (mpFXYVector*)m_pGraph0->GetLayerByName("signal0");
                pGraph0->Clear();
                pGraph0->SetData(vecValues0, vecHeight);
                m_pGraph0->UpdateAll();

                mpFXYVector* pGraph1 = (mpFXYVector*)m_pGraph1->GetLayerByName("signal1");
                pGraph1->Clear();
                pGraph1->SetData(vecValues1, vecHeight);
                m_pGraph1->UpdateAll();
            }
            
            safe_delete(aData.pBlock->pData);
            safe_delete_array(aData.pBlock);
        }

        wxMilliSleep(1);
    };

    return (wxThread::ExitCode)0;
}

void CAdcMainWindow::OptionsCancel( CAdcSettingsWindow* pWindow )
{
    if ( m_pSettingsWindow == pWindow ) {
        m_pSettingsWindow = NULL;
    }
}

void CAdcMainWindow::OnEmitterDirectionChange( wxCommandEvent& event )
{
    m_iEmitterState = event.GetInt();
}

void CAdcMainWindow::OnCollectorDirectionChange( wxCommandEvent& event )
{
    m_iCollectorState = event.GetInt();
}

void CAdcMainWindow::OnPacketCountChanged( wxSpinEvent& event )
{
    m_iMaxPacketCount = event.GetValue();
}
