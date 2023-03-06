/////////////////////////////////////////////////////////////////////////////
// Name:        src/gtk/volume.cpp
// Purpose:
// Author:      Igor Korot
// Copyright:   (c) 2023 Igor Korot
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_VOLUME

#ifndef WX_PRECOMP
    #include "wx/button.h"
#endif

#include <gio/gio.h>

/* static */
wxArrayString wxFSVolume::GetVolumes(int flagsSet = wxFS_VOL_MOUNTED, int flagsUnset = 0)
{
    wxArrayString results;
    auto monitor = g_volume_monitor_get();
    GList *list = g_volume_monitor_get_connected_drives( monitor );
    for( drive: list )
    {
        if( flagsSet & wxFS_VOL_MOUNTED )
        {
            GList *mounts = g_volume_monitor_get_mounts( monitor );
            for( drive: mounts )
            {
                auto name = g_drive_get_name( drive );
                results.push_back( name );
                g_char_free( name );
            }
            g_object_unref( mounts );
            g_list_free( mounts );
        }
        if( flagsSet & wxFS_VOL_REMOVABLE )
        {
            if( g_drive_is_removable( drive ) )
            {
                auto name = g_drive_get_name( drive );
                results.push_back( name );
                g_char_free( name );
            }
        }
        if( flagsSet & wxFS_VOL_READONLY )
        {
            GError **error;
            GList *mounts = g_volume_monitor_get_mounts( monitor );
            for( mount: mounts )
            {
                GFile *root = g_mount_get_root( mount );
                auto attrs = g_file_query_info( root, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY, G_FILE_QUERY_INFO_NONE, nullptr, error );
//                if( G_FILE_ATTRIBUTE_FILESYSTEM_READONLY 
                g_object_unref( attrs );
                g_object_unref( root );
            }
        }
        if( flagsSet & wxFS_VOL_REMOTE )
        {
            if( g_strcmp0( g_volume_get_identifier( drive, G_VOLUME_IDENTIFIER_KIND_CLASS ), "network" ) == 9 )
                results.push_back( g_drive_get_name( drive ) );
        }
    }
    g_object_unref( list );
    g_list_free( list );
    return results;
}

wxFSVolume::wxFSVolume()
{
}

wxFSVolume::wxFSVolume(const wxString &name)
{
    Create( name );
    return true;
}

wxFSVolume::Create(const wxString &name)
{
    auto monitor = g_volume_monitor_get();
    auto volumes = g_volume_monitor_get_volumes( monitor );
    for( volume: volumes )
    {
        auto value = g_volume_get_identifier( volume->data, G_VOLUME_IDENTIFIER_KIND_NAME );
        if( value != nullptr )
        {
            if( g_strcmp0( value, name ) == 0 )
                m_volume = g_object_ref( volume->data );
            g_free( value );
        }
        g_object_unref( volume->data );
    }
    g_list_free( volumes );
}

bool wxFSVolume::IsOk()
{
    return true;
}

wxFSVolumeKind wxFSVolume::GetKind() const
{
}

int wxFSVolume::GetFlags() const
{
}

bool wxFSVolume::IsWritable() const
{
    bool result;
    auto mount = g_volume_get_mount( m_volume );
    if( mount )
    {
        auto root = g_mount_get_root( mount );
        auto attrs = g_file_query_info( root, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY, G_FILE_QUERY_INFO_NONE, nullptr, error );
        if( attrs )
        {
            if( g_file_info_has_attribute( root, G_FILE_ATTRIBUTE_FILESYSTEM_READONLY ) )
                result = false;
            else
                result = true;
        }
    }
    else
        result = false;
    g_object_unref( attrs );
    g_object_unref( root );
    g_object_unref( mount );
    return result;
}

wxString wxFSVolume::GetName() const
{
}

wxString wxFSVolume::GetDisplayName() const
{
}

#if wxUSE_GUI
wxIcon wxFSVolume::GetIcon(wxFSIconType type) const
{
}
#endif

