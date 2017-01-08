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

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_file.h"
#include "javacall_memory.h"

#include <nds.h>
#include <fat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#define MAX_FILE_NAME_LEN JAVACALL_MAX_FILE_NAME_LENGTH

//#define DEBUG_JAVACALL_FILE 1

int fs_init = 0;

char* javacall_UNICODEsToUtf8(const javacall_utf16* fileName, int fileNameLen) {
    static char result[JAVACALL_MAX_FILE_NAME_LENGTH + 1];
    int i;
    if (fileNameLen > JAVACALL_MAX_FILE_NAME_LENGTH || fileName == NULL) {
        return NULL;
    }

    for (i=0; fileName[i]!='\0' && i<fileNameLen; i++) {
        result[i] = (char)fileName[i];
    }
    result[i] = 0;
    return &result[0];
} 

/**
 * Initializes the File System
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_init(void) {

    if (fs_init)
        return JAVACALL_OK;

#ifdef NDS_CONSOLE_DEBUG
    javacall_printf("javacall_file_init\n");
#endif

    if (!fatInitDefault()) {
        return (JAVACALL_FAIL);
    }

    fs_init = 1;

    return JAVACALL_OK;

}
/**
 * Cleans up resources used by file system
 * @return <tt>JAVACALL_OK</tt> on success, <tt>JAVACALL_FAIL</tt> or negative value on error
 */ 
javacall_result javacall_file_finalize(void) {
    return JAVACALL_OK;
}

#define DEFAULT_CREATION_MODE_PERMISSION (0666)

/**
 * The open a file
 * @param unicodeFileName path name in UNICODE of file to be opened
 * @param fileNameLen length of file name
 * @param flags open control flags
 *        Applications must specify exactly one of the first three
 *        values (file access modes) below in the value of "flags"
 *        JAVACALL_FILE_O_RDONLY, JAVACALL_FILE_O_WRONLY, JAVACALL_FILE_O_RDWR
 *
 *        Any combination (bitwise-inclusive-OR) of the following may be used:
 *        JAVACALL_FILE_O_CREAT, JAVACALL_FILE_O_TRUNC, JAVACALL_FILE_O_APPEND,
 *
 * @param handle address of pointer to file identifier
 *        on successful completion, file identifier is returned in this 
 *        argument. This identifier is platform specific and is opaque
 *        to the caller.  
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 * 
 */

javacall_result javacall_file_open(const javacall_utf16 * unicodeFileName, int fileNameLen, int flags, /*OUT*/ javacall_handle * handle) {

    char mode[5];
    FILE *fd;
    javacall_bool need_fseek=JAVACALL_FALSE;
    char *filename = javacall_UNICODEsToUtf8(unicodeFileName, fileNameLen);

    if (filename == NULL) {
#ifdef DEBUG_JAVACALL_FILE
        javacall_printf("javacall_file_open(): path name too long\n");
#endif
       return JAVACALL_FAIL;
    }

    if (flags == JAVACALL_FILE_O_RDONLY) {
        strcpy(mode, "rb");
    } else if (flags == JAVACALL_FILE_O_CREAT) { // JAVACALL_FILE_O_CREAT | JAVACALL_FILE_O_RDONLY
        // create empty file first
        fd = fopen(filename, "wb");
        if (fd == NULL) return JAVACALL_FAIL;
        fclose(fd);

        strcpy(mode, "rb");
    } else if (flags & JAVACALL_FILE_O_WRONLY) {
        // test if file already exists
        fd = fopen(filename, "r");
        if (!(flags & JAVACALL_FILE_O_CREAT) && (fd == NULL)) {
            return JAVACALL_FAIL;
        }
        // file already exists
        fclose(fd);
        if (flags & JAVACALL_FILE_O_TRUNC) {
            strcpy(mode, "wb");
        } else {
            strcpy(mode, "ab");
        }
    } else if (flags & JAVACALL_FILE_O_RDWR) {
        // test if file already exists
        fd = fopen(filename, "r");
        if (!(flags & JAVACALL_FILE_O_CREAT) && (fd == NULL)) {
            return JAVACALL_FAIL;
        }
        // file already exists
        fclose(fd);
        if (flags & JAVACALL_FILE_O_TRUNC) {
            strcpy(mode, "w+b");
        } else if (flags & JAVACALL_FILE_O_APPEND) {
            strcpy(mode, "a+b");
        } else {  // not specify how to write, fseek to 0
            need_fseek = JAVACALL_TRUE;
            strcpy(mode, "a+b");
        }
    } else {
        return JAVACALL_FAIL;
    }

    fd = fopen(filename, mode);

    if (fd == NULL) {
        *handle = NULL;
        return JAVACALL_FAIL;
    }

    if (need_fseek) {
        fseek(fd, 0, SEEK_SET);
    }
    *handle = (javacall_handle)fd;

    return JAVACALL_OK;
}

/**
 * Closes the file with the specified handlei
 * @param handle handle of file to be closed
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_close(javacall_handle handle) {

//javacall_printf("\tjavacall_file_close\n");

    int rc = fclose((FILE *)handle);
    return (rc == 0) ? JAVACALL_OK : JAVACALL_FAIL;
}


/**
 * Reads a specified number of bytes from a file, 
 * @param handle handle of file 
 * @param buf buffer to which data is read
 * @param size number of bytes to be read. Actual number of bytes
 *              read may be less, if an end-of-file is encountered
 * @return the number of bytes actually read
 */
long javacall_file_read(javacall_handle handle, unsigned char *buf, long size) {
    return (fread(buf, 1, size, (FILE *)handle));
}

/**
 * Writes bytes to file
 * @param handle handle of file 
 * @param buf buffer to be written
 * @param size number of bytes to write
 * @return the number of bytes actually written. This is normally the same 
 *         as size, but might be less (for example, if the persistent storage being 
 *         written to fills up).
 */
long javacall_file_write(javacall_handle handle, const unsigned char *buf, long size) {
	return (fwrite(buf, 1, size, (FILE *)handle));
}

/**
 * Deletes a file from the persistent storage.
 * @param unicodeFileName name of file to be deleted
 * @param fileNameLen length of file name
 * @return JAVACALL_OK on success, <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_delete(const javacall_utf16 * unicodeFileName, int fileNameLen) {

    char* pszOsFilename = javacall_UNICODEsToUtf8(unicodeFileName, fileNameLen);

    if (pszOsFilename == NULL) {
        return JAVACALL_FAIL;
    }

//javacall_printf("javacall_file_delete: %s\n", pszOsFilename);

    int status = unlink(pszOsFilename);
    
    return (status == 0) ? JAVACALL_OK : JAVACALL_FAIL;
}

/**
 * The  truncate function is used to truncate the size of an open file in 
 * the filesystem storage.
 * @param handle identifier of file to be truncated
 *         This is the identifier returned by javacall_file_open()
 *         The handle may be optionally modified by the implementation
 *         of this function
 * @param size size to truncate to
 * @return <tt>JAVACALL_OK</tt> on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value on error
 */
javacall_result javacall_file_truncate(javacall_handle handle, javacall_int64 size) {
/*
    int status = ftruncate((int)handle, (off_t)size);
    return (status == 0) ? JAVACALL_OK : JAVACALL_FAIL;
    */
    javacall_print("[JAVACALL STUB]: javacall_file_truncate()\n");
    return JAVACALL_OK;
}

/**
 * Sets the file pointer associated with a file identifier 
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @param offset number of bytes to offset file position by
 * @param flag controls from where offset is applied, from 
 *                 the beginning, current position or the end
 *                 Can be one of JAVACALL_FILE_SEEK_CUR, JAVACALL_FILE_SEEK_SET 
 *                 or JAVACALL_FILE_SEEK_END
 * @return on success the actual resulting offset from beginning of file
 *         is returned, otherwise -1 is returned
 */
javacall_int64 javacall_file_seek(javacall_handle handle, javacall_int64 offset, javacall_file_seek_flags flag) {

    FILE* f = (FILE *)handle;
    int whence = 
        (flag == JAVACALL_FILE_SEEK_SET) ? SEEK_SET :
        (flag == JAVACALL_FILE_SEEK_CUR) ? SEEK_CUR :
        (flag == JAVACALL_FILE_SEEK_END) ? SEEK_END : 0;
    
    if (0 == fseek(f, offset, whence)) {
        return (javacall_int64)ftell(f);
    } else {
        return -1;
    }
}


/**
 * Get file size 
 * @param handle identifier of file
 *               This is the identifier returned by pcsl_file_open()
 * @return size of file in bytes if successful, -1 otherwise
 */
javacall_int64 javacall_file_sizeofopenfile(javacall_handle handle) {

    //stat is not working so we have to emulate it

    FILE *f = (FILE *)handle;
    long curr = ftell(f);

    if (curr <= -1)
        return (-1);

    if (fseek(f, 0L, SEEK_END) != 0)
        return (-1);

    int sz = ftell(f);
    fseek(f, curr, SEEK_SET);

    //Relies on ftell to return -1 to do the right thing
    return ((javacall_int64)sz);
}

/**
 * Get file size
 * @param fileName name of file in unicode format
 * @param fileNameLen length of file name
 * @return size of file in bytes if successful, -1 otherwise 
 */
javacall_int64 javacall_file_sizeof(const javacall_utf16 * fileName, int fileNameLen) {

    javacall_handle handle = NULL;
    int flags = 0;
    
    javacall_result rslt1 = javacall_file_open(fileName, fileNameLen, flags, &handle);
    if (rslt1 != JAVACALL_OK) {
        return -1;
    }
    long size = javacall_file_sizeofopenfile(handle);
    javacall_file_close(handle);

    return size;
}

/**
 * Check if the file exists in file system storage.
 * @param fileName name of file in unicode format
 * @param fileNameLen length of file name
 * @return <tt>JAVACALL_OK </tt> if it exists and is a regular file, 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_exist(const javacall_utf16 * fileName, int fileNameLen) {

    char* szOsFilename=javacall_UNICODEsToUtf8(fileName, fileNameLen);
    int attrs;
    struct stat st;

    if(!szOsFilename) {
        javacall_print("Error: javacall_file_exist(), path name is too long\n");
        return JAVACALL_FAIL;
    }

    attrs = stat(szOsFilename, &st);
    if (attrs) {
#ifdef DEBUG_JAVACALL_FILE
        javacall_printf("javacall_file_exist():%s path not found\n", szOsFilename);
#endif
        return JAVACALL_FAIL;
    }

    return ((st.st_mode & S_IFDIR) == 0) ? JAVACALL_OK : JAVACALL_FAIL;
}


/** 
 * Force the data to be written into the file system storage
 * @param handle identifier of file
 *               This is the identifier returned by javacall_file_open()
 * @return JAVACALL_OK  on success, <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_flush(javacall_handle handle) {
    return (JAVACALL_OK);
/*
    int fd = fileno((FILE *)handle);
    if (fd <= -1)
        return JAVACALL_FAIL;
    return ((fsync(fd) == 0)? JAVACALL_OK: JAVACALL_FAIL);
*/
}

/**
 * Renames the filename.
 * @param unicodeOldFilename current name of file
 * @param oldNameLen current name length
 * @param unicodeNewFilename new name of file
 * @param newNameLen length of new name
 * @return <tt>JAVACALL_OK</tt>  on success, 
 *         <tt>JAVACALL_FAIL</tt> or negative value otherwise
 */
javacall_result javacall_file_rename(const javacall_utf16 * unicodeOldFilename, int oldNameLen, 
        const javacall_utf16 * unicodeNewFilename, int newNameLen) {
    //According to psp newlib, rename doesn't support "move", only support rename in same directory
    
    static char oldfn[MAX_FILE_NAME_LEN+1];
    char* pszOldFilename = javacall_UNICODEsToUtf8(unicodeOldFilename, oldNameLen);
    if (pszOldFilename == NULL) {
        return JAVACALL_FAIL;
    }
    
    strcpy(oldfn, pszOldFilename);

    char* pszNewFilename = javacall_UNICODEsToUtf8(unicodeNewFilename, newNameLen);
    if (pszNewFilename == NULL) {        
        return JAVACALL_FAIL;
    }

    int status = rename(oldfn, pszNewFilename);
    return (status == 0) ? JAVACALL_OK : JAVACALL_FAIL;

}


#ifdef __cplusplus
}
#endif
