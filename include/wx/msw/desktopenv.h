/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/desktopenv.h
// Purpose:     wxDesktopEnv class
// Author:      Igor Korot
// Modified by:
// Created:     17/11/2015
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_DESKTOPENV_H_
#define _WX_MSW_DESKTOPENV_H_

// ----------------------------------------------------------------------------
// Desktop Environment
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxDesktopEnv : public wxDesktopEnvBase
{
public:
    wxDesktopEnv();
    ~wxDesktopEnv() { }
    virtual bool MoveFileToRecycleBin(const wxString &fileName);
};

#endif
