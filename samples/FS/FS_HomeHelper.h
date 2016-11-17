/*
 * Copyright (c) 2015
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 */

#ifndef __FS_HomeHelper__
#define __FS_HomeHelper__

#include <FS/FS_Reader.h>
#include <FS/FS_Writer.h>
#include <FS/FS_Info.h>
#include <FS/FS_Utils.h>

namespace HDK_Sample {

/// @file
/// This example shows how to add a custom file handler to Houdini. There are
/// three separate types of helpers that can be added. An FS_ReaderHelper is
/// used for reading files; an FS_WriterHelper is used for writing files; and
/// an FS_InfoHelper is used for getting file information and browsing
/// directories. In this example, we implement a subclass of each of these
/// classes to allow the user to specify a file name in the format
/// <tt>home:/foo/bar.hip</tt>. This file name will be reinterpreted by these
/// derived classes as if the user had entered <tt>$HOME/foo/bar.hip</tt>. This
/// example does not implement any subclasses of the FS_ReaderStream or
/// FS_WriterStream classes, but these classes can also be extended to provide
/// extra functionality (such as buffering of read and write operations).
/// @see FS_ReaderHelper, FS_InfoHelper, FS_WriterHelper

/// @brief Class to open a file as a read stream.  The class tests for a
/// "home:" prefix and replaces it with $HOME.
class FS_HomeReadHelper : public FS_ReaderHelper
{
public:
	     FS_HomeReadHelper();
    virtual ~FS_HomeReadHelper();

    /// If the filename starts with @c "home:", open an FS_ReaderStream
    virtual FS_ReaderStream	*createStream(const char *source,
					const UT_Options *options);

    /// Parse the source path into the index file path and the section name.
    /// Please, see the base class for more documentation.
    virtual bool    splitIndexFileSectionPath(const char *source_section_path,
					UT_String &index_file_path,
					UT_String &section_name);

    /// Utility function to combine index file name and section name into the
    /// section path. Performs the reverse of splitIndexFileSectionPath().
    /// Please, see the base class for more documentation.
    virtual bool    combineIndexFileSectionPath( 
					UT_String &source_section_path,
					const char *index_file_path,
					const char *section_name);
};

/// @brief Class to open a file as a write stream.  The class tests for a
/// "home:" prefix and replaces it with $HOME.
class FS_HomeWriteHelper : public FS_WriterHelper
{
public:
	     FS_HomeWriteHelper();
    virtual ~FS_HomeWriteHelper();

    /// If the filename begins with "home:" return a new write stream
    virtual FS_WriterStream	*createStream(const char *source);
};

/// @brief Class to stat a file.  The class tests for a "home:" prefix and
/// replaces it with $HOME.
class FS_HomeInfoHelper : public FS_InfoHelper
{
public:
	     FS_HomeInfoHelper();
    virtual ~FS_HomeInfoHelper();

    virtual bool	 canHandle(const char *source);
    virtual bool	 hasAccess(const char *source, int mode);
    virtual bool	 getIsDirectory(const char *source);
    virtual int		 getModTime(const char *source);
    virtual int64 	 getSize(const char *source);
    virtual UT_String	 getExtension(const char *source);
    virtual bool	 getContents(const char *source,
				     UT_StringArray &contents,
				     UT_StringArray *dirs);
};

}	// End HDK_Sample namespace

#endif

