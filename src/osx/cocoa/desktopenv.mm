/////////////////////////////////////////////////////////////////////////////
// Name:        src/osx/cocoa/desktopenv.mm
// Purpose:     wxDesktopEnv
// Author:      Igor Korot
// Modified by:
// Created:     17/11/15
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////
#import <Foundation/NSArray.h>
#import <Foundation/NSURL.h>
#import <AppKit/NSWorkspace.h>

#include "wx/filefn.h"
#include "wx/osx/core/cfstring.h"
#include "wx/desktopenv.h"

wxDesktopEnv::wxDesktopEnv()
{
}

wxDesktopEnv::~wxDesktopEnv()
{
}

bool wxDesktopEnv::MoveFileToRecycleBin(const wxString &fileName)
{
    bool result = false;
    if( wxDesktopEnvBase::MoveFileToRecycleBin( fileName ) )
    {
        if( wxDirExists( fileName ) )
            result = MoveDirectoryToTrash( fileName );
        else if( wxFileExists( fileName ) )
            result = MoveFileToTrash( fileName );
    }
    return result;
}

bool wxDesktopEnv::MoveDirectoryToTrash(const wxString &dirName)
{
    bool ret = true;
    wxString temp = "file:///" + dirName;
    NSURL *url = [NSURL URLWithString:wxCFStringRef( temp ).AsNSString()];
    NSArray *files = [NSArray arrayWithObject:url];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
    BOOL result = [[NSFileManager defaultManager] trashItemAtURL:url resultingItemURL:nil error:nil];
    if( result == NO )
        ret = false;
#else
    OSStatus status = FSPathMoveObjectToTrashSync( dirName.c_str(), NULL, kFSFileOperationDefaultOptions );
#endif
    return ret;
}

bool wxDesktopEnv::MoveFileToTrash(const wxString &fileName)
{
    bool ret = true;
    wxString temp = "file:///" + fileName;
    NSURL *url = [NSURL URLWithString:wxCFStringRef( temp ).AsNSString()];
    NSArray *files = [NSArray arrayWithObject:url];
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1080
    BOOL result = [[NSFileManager defaultManager] trashItemAtURL:url resultingItemURL:nil error:nil];
    if( result == NO )
        ret = false;
#else
    OSStatus status = FSPathMoveObjectToTrashSync( fileName.c_str(), NULL, kFSFileOperationDefaultOptions );
#endif
    return ret;
}
