/////////////////////////////////////////////////////////////////////////////
// Name:        wx/gtk/volume.h
// Purpose:     wxGTK wxFSVolume class declaration
// Author:      Igor Korot
// Copyright:   (c) 2023 Igor Korot
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GTK_BUTTON_H_
#define _WX_GTK_BUTTON_H_

class wxFSVolume
{
public:
    wxFSVolume();
    wxFSVolume(const wxString &name);
    bool Create(const wxString &name);
    bool IsOk();
    wxFSVolumeKind GetKind() const;
    int GetFlags() const;
    bool IsWritable() const;
    wxString GetName() const;
    wxString GetDisplayName() const;
#if wxUSE_GUI
    wxIcon GetIcon(wxFSIconType type) const;
#endif
    static wxArrayString GetVolumes(int flagsSet = wxFS_VOL_MOUNTED, int flagsUnset = 0);
private:
    GVolume *m_volume;
};
