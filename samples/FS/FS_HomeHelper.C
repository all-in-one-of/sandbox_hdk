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

#include <UT/UT_Assert.h>
#include <UT/UT_DSOVersion.h>
#include <UT/UT_EnvControl.h>
#include <UT/UT_NTStreamUtil.h>
#include <UT/UT_DirUtil.h>
#include <UT/UT_OpUtils.h>
#include <UT/UT_SysClone.h>
#include <UT/UT_WorkArgs.h>
#include <UT/UT_WorkBuffer.h>
#include "FS_HomeHelper.h"

#define HOME_SIGNATURE		"home:"
#define HOME_SIGNATURE_LEN	5

// NB: The compiled dso needs to be installed somewhere in $HOUDINI_DSO_PATH
//     under the 'fs' subdirectory.  For example: $HOME/houdiniX.Y/dso/fs.

void
installFSHelpers()
{
    new HDK_Sample::FS_HomeReadHelper();
    new HDK_Sample::FS_HomeWriteHelper();
    new HDK_Sample::FS_HomeInfoHelper();
}

using namespace HDK_Sample;

// ============================================================================
// The FS_HOMEREADER_HANDLE_OPTIONS define illustrates how to implement a file
// system protocol that uses '?' character in the file paths as a special marker
// for its own use, which conflicts with the default interpretation of '?' by
// FS library of a section name separator for index files.
// For more info, please see FS_Reader.h
// 
// When FS_HOMEREADER_HANDLE_OPTIONS is not defined, the 'home:' protocol
// implements a much simpler mechanism that limits itself to replacing
// "home:" prefix with the home path. And since that simpified version does not
// assign any special meaning to '?' (nor '&'), it does not need to worry about
// explicitly handling index files (which is then handled automatically by the
// base classes).
//
// When FS_HOMEREADER_HANDLE_OPTIONS is defined, the 'home:' protocol
// implemented in this example uses '?' and '&' markers for delineating file
// access options.  With '?' redefined as options separator, this protocol 
// needs to establish a new convention for representing a path to an index file
// section (unless the protocol intentionally does not need or does not want to
// support direct access to index file sections). However, this example shows
// how this can be done, by designating the option "section=" for specifying the
// section name inside the index file given by the path portion before the '?' 
// delimeter.
//
//
// With FS_HOMEREADER_HANDLE_OPTIONS defined, The 'home:' protocol recognizes
// the following options: 
//   "version" - to append the given version to the file name
//   "ext"     - to append the given extension to the file name
//   "section" - to specify the section name in the given index file 
// E.g.,
//   "home:/temp?version=1_2&ext=txt" --> "~/temp-1_2.ext"
//   "home:/temp?section=foo&ext=idx" --> "~/temp.idx?foo"
//   "home:/temp.idx?section=foo"     --> "~/temp.idx?foo"
//   "home:/no_options.txt"	      --> "~/no_options.txt"
//
#define FS_HOMEREADER_HANDLE_OPTIONS

#ifdef FS_HOMEREADER_HANDLE_OPTIONS
static inline bool
fsFindLastOption(const char *option_name, const UT_WorkArgs &options,
		 UT_String *option_value = NULL, int *option_index = NULL)
{
    UT_String	option_tag;
    
    option_tag  = option_name;
    option_tag += "=";

    if( option_value )
	option_value->clear();
    if( option_index )
	*option_index = -1;
    for( int i = options.getArgc()-1; i >= 0; i--  )
    {
	UT_String	token( options(i) );

	if( token.startsWith( option_tag ))
	{
	    if( option_value )
		token.substr( *option_value, option_tag.length() );
	    if( option_index )
		*option_index = i;
	    return true;
	}
    }

    return false;
}

static inline void
fsGetFileAndOptions( const char *source, 
		     UT_String &file_str, UT_String &options_str )
{
    UT_String	    source_str( source );
    UT_WorkArgs	    list;
    int		    count;

    source_str.tokenize( list, '?' );
    count = list.getArgc();
    UT_ASSERT( 0 <= count && count <= 2 );
    file_str.harden( count > 0 ? list(0) : "" );
    options_str.harden( count > 1 ? list(1) : "" );
}

static inline void
fsAppendOptionsExceptForSkipped( UT_WorkBuffer &buff, 
				 const UT_WorkArgs &options, int skip_index )
{
    bool do_questionmark = true;
    bool do_ampersand    = false;
    for( int i = 0; i < options.getArgc(); i++ )
    {
	if( i == skip_index )
	    continue;

	if( do_questionmark )
	{
	    buff.append('?');
	    do_questionmark = false;
	}

	if( do_ampersand )
	    buff.append('&');

	buff.append( options(i) );
	do_ampersand = true;
    }
}

static inline bool
fsSplitPathIntoFileAndSection( const char *source, 
			       UT_String &file_name, UT_String &section_name)
{
    UT_String	    file_str, options_str;
    UT_WorkArgs	    options;
    int		    section_option_index;

    // Parse the source path and look for 'section' option, but search the list
    // backwards (ie, get the last section option), which handles nested index
    // files in similar way as the standard index file notation (ie,
    // "home:x?section=a&section=b" is equivalent to "$HOME/x?a?b").
    fsGetFileAndOptions( source, file_str, options_str );
    options_str.tokenize( options, '&' );
    fsFindLastOption( "section", options, &section_name, &section_option_index);

    // Reconstruct the source path, but without the section name, which we 
    // just extracted from the options. This will allow accessing the index file
    // stream and then forming the substream for that section.
    UT_WorkBuffer buff;
    buff.append( file_str );
    fsAppendOptionsExceptForSkipped( buff, options, section_option_index );
    buff.copyIntoString( file_name );

    return section_name.isstring();
}
    
static inline void
fsCombineFileAndSectionIntoPath( UT_String &source, 
			     const char *file_name, const char *section_name)
{
    UT_WorkBuffer   buffer;
    UT_String	    file_name_str( file_name );
    char	    separator;

    // Figure out the separator: '?' if no options yet, or '&' if there are.
    UT_ASSERT( UTisstring( file_name ) && UTisstring( section_name ));
    separator =  file_name_str.findChar('?') ? '&' : '?';

    // Copy the base file name and append the section option.
    buffer.strcpy( file_name );
    buffer.append( separator );
    buffer.append( "section=" );
    buffer.append( section_name );

    buffer.copyIntoString( source );
}

static inline void
fsStripSectionOptions( UT_String &path, UT_StringArray &sections )
{
    UT_String	section_name;

    // Note, we can arbitrarily choose whether to put the innermost sections
    // at the beginning or the end of 'sections' array, as long as we handle
    // this order correctly in fsAppendSectionNames().
    // However, just to be consistent with the convention used by
    // UT_OpUtils::splitOpIndexFileSectionPath() and
    // FS_Reader::splitIndexFileSectionPath() we put innermost sections 
    // at the end (in case we ever need to call these static helper functions).
    while( fsSplitPathIntoFileAndSection( path, path, section_name ))
	sections.insert( section_name, /*index=*/ 0 );
}

static inline void
fsAppendSectionNames( UT_String &path, UT_StringArray &sections )
{
    // Construct section names using the standard convention for index files
    // (ie, using '?' as name separator) so that the default writer can 
    // understand the path.
    // Note, fsStripSectionOptions() puts innermost sections at the end 
    // of the array, so we iterate forward to indeed get the last entry
    // in the array to be the innermost section.
    for( int i = 0; i < sections.entries(); i++ )
	UT_OpUtils::combineStandardIndexFileSectionPath( path, path, 
							 sections(i) );
}

static inline void
fsAppendFileSuffix( UT_String &str, 
		    const UT_WorkArgs &options, const char *option_name,
		    const char *separator )
{
    UT_String	option_value;

    if( fsFindLastOption( option_name, options, &option_value ))
    {
	str += separator;
	str += option_value;
    }
}

static inline void
fsProcessNonSectionOptions( UT_String &source )
{
    UT_String	    options_str;
    UT_WorkArgs	    options;

    // We follow the description of the options in the comment next to the 
    // FS_HOMEREADER_HANDLE_OPTIONS define.
    // Split the source path into file and options, and then modify the
    // final file path according to the options. 
    // Note, there should not be any 'section' option at this point anymore,
    // since it should have been alrwady stripped in
    // splitIndexFileSectionPath().
    fsGetFileAndOptions( source, source, options_str );
    options_str.tokenize( options, '&' );
    UT_ASSERT( ! fsFindLastOption( "section", options ));
    fsAppendFileSuffix( source, options, "version", "-" );
    fsAppendFileSuffix( source, options, "ext", "." );
}

#endif // FS_HOMEREADER_HANDLE_OPTIONS

// ============================================================================
static inline void
fsPrefixPathWithHome(UT_String &path)
{
    UT_WorkBuffer   buff;

    // Substitute 'home:' prefix with home path.
    UT_ASSERT( path.length() >= HOME_SIGNATURE_LEN );
    buff.append( UT_EnvControl::getString(ENV_HOME));
    if( path(HOME_SIGNATURE_LEN) != '/' )
	buff.append('/');
    buff.append( &path.buffer()[HOME_SIGNATURE_LEN] );

    buff.copyIntoString(path);
}

static bool
fsConvertToStandardPathForWrite(UT_String &destpath, const char *srcpath)
{
    // Handle only the 'home:' protocol paths.
    if( strncmp(srcpath, HOME_SIGNATURE, HOME_SIGNATURE_LEN) != 0 )
	return false;

    destpath = srcpath;
#ifdef FS_HOMEREADER_HANDLE_OPTIONS
    // For writing, we convert the "section=" options for inex files in 'home:'
    // protocol into the standard notation because the FS_WriterStream class,
    // used by FS_HomeWriteHelper, can write to index files only when
    // specified in that standard notation.
    // Alternatively, we could derive own class from FS_WriterStream and handle
    // writing to index files there. However, since 'home:' protocol really
    // refers to the disk files, we take advantage of the FS_WriterStream
    // base class capabilities.
    UT_StringArray  sections;
    fsStripSectionOptions( destpath, sections );
    fsProcessNonSectionOptions( destpath );
    fsAppendSectionNames( destpath, sections );
#endif 

    fsPrefixPathWithHome(destpath);
    return true;
}

static bool
fsConvertToStandardPathForRead(UT_String &destpath, const char *srcpath)
{
    // Handle only the 'home:' protocol paths.
    if( strncmp(srcpath, HOME_SIGNATURE, HOME_SIGNATURE_LEN) != 0 )
	return false;

    destpath = srcpath;
#ifdef FS_HOMEREADER_HANDLE_OPTIONS
    // For read, no need to handle index file sections, since there should not 
    // be any in the 'srcpath' because they were stripped in 
    // splitIndexFileSectionPath(). So for reading only, there is no need to 
    // implement some of the static functions that are needed only for writing.
    // In particular, there is no need for fsAppendSectionNames() which builds
    // the file path and appends section names using standard '?' notation.
    fsProcessNonSectionOptions( destpath );
#endif 

    fsPrefixPathWithHome(destpath);
    return true;
}

static bool
fsConvertToStandardPathForInfo(UT_String &destpath, const char *srcpath)
{
#ifdef FS_HOMEREADER_HANDLE_OPTIONS
    // For info, the FS_Info class already handles index files implicitly 
    // (via FS_ReadHelper),  and calls this info helper class only for non-index
    // files. So here, the srcpath should not be pointing to any index file
    // section, just like in the case of fsConvertToStandardPathForRead().
    // So call the conversion function for reading.
#endif 
    return fsConvertToStandardPathForRead(destpath, srcpath);
}

// ============================================================================
FS_HomeReadHelper::FS_HomeReadHelper()
{
    UTaddAbsolutePathPrefix(HOME_SIGNATURE);
}

FS_HomeReadHelper::~FS_HomeReadHelper()
{
}

FS_ReaderStream *
FS_HomeReadHelper::createStream(const char *source, const UT_Options *)
{
    FS_ReaderStream		*is = 0;
    UT_String			 homepath;

    if( fsConvertToStandardPathForRead(homepath, source) )
	is = new FS_ReaderStream(homepath);

    return is;
}

bool
FS_HomeReadHelper::splitIndexFileSectionPath(const char *source_section_path,
					UT_String &index_file_path,
					UT_String &section_name)
{
#ifdef FS_HOMEREADER_HANDLE_OPTIONS
    // We should be splitting only our own paths.
    if( strncmp(source_section_path, HOME_SIGNATURE, HOME_SIGNATURE_LEN) != 0 )
	return false;

    // If the index file section sparator (ie, the question mark '?') has
    // a special meaning in "home:" protocol, then this virtual overload is
    // necessary to avoid the default parsing of source path when creating a
    // stream. See the base class header comments for more details.
    // Here, '?' separates the file path from file access options such as
    // file version. Eg, in this example "home:myfile?version=1.2" file version
    // is simply concatenated to the file name to form "myfile-1.2", but 
    // in principle it could use any other mechanism (like retrieveing it
    // from a revision system or a database, etc).
    // To support the index files the protocol has a 'section' option, 
    // "home:myfile?section=mysection", which allows accessing the section
    // 'mysection' inside 'myfile' (equivalent to "$HOME/myfile?mysection"), 
    // or "home:myfile?version=1.2&section=mysection" (equivalent to
    // "$HOME/myfile-1.2?mysection").
    fsSplitPathIntoFileAndSection( source_section_path, 
				   index_file_path, section_name );

    // Return true to indicate we are handling own spltting of section paths,
    // whether or not the given path actually refers to an index file section.
    return true;
#else
    // If we are not handling '?' in any special way, then we return false. 
    // We could also remove this method althogehter from this class and 
    // rely on the base class for the default behaviour.
    return false;
#endif // FS_HOMEREADER_HANDLE_OPTIONS
}

bool
FS_HomeReadHelper::combineIndexFileSectionPath( UT_String &source_section_path,
					const char *index_file_path,
					const char *section_name)
{
#ifdef FS_HOMEREADER_HANDLE_OPTIONS
    // We should be combining only our own paths.
    if( strncmp(index_file_path, HOME_SIGNATURE, HOME_SIGNATURE_LEN) != 0 )
	return false;

    // See the comments in splitIndexFileSectionPath()
    fsCombineFileAndSectionIntoPath( source_section_path, 
				     index_file_path, section_name );

    // Return true to indicate we are handling own gluing of section paths.
    return true;
#else
    // If we are not handling '?' in any special way, then we return false. 
    // We could also remove this method althogehter from this class and 
    // rely on the base class for the default behaviour.
    return false;
#endif // FS_HOMEREADER_HANDLE_OPTIONS
}


// ============================================================================
FS_HomeWriteHelper::FS_HomeWriteHelper()
{
    UTaddAbsolutePathPrefix(HOME_SIGNATURE);
}

FS_HomeWriteHelper::~FS_HomeWriteHelper()
{
}

FS_WriterStream *
FS_HomeWriteHelper::createStream(const char *source)
{
    FS_WriterStream		*os = 0;
    UT_String			 homepath;

    if( fsConvertToStandardPathForWrite(homepath, source) )
	os = new FS_WriterStream(homepath);

    return os;
}

// ============================================================================
FS_HomeInfoHelper::FS_HomeInfoHelper()
{
    UTaddAbsolutePathPrefix(HOME_SIGNATURE);
}

FS_HomeInfoHelper::~FS_HomeInfoHelper()
{
}

bool
FS_HomeInfoHelper::canHandle(const char *source)
{
    return (strncmp(source, HOME_SIGNATURE, HOME_SIGNATURE_LEN) == 0);
}

bool
FS_HomeInfoHelper::hasAccess(const char *source, int mode)
{
    UT_String			 homepath;

    if( fsConvertToStandardPathForInfo(homepath, source) )
    {
	FS_Info			 info(homepath);

	return info.hasAccess(mode);
    }

    return false;
}

bool
FS_HomeInfoHelper::getIsDirectory(const char *source)
{
    UT_String			 homepath;

    if( fsConvertToStandardPathForInfo(homepath, source) )
    {
	FS_Info			 info(homepath);

	return info.getIsDirectory();
    }

    return false;
}

int
FS_HomeInfoHelper::getModTime(const char *source)
{
    UT_String			 homepath;

    if( fsConvertToStandardPathForInfo(homepath, source) )
    {
	FS_Info			 info(homepath);

	return info.getModTime();
    }

    return 0;
}

int64
FS_HomeInfoHelper::getSize(const char *source)
{
    UT_String			 homepath;

    if( fsConvertToStandardPathForInfo(homepath, source) )
    {
	FS_Info			 info(homepath);

	return info.getFileDataSize();
    }

    return 0;
}

UT_String
FS_HomeInfoHelper::getExtension(const char *source)
{
    UT_String			 homepath;

    if( fsConvertToStandardPathForInfo(homepath, source) )
    {
#ifdef FS_HOMEREADER_HANDLE_OPTIONS
	UT_String	    filename_str;
	UT_String	    options_str;
	UT_String	    option_value;
	UT_WorkArgs	    options;

	fsGetFileAndOptions( source, filename_str, options_str );
	options_str.tokenize( options, '&' );
	if( fsFindLastOption( "ext", options, &option_value ))
	{
	    UT_String	     extension(UT_String::ALWAYS_DEEP, ".");
	    extension += option_value;
	    return extension;
	}
#endif
	return FS_InfoHelper::getExtension(homepath);
    }

    return FS_InfoHelper::getExtension(source);
}

bool
FS_HomeInfoHelper::getContents(const char *source,
			       UT_StringArray &contents,
			       UT_StringArray *dirs)
{
    UT_String			 homepath;

    if( fsConvertToStandardPathForInfo(homepath, source) )
    {
	FS_Info			 info(homepath);

	return info.getContents(contents, dirs);
    }

    return false;
}

