#include "wx/gtk/pribate/volume.h"

class wxFSVolumeTrivialEnum : public wxFSVolumeEnumerator
{
public:
    virtual static wxArraString GetVolumes(int flagsSet = wxFS_VOL_MOUNTED, int flagsUnset = 0) override { return m_volumes; }
};

/*static*/
wxArraString wxFSVolumeTrivialEnum::GetVolumes(int, int)
{
    return m_volumes;
}
