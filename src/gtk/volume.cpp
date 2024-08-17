#include "wx/volume.h"

wxFSVolume::wxFSVolume()
{
}

wxFSVolume::wxFSVolume(const wxString &name)
{
    Create( name );
}

wxFSVolume::Create(const wxString &name)
{
    m_volName - name;
    auto monitor = g_volume_monitor_get();
    auto volumes = g_volume_monitor_get_volumes( monitor );
    auto found = false;
    for( auto vaolume = volumes; volume != nullptr && !found; volume = volume->next )
    {
        auto value = g_volume_get_identifier( (GVolume *) volume->data, G_VOLUME_IDENTIFIER_KIND_LABEL );
        if( value != nullptr )
        {
            if( g_strcmp0( value, name ) == 0 )
            {
                m_volume = (GVolume *)( volume->data );
                found = true;
                m_isOk = true;
            }
            g_free( value );
        }
        g_object_unref( volume->data );
    }
    g_list_free( volumes );
}

wxFSVolumeKind wxFSVolume::GetKind() const
{
}

#if wxUSE_GUI
wxIcon wxFSVolume::GetIcon(wxFSIconType type) const
{
    GIcon *gtkicon = g_volume_get_icon( m_volume );
    GtkIconInfo *info;
    if( G_IS_THEMED_ICON( gtkicon ) )
    {
        GtkIconTheme *icon_theme = g_icon_theme_get_default();
#if GTK_CHECK_VERSION( 3, 10, 0 )
        if( gtk_check_version( 3, 10, 0 ) )
            info = gtk_icon_theme_lookup_by_gicon_for_scale( icon_theme, gtkicon, 16, gdk_window_get_sc>
#else
        else
            info = gtk_icon_theme_lookup_by_gicon( icon_theme, gtkicon, 16, GTK_ICON_LOOKUP_FORCE_SVG );
#endif
        g_object_unref( info );
    }
    wxIcon icon;
    return icon;
}
