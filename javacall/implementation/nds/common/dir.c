/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

#include <nds.h>
#include <stdio.h>
#include <fat.h>
#include <sys/dir.h>
#include <unistd.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif
    
#include "javacall_dir.h"

/**
 * returns a handle to a file list. This handle can be used in
 * subsequent calls to javacall_dir_get_next() to iterate through
 * the file list and get the names of files that match the given string.
 * 
 * @param path the name of a directory, but it can be a
 *             partial file name
 * @param pathLen length of directory name or
 *        JAVACALL_UNKNOWN_LENGTH, which may be used for null terminated string 
 * @return pointer to an opaque filelist structure, that can be used in
 *         javacall_dir_get_next() and javacall_dir_close
 *         NULL returned on error, for example if root directory of the
 *         input 'string' cannot be found.
 */
javacall_handle javacall_dir_open(const javacall_utf16* path, int pathLen) {

    char* szpath = (char *)javacall_UNICODEsToUtf8(path, pathLen);

#ifdef NDS_CONSOLE_DEBUG
    javacall_printf("javacall_dir_open: %s\n", szpath);
#endif

    static DIR_ITER* handle = NULL;

    handle = diropen(szpath);

    if (handle == NULL)
        return (NULL);

    return ((javacall_handle)handle);
}
    
/**
 * closes the specified file list. This handle will no longer be
 * associated with this file list.
 * @param handle pointer to opaque filelist struct returned by
 *               javacall_dir_open 
 */
void javacall_dir_close(javacall_handle handle) {

#ifdef NDS_CONSOLE_DEBUG
    javacall_printf("javacall_dir_close\n");
#endif

    dirclose((DIR_ITER*)handle);
}
    
/**
 * return the next filename in directory path (UNICODE format)
 * The order is defined by the underlying file system.
 * 
 * On success, the resulting file will be copied to user supplied buffer.
 * The filename returned will omit the file's path
 * 
 * @param handle pointer to filelist struct returned by javacall_dir_open
 * @param outFilenameLength will be filled with number of chars written 
 * 
 * 
 * @return pointer to UNICODE string for next file on success, 0 otherwise
 * returned param is a pointer to platform specific memory block
 * platform MUST BE responsible for allocating and freeing it.
 */
javacall_utf16* javacall_dir_get_next(javacall_handle handle, int* /*OUT*/ outFilenameLength) {

    static javacall_utf16 name[JAVACALL_MAX_FILE_NAME_LENGTH + 1];
    static char filename[JAVACALL_MAX_FILE_NAME_LENGTH + 1];

    struct stat st;

    if (dirnext((DIR_ITER*)handle, filename, &st) != 0)
        return (javacall_utf16* )NULL;

//javacall_printf("_dir_get_next: %s,(%s),", filename
        //, ((st.st_mode & S_IFDIR)? "D": "f"));

    int len = (int)strlen(filename);

//javacall_printf(": '%s',(%d) ", filename, len);

/*
#ifdef NDS_CONSOLE_DEBUG
    javacall_printf("javacall_dir_get_next: %s, len=%d, dir=%c\n"
            , filename, len, (st.st_mode & S_IFDIR)? 't': 'f');
#endif
*/

    if (len >= JAVACALL_MAX_FILE_NAME_LENGTH - 1)
        return NULL;

    else {
        if (st.st_mode & S_IFDIR) {
            name[len] = (javacall_utf16)'/';
            *outFilenameLength = len + 1;
            name[len + 1] = 0;
        } else {
            *outFilenameLength = len;
            name[len] = 0;
        }
        while (len--)
            name[len] = (javacall_utf16)filename[len] & 0xff;

//iprintf("\tname = %s\n", name);
        return (name);
    }

    return (javacall_utf16* ) NULL;
}
    
/**
 * Checks the size of free space in storage. 
 * @return size of free space
 */
javacall_int64 javacall_dir_get_free_space_for_java(void){
    return 50*1024*1024;
}
    
    
/**
 * Returns the root path of java's application directory.
 * 
 * @param rootPath returned value: pointer to unicode buffer, allocated 
 *        by the VM, to be filled with the root path.
 * @param rootPathLen IN  : lenght of max rootPath buffer
 *                    OUT : lenght of set rootPath
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_dir_get_root_path(javacall_utf16* /* OUT */ rootPath,
                                           int* /* IN | OUT */ rootPathLen) {
    static char* root = "fat1:/java";
    int i;
    int len = strlen(root);

    if (*rootPathLen < len)
        return JAVACALL_FAIL;

    for (i = 0; i < len; i++)
        rootPath[i] = (javacall_utf16)root[i];
    *rootPathLen = i;

#ifdef NDS_CONSOLE_DEBUG
    javacall_printf("javacall_dir_get_root_path: %s, len=%d\n", rootPath, *rootPathLen);
#endif

    return (JAVACALL_OK);
}  

/**
 * Returns the root path of java's configuration directory.
 * 
 * @param configPath returned value: pointer to unicode buffer, allocated 
 *        by the VM, to be filled with the root path.
 * @param configPathLen IN  : lenght of max rootPath buffer
 *                    OUT : lenght of set rootPath
 * @return <tt>JAVACALL_OK</tt> if operation completed successfully
 *         <tt>JAVACALL_FAIL</tt> if an error occured
 */
javacall_result javacall_dir_get_config_path(javacall_utf16* /* OUT */ configPath,
                                           int* /*IN | OUT*/ configPathLen) {
    static char* conf_dir = "fat1:/java/appdb";
    int i;
    int len = strlen(conf_dir);

    if (*configPathLen < len)
        return JAVACALL_FAIL;

    for (i = 0; i < len; i++)
        configPath[i] = (javacall_utf16)conf_dir[i];
    *configPathLen = i;

#ifdef NDS_CONSOLE_DEBUG
    javacall_printf("javacall_dir_get_config_path: %s, len=%d\n", configPath, *configPathLen);
#endif

    return (JAVACALL_OK);
}

/**
 *  Returns file separator character used by the underlying file system
 * (usually this function will return '\\';)
 * @return 16-bit Unicode encoded file separator
 */
javacall_utf16 javacall_get_file_separator(void) {
    return '/';
}
    
    
/**
 * Check if the given path is located on secure storage
 * The function should return JAVACALL_TRUE only in the given path
 * is located on non-removable storage, and cannot be accessed by the 
 * user or overwritten by unsecure applications.
 * @return <tt>JAVACALL_TRUE</tt> if the given path is guaranteed to be on 
 *         secure storage
 *         <tt>JAVACALL_FALSE</tt> otherwise
 */
javacall_bool javacall_dir_is_secure_storage(javacall_utf16* classPath, int pathLen) {
    return JAVACALL_FALSE;
}

#ifdef __cplusplus
}
#endif

