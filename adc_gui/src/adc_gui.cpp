#include "adc_gui.h"
#include "adc_main_frame.h"

IMPLEMENT_APP(CAdcGuiApp)

bool CAdcGuiApp::OnInit()
{
    if ( !wxApp::OnInit() ) {
        return false;
    }

    wxFrame* pFrame = new CAdcMainWindow;
    return true;
}

int CAdcGuiApp::OnExit()
{
    return 0;
}
