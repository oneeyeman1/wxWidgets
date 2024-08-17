class wxFSVolumeEnumerator
{
public:
    virtual static wxArrayString GetVolumes();
protected:
    wxArrayString m_volumes;
};

