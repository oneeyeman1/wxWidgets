class wxFSVolume : public wxFSVolumeBase, public wxFSVolumeEnumerator
{
public:
    wxFSVolume();
    wxFSVolume(const wxString &name);
    bool Create(const wxString &name);
    bool IsOK() const { return m_isOk; }
    wxFSVolumeKind GetKind() const;
    wxIcon GetIcon(wxFSIconType type) const
private:
    bool m_isOk = false;
};
