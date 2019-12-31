/* clang-format off */
/*
    A small drop-in library providing some functions related to directories.

    version 0.1, April, 2015

    Copyright (C) 2015- Fredrik Kihlander

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.

    Fredrik Kihlander

CONTRIBUTORS:

    Fredrik Kihlander   (implementation)
    Fredrik Engkvist    (single header ANSI C conversion,
                         added 'dir_path_tidy'+'dir_path_filename'+'dir_path_extension',
                         added the following new dir-walk options/flags :
                           'DIR_WALK_SINGLE_DIRECTORY' +
                           'DIR_WALK_ONLY_DIRECTORIES' +
                           'DIR_WALK_ONLY_FILES' +
                           'DIR_WALK_IGNORE_DOT_DIRECTORIES' +
                           'DIR_WALK_IGNORE_DOT_FILES' +
                           'DIR_WALK_ROOT_RELATIVE_PATHS' +
                           option to specify the path-separator for 'dir-walk paths',
                         optional directory and file glob patterns in dir-walk,
                         handling of both (forward/back)-slash path separators in 'dir_glob_match' input-path,
                         handling of glob patters that ends with '**')
*/

/*
   This file provides both the interface and the implementation,
   which means that in *ONE* source file, put:
   #define DIRUTIL_IMPLEMENTATION
   #include "dirutil.h"

   Other source files should just include dirutil.h
*/

#ifndef FILE_DIR_H_INCLUDED
#define FILE_DIR_H_INCLUDED

#ifdef __cplusplus
   extern "C" {
#endif

#if defined(DIRUTIL_STATIC)
   #define DIRUTIL_API static
#else
   #define DIRUTIL_API extern
#endif

/* default slash on current platform */
#if defined( _WIN32 )
   #define DIR_SEP_PLATFORM '\\'
#else
   #define DIR_SEP_PLATFORM '/'
#endif

enum dir_error
{
   DIR_ERROR_OK,

   DIR_ERROR_FAILED,
   DIR_ERROR_PATH_TOO_DEEP,
   DIR_ERROR_PATH_DO_NOT_EXIST,

   DIR_ERROR_FORCEINT = 65536 /* force the enum to be signed integer */
};

/**
 * Create directory.
 * @param path dir to create
 */
DIRUTIL_API enum dir_error dir_create( const char* path );

/**
 * Create all non-existing directories in path.
 * @param path directories to create
 */
DIRUTIL_API enum dir_error dir_mktree( const char* path );

/**
 * Remove directory recursively
 * @param path dir to remove
 *
 * @note this is not an atomic operation and if it fails it might leave the directory partly removed.
 */
DIRUTIL_API enum dir_error dir_rmtree( const char* path );

/**
 * Callback called for each item with dir_walk.
 * @param path full path to current item with input path to dir_walk() as a base.
 * @param type item type, file or dir.
 * @param userdata passed to dir_walk.
 *
 * @return 0 (zero) to keep iterating, non-zero to stop iterating (NB: NYI - currently return value is ignored)
 */
enum dir_item_type
{
   DIR_ITEM_FILE,
   DIR_ITEM_DIR,
   DIR_ITEM_UNHANDLED,

   DIR_ITEM_FORCEINT = 65536 /* force the enum to be signed integer */
};
/**
 * all paths is tidy i.e. no running slashes, no trailing slashes for directories
 * and have path separators directed by input flags to 'dir_walk'/'dir_walkex')
 */
typedef int ( *dir_walk_callback )( const char* path, unsigned int path_len, enum dir_item_type type, void* userdata );

enum dir_walk_flags
{
   DIR_WALK_NO_FLAGS = 0,
   DIR_WALK_DEPTH_FIRST = 1 << 1,

   DIR_WALK_SINGLE_DIRECTORY = 1 << 2,       /* if it should _NOT_ walk directories and just list content in input-folder */

   DIR_WALK_ONLY_DIRECTORIES = 1 << 3,       /* only invoke callback for directories */
   DIR_WALK_ONLY_FILES = 1 << 4,             /* only invoke callback for files */

   DIR_WALK_IGNORE_DOT_DIRECTORIES = 1 << 5, /* ignore all directories that starts with a '.' */
   DIR_WALK_IGNORE_DOT_FILES = 1 << 6,       /* ignore all files that starts with a '.' */

   DIR_WALK_ROOT_RELATIVE_PATHS = 1 << 7,    /* the paths in user callback have input/root-directory part stripped
                                                ex:
                                                   input/root-directory = "local/folder"

                                                   item = "local/folder/subfolder" -> "subfolder" (in user callback)
                                                   item = "local/folder/subfolder/file.txt" -> "subfolder/file.txt" (in user callback)
                                             */
   /**
    * either specify slash-type or get platform default ('\' on Windows and '/' elsewhere)
    * all items in the invoked callback will have paths with this selected (or default) path separator
    */
   DIR_WALK_PATHS_SLASH_FORWARD = 1 << 14,
   DIR_WALK_PATHS_SLASH_BACK = 1 << 15,

   DIR_WALK_PATHS_SLASH_MASK = 0xc000,

   DIR_WALK_FORCEINT = 65536 /* force the enum to be signed integer */
};

/**
 * Invokes callback for item, depending on flags and _optional_ glob patterns,
 * in in the directory and it's sub-directories.
 *
 * @param path to walk.
 * @param _optional_ glob pattern for directories
 * @param _optional_ glob pattern for files
 * @param flags controlling the walk (enum dir_walk_flags).
 * @param callback to invoke for each item in walk that matches flags and _optional_ glob patterns
 * @param userdata passed to callback.
 *
 * @note the callback is invoked for each item that matches the flags and the _optional_
 *       glob pattern.
 *
 * @note the glob pattern for directories start matching on the first level after the input/root-directory
 *       i.e. input = "local/folder", item = "local/folder/subfolder" the glob pattern starts at "subfolder".
 *
 * @note the glob pattern for files matches against the file-part of the full filepath and _NOT_ the full path.
 *
 */
DIRUTIL_API enum dir_error dir_walkex( const char* path, unsigned int flags,
   const char* optional_glob_directories, const char* optional_glob_files,
   dir_walk_callback callback, void* userdata );
/* convenience (and backward compatibility) macro when no glob patterns is used */
#define dir_walk( path, flags, callback, userdata ) dir_walkex( path, flags, 0, 0, callback, userdata )

/**
 * Tidy up the path inplace. Convert all slashes to slash specified by input, remove
 * running slashes (i.e. '//' -> '/'), unquote the path (i.e. '"local/folder"' -> 'local/folder'),
 * trim white spaces and removes, if present, the trailing slash.
 *
 * @param path to tidy.
 * @param path separator to use ('/' or '\')
 * @param length of path to tidy (NB: length, not including null-terminator)
 * @return length of the resulting path (NB: length, not including null-terminator)
 *
 * @note UNC paths (paths that starts with '\\') will always keep the start ('\\')
 *       untouched regardless if requested slash was '/'.
 *       example: with requested '/'-substitution : '\\Server\folder' -> '\\Server/folder'
 */
DIRUTIL_API unsigned int dir_path_tidy( char* path, char slash, unsigned int path_len );

/**
 * Filename part of the path.
 * @param path.
 * @param length of path.
 * @param length of filename, if path contained a filename.
 * @return pointer to the filename part of the path, if path contained a filename, else null.
 */
DIRUTIL_API const char* dir_path_filename( const char* path, unsigned int path_len, unsigned int* filename_len );

/**
 * File extension part of the path.
 * @param path.
 * @param length of path.
 * @param length of extension, if path contained a filename with extension.
 * @return pointer to the extension part of the path, if path contained a filename with extension, else null.
 */
DIRUTIL_API const char* dir_path_extension( const char* path, unsigned int path_len, unsigned int* extension_len );

/**
 * Matches a Unix style glob-pattern, with added support for ** from ant, vs a path.
 *
 * Rules:
 * ?  - match one char except dir-separator.
 * *  - match any amount of chars ( including the empty string ) except dir-separator.
 * ** - match zero or more path-segments.
 * [] - match one of the chars the brackets except dir-separator, - can be used to specify a range.
 *      example:
 *         [abx], match an a, b or x ( lower case )
 *         [0-9], match any of the chars 0,1,2,3,4,5,6,7,8,9
 * {} - match any of the ,-separated strings within the brackets.
 *      example:
 *         {.txt,.doc}
 *
 * @note a glob-pattern that ends with a '**' considers the rest of the path a match,
 *       had it matched up to that point i.e. :
 *       dir_glob_match( "a/b/c**", "a/b/c/d/e/file.txt" ) is a match
 *
 * @note {} currently do not support sub-expressions of the other types. This could be added if there
 *       is any need for it.
 *
 * For more information see http://man7.org/linux/man-pages/man7/glob.7.html
 *
 * @param glob_pattern is an glob pattern (NB: use forward-slash ('/') to denote path separators).
 *
 * @param path is path to match, where both forward and back-slash is valid path separators,
 *        mixed and runs of both as well.
 *
 * @return DIR_GLOB_MATCH on match, DIR_GLOB_NO_MATCH on mismatch, otherwise error-code.
 */

enum dir_glob_result
{
   DIR_GLOB_MATCH,
   DIR_GLOB_NO_MATCH,
   DIR_GLOB_INVALID_PATTERN,

   DIR_GLOB_FORCEINT = 65536 /* force the enum to be signed integer */
};

DIRUTIL_API enum dir_glob_result dir_glob_match( const char* glob_pattern, const char* path );

#ifdef __cplusplus
   }
#endif

#endif

#if defined(DIRUTIL_IMPLEMENTATION)

#include <string.h>

#define dir_strlen32(s) (unsigned int)strlen( (s) )

#if defined( _WIN32 )
   #include <direct.h> /* rmdir */

   #ifndef WIN32_LEAN_AND_MEAN
      #define WIN32_LEAN_AND_MEAN
      #define WIN32_LEAN_AND_MEAN_UNDEF
   #endif
   #ifndef VC_EXTRALEAN
      #define VC_EXTRALEAN
      #define VC_EXTRALEAN_UNDEF
   #endif
   #include <Windows.h>
   #ifdef WIN32_LEAN_AND_MEAN_UNDEF
      #undef WIN32_LEAN_AND_MEAN_UNDEF
      #undef WIN32_LEAN_AND_MEAN
   #endif
   #ifdef VC_EXTRALEAN_UNDEF
      #undef VC_EXTRALEAN_UNDEF
      #undef VC_EXTRALEAN
   #endif
#else
   #include <sys/stat.h>
   #include <errno.h>
   #include <unistd.h>

   #include <dirent.h>
   #ifndef _DIRENT_HAVE_D_TYPE
      #error "_DIRENT_HAVE_D_TYPE undefined is unhandled (probably missing _DEFAULT_SOURCE or _BSD_SOURCE define)"
   #endif
#endif

/* forward declare glob-match implementation */
static enum dir_glob_result dir_glob_match_impl( const char* glob_pattern, const char* glob_end, const char* path );

#define DIR_IS_SEP(c) ((c) == '\\' || (c) == '/')

static char dir_walk_slash_by_flags( unsigned int flags )
{
   switch ( flags & DIR_WALK_PATHS_SLASH_MASK )
   {
   case DIR_WALK_PATHS_SLASH_FORWARD: return '/';
   case DIR_WALK_PATHS_SLASH_BACK: return '\\';
   default:
      return DIR_SEP_PLATFORM;
   }
}

static unsigned dir_walk_trim_convert_slashes_inplace( char* dest, char slash_substitute )
{
   char c;
   char *runner = dest, *odest = dest;
#if defined( _WIN32 )
   /* handle UNC paths */
   if ( runner[0] == DIR_SEP_PLATFORM && runner[1] == DIR_SEP_PLATFORM )
   {
      *dest++ = *runner++;
      *dest++ = *runner++;
      while ( DIR_IS_SEP( *runner ) )
         ++runner;
   }
#endif
   while ( ( c = *runner++ ) )
   {
      switch ( c ) {
      case '/':
      case '\\': {
         c = slash_substitute;
         /* next is slash ? */
         if ( DIR_IS_SEP( *runner ) )
            break;
      }
      /* FALLTHRU */
      default:
         *dest++ = c;
      }
   }

   *dest = '\0';
   return (unsigned)(dest - odest);
}

static enum dir_error dir_walk_impl( char* path_buffer, unsigned int path_len, unsigned path_buffer_size, unsigned root_path_len,
   const char* optional_glob_directories, const char* optional_glob_directories_end,
   const char* optional_glob_files, const char* optional_glob_files_end,
   unsigned int flags, dir_walk_callback callback, void* userdata )
{
#if defined ( _WIN32 )
   HANDLE ffh;
   WIN32_FIND_DATAA ffd;
#else
   struct dirent* ent;
   DIR* dir;
#endif
   enum dir_error result = DIR_ERROR_OK;

   char slash = dir_walk_slash_by_flags( flags );

   int should_walk_directories = ( flags & DIR_WALK_SINGLE_DIRECTORY ) == 0;
   int should_ignore_dot_directories = flags & DIR_WALK_IGNORE_DOT_DIRECTORIES;
   int should_ignore_dot_files = flags & DIR_WALK_IGNORE_DOT_FILES;
   int should_call_callback_directories = ( flags & DIR_WALK_ONLY_FILES ) == 0;
   int should_call_callback_files = ( flags & DIR_WALK_ONLY_DIRECTORIES ) == 0;
   unsigned int callback_path_offset = ( flags & DIR_WALK_ROOT_RELATIVE_PATHS ) ? ( root_path_len + 1 ) : 0;

#if defined ( _WIN32 )
   if ( path_buffer_size < 3 )
      return DIR_ERROR_PATH_TOO_DEEP;

   path_buffer[path_len]     = slash;
   path_buffer[path_len + 1] = '*';
   path_buffer[path_len + 2] = '\0';

   ffh = FindFirstFileA( path_buffer, &ffd );
   if ( ffh == INVALID_HANDLE_VALUE )
      return DIR_ERROR_PATH_DO_NOT_EXIST;
#else
   dir = opendir( path_buffer );
   if ( dir == 0x0 )
      return DIR_ERROR_PATH_DO_NOT_EXIST;
#endif

#if defined ( _WIN32 )
   do
#else
   while ( ( ent = readdir( dir ) ) != 0x0 )
#endif
   {
      unsigned int item_len, current_path_len;
      #if defined ( _WIN32 )
         const char* item_name = ffd.cFileName;
         int is_dir = ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
      #else
         const char* item_name = ent->d_name;
         int is_dir = ent->d_type == DT_DIR;
         if ( ent->d_type == DT_UNKNOWN )
         {
            struct stat s;
            if ( stat( path_buffer, &s ) != 0 )
               return DIR_ERROR_FAILED;
            is_dir = S_ISDIR( s.st_mode );
         }
      #endif

      if ( strcmp( item_name, "." ) == 0 || strcmp( item_name, ".." ) == 0 )
         continue;

      if ( is_dir && should_ignore_dot_directories && *item_name == '.' )
         continue;

      if ( !is_dir && should_ignore_dot_files && *item_name == '.' )
         continue;

      item_len = dir_strlen32( item_name );

      if ( path_buffer_size < item_len + 2 ) /* 2 == '/' + null-terminator */
      {
         result = DIR_ERROR_PATH_TOO_DEEP;
         break;
      }

      path_buffer[path_len] = slash;

      memcpy( &path_buffer[path_len + 1], item_name, item_len + 1 );

      current_path_len = path_len + item_len + 1;

      if ( is_dir && ( should_walk_directories || should_call_callback_directories ) )
      {
         int matches_pattern = !optional_glob_directories || DIR_GLOB_MATCH == dir_glob_match_impl( optional_glob_directories, optional_glob_directories_end, &path_buffer[root_path_len + 1] );

         if ( !matches_pattern )
            continue;

         if ( flags & DIR_WALK_DEPTH_FIRST )
         {
            if ( should_walk_directories )
               dir_walk_impl( path_buffer, path_len + item_len + 1, path_buffer_size - item_len - 1, root_path_len,
               optional_glob_directories, optional_glob_directories_end, optional_glob_files, optional_glob_files_end, flags, callback, userdata );

            if ( should_call_callback_directories )
               callback( path_buffer + callback_path_offset, current_path_len - callback_path_offset, DIR_ITEM_DIR, userdata );
         }
         else
         {
            if ( should_call_callback_directories )
               callback( path_buffer + callback_path_offset, current_path_len - callback_path_offset, DIR_ITEM_DIR, userdata );

            if ( should_walk_directories )
               dir_walk_impl( path_buffer, path_len + item_len + 1, path_buffer_size - item_len - 1, root_path_len,
               optional_glob_directories, optional_glob_directories_end, optional_glob_files, optional_glob_files_end, flags, callback, userdata );
         }
      }
      else if ( !is_dir && should_call_callback_files ) {
         int matches_pattern = !optional_glob_files || DIR_GLOB_MATCH == dir_glob_match_impl( optional_glob_files, optional_glob_files_end, item_name );

         if ( !matches_pattern )
            continue;

         callback( path_buffer + callback_path_offset, current_path_len - callback_path_offset, DIR_ITEM_FILE, userdata );
      }

   }
#if defined ( _WIN32 )
   while ( FindNextFileA( ffh, &ffd ) != 0 );
#endif
   path_buffer[path_len] = '\0';

#if defined ( _WIN32 )
   FindClose( ffh );
#else
   closedir( dir );
#endif
   return result;
}

static int dir_walk_iswhite( int c )
{
   return   ( ' '  == c ) ||
            ( '\t' == c ) ||
            ( '\n' == c ) ||
            ( '\v' == c ) ||
            ( '\f' == c ) ||
            ( '\r' == c );
}

static unsigned int dir_walk_path_trimwhite_unquote( char* path_buffer, unsigned int path_len )
{
   char* p = path_buffer;
   while ( ( path_len && dir_walk_iswhite( path_buffer[path_len - 1] ) ) || ( path_buffer[path_len - 1] == '"' ) )
      --path_len;

   while ( ( path_len && dir_walk_iswhite( *p ) ) || *p == '"' )
      --path_len, ++p;

   if ( p != path_buffer )
      memmove( path_buffer, p, path_len );

   path_buffer[path_len] = '\0';
   return path_len;
}

DIRUTIL_API unsigned int dir_path_tidy( char* path, char slash, unsigned int path_len )
{
   path_len = path_len ? path_len : dir_strlen32( path );
   path_len = dir_walk_path_trimwhite_unquote( path, path_len );
   path_len = dir_walk_trim_convert_slashes_inplace( path, slash );

   if ( path_len && DIR_IS_SEP( path[path_len - 1] ) )
   {
      --path_len;
      path[path_len] = '\0';
   }
   return path_len;
}

DIRUTIL_API const char* dir_path_filename( const char* path, unsigned int path_len, unsigned int* filename_len )
{
   unsigned full_path_len = path_len;
   if ( !path_len || DIR_IS_SEP( path[path_len - 1] ) )
   {
      *filename_len = 0;
      return 0;
   }

   while ( path_len && !DIR_IS_SEP( path[path_len - 1] ) )
      --path_len;

   *filename_len = full_path_len - path_len;
   return path + path_len;
}

DIRUTIL_API const char* dir_path_extension( const char* path, unsigned int path_len, unsigned int* extension_len )
{
   unsigned full_path_len = path_len;

   if ( !path_len || DIR_IS_SEP( path[path_len - 1] ) || path[path_len - 1] == '.' )
      goto no_extension;

   while ( path_len > 1 && path[path_len - 1] != '.' && !DIR_IS_SEP( path[path_len - 1] ) )
      --path_len;

   if ( path_len > 1 && !DIR_IS_SEP( path[path_len - 2] ) )
   {
      *extension_len = full_path_len - path_len;
      return path + path_len;
   }

no_extension:
   *extension_len = 0;
   return 0;
}

DIRUTIL_API enum dir_error dir_walkex( const char* path, unsigned int flags, const char* optional_glob_directories, const char *optional_glob_files, dir_walk_callback callback, void* userdata )
{
   char path_buffer[4096];
   unsigned int path_len = dir_strlen32( path );

   if ( path_len >= sizeof( path_buffer ) - 1 )
      return DIR_ERROR_FAILED;

   memcpy( path_buffer, path, path_len + 1 );

   path_len = dir_path_tidy( path_buffer, dir_walk_slash_by_flags( flags ), path_len );

   if ( !path_len )
      return DIR_ERROR_FAILED;

   return dir_walk_impl( path_buffer, path_len, sizeof( path_buffer ) - path_len, path_len,
      optional_glob_directories, ( ( optional_glob_directories && *optional_glob_directories ) ? optional_glob_directories + strlen( optional_glob_directories ) : 0),
      optional_glob_files, ( (optional_glob_files && *optional_glob_files ) ? optional_glob_files + strlen( optional_glob_files ) : 0),
      flags, callback, userdata );
}

DIRUTIL_API enum dir_error dir_create( const char* path )
{
#if defined( _WIN32 )
   if ( CreateDirectoryA( path, 0x0 ) )
      return DIR_ERROR_OK;
   if ( GetLastError() == ERROR_ALREADY_EXISTS )
      return DIR_ERROR_OK;
#else
   if ( mkdir( path, 0777 ) == 0 )
      return DIR_ERROR_OK;
   if ( errno == EEXIST )
      return DIR_ERROR_OK;
#endif
   return DIR_ERROR_FAILED;
}

static int dir_walk_rmitem( const char* path, unsigned int path_len, enum dir_item_type type, void* userdata )
{
   (void)path_len;
   switch ( type )
   {
   case DIR_ITEM_FILE:
#if defined( _WIN32 )
      if ( !DeleteFileA( path ) )
         *((enum dir_error*)userdata) = DIR_ERROR_FAILED;
#else
      if ( unlink( path ) != 0 )
         *((enum dir_error*)userdata) = DIR_ERROR_FAILED;
#endif
      break;
   case DIR_ITEM_DIR:
      #if defined ( _WIN32 )
         #pragma warning( push )
         #pragma warning( disable : 4996 ) /* 'rmdir': the POSIX name for this item is deprecated... */
      #endif
      if ( rmdir( path ) != 0 )
         *((enum dir_error*)userdata) = DIR_ERROR_FAILED;
      #if defined ( _WIN32 )
         #pragma warning( pop )
      #endif
      break;
   default:
      break;
   }
   return *((enum dir_error*)userdata) == DIR_ERROR_OK ? 0 : 1;
}

DIRUTIL_API enum dir_error dir_rmtree( const char* path )
{
   enum dir_error res = DIR_ERROR_OK;
   enum dir_error e = dir_walk( path, DIR_WALK_DEPTH_FIRST, dir_walk_rmitem, &res );
   if ( e != DIR_ERROR_OK )
      return e;
   if ( res != DIR_ERROR_OK )
      return res;

   #if defined ( _WIN32 )
      #pragma warning( push )
      #pragma warning( disable : 4996 ) /* 'rmdir': the POSIX name for this item is deprecated... */
   #endif
   return rmdir( path ) == 0 ? DIR_ERROR_OK : DIR_ERROR_FAILED;
   #if defined ( _WIN32 )
      #pragma warning( pop )
   #endif
}

DIRUTIL_API enum dir_error dir_mktree( const char* path )
{
   char path_buffer[4096];
   char* beg = path_buffer;
   unsigned int path_len = dir_strlen32( path );
   if ( path_len >= sizeof( path_buffer ) - 1 )
      return DIR_ERROR_FAILED;

   memcpy( path_buffer, path, path_len + 1 );

   path_len = dir_path_tidy( path_buffer, DIR_SEP_PLATFORM, path_len );

   /* if path is absolute, we need to start trying to create directories from the second '/',
    * otherwise we would try to create the dir "" */
   if ( *beg == DIR_SEP_PLATFORM )
      ++beg;

   for ( ;; )
   {
      enum dir_error err;
      char* sep = strchr( beg, DIR_SEP_PLATFORM );
      if ( sep == 0 )
         return dir_create( path_buffer );

      *sep = '\0';
      err = dir_create( path_buffer );
      if ( err != DIR_ERROR_OK )
         return err;

      *sep = DIR_SEP_PLATFORM;
      beg = sep + 1;
   }
}

static int dir_glob_match_range( const char* range_start, const char* range_end, char match_char )
{
   int match_return = 1;
   if ( *range_start == '!' )
   {
      match_return = 0;
      ++range_start;
   }

   while ( range_start != range_end + 1 )
   {
      if ( range_start[1] == '-' )
      {
         if ( range_start[0] <= match_char && match_char <= range_start[2] )
            return match_return;
         range_start += 3;
      }
      else
      {
         if ( *range_start == match_char )
            return match_return;
         ++range_start;
      }
   }

   return !match_return;
}

static int dir_glob_match_groups( const char* group_start, const char* group_end, const char* match_this )
{
   /* ... comma separated ... */
   const char* item_start = group_start;

   while ( item_start < group_end )
   {
      unsigned int item_len;
      const char* item_end = item_start;
      while ( *item_end != ',' && item_end != group_end + 1 )
         ++item_end;

      item_len = (unsigned int)( item_end - item_start );
      if ( strncmp( match_this, item_start, item_len ) == 0 )
         return (int)item_len;

      item_start = item_end + 1;
   }
   return -1;
}

/* find either 'a' or 'b' on 's' and returns the last occurrence of either 'a' or 'b' if found */
static const char *dir_strchr2( const char *s, int a, int b )
{
   while ( *s )
   {
      if ( *s == a || *s == b )
      {
         while ( s[1] == a || s[1] == b )
            ++s;
         return s;
      }
      ++s;
   }
   return 0;
}

static enum dir_glob_result dir_glob_match_impl( const char* glob_pattern, const char* glob_end, const char* path )
{
   const char* unverified = path;

   while ( glob_pattern != glob_end )
   {
      switch ( *glob_pattern )
      {
      case '\0':
         return DIR_GLOB_NO_MATCH;
      case '*':
      {
         switch ( glob_pattern[1] )
         {
         case '\0':
         {
            /* try to find a path-separator in unverified since a pattern without a path-separator should only match files. */
            if ( dir_strchr2( unverified, '/', '\\' ) )
               return DIR_GLOB_NO_MATCH; /* should match directories */
            return DIR_GLOB_MATCH;
         }

         case '*':
         {
            const char* sub_search;
            if ( glob_pattern[2] != '/' )
               return glob_pattern[2] == '\0' ? DIR_GLOB_MATCH : DIR_GLOB_INVALID_PATTERN; /* a pattern that that ends with a '**' matches the rest */

            for (sub_search = unverified - 1;
               sub_search;
               sub_search = dir_strchr2( sub_search + 1, '/', '\\' ) )
            {
               enum dir_glob_result res;
               ++sub_search;
               res = dir_glob_match_impl( glob_pattern + 3, glob_end, sub_search );
               if ( res != DIR_GLOB_NO_MATCH )
                  return res;
            }
            return DIR_GLOB_NO_MATCH;
         }

         default:
         {
            /* search in unverified for char */
            const char* next = unverified;
            while ( *next && ( *next != glob_pattern[1] ) && !DIR_IS_SEP( *next ) )
               ++next;

            switch ( *next )
            {
            case '\0':
               return DIR_GLOB_NO_MATCH; /* failed, could not find char after * */
            case '\\':
            case '/':
               if ( glob_pattern[1] == '/' )
               {
                  unverified = next + 1;
                  while ( DIR_IS_SEP( *unverified ) )
                     ++unverified; /* skip running slashes */
                  glob_pattern += 2;
               }
               else
                  return DIR_GLOB_NO_MATCH;
               break;
            default:
               unverified = next;
               ++glob_pattern;
               break;
            }
         }
         break;

         }
      }
      break;
      case '?':
      {
         if ( DIR_IS_SEP( *unverified ) )
            return DIR_GLOB_NO_MATCH;
         ++unverified;
         ++glob_pattern;
      }
      break;

      case '[':
      {
         const char* range_start = glob_pattern + 1;
         const char* range_end = strchr( range_start, ']' );
         if ( range_end == 0x0 )
            return DIR_GLOB_INVALID_PATTERN;

         if ( !dir_glob_match_range( range_start, range_end - 1, *unverified ) )
            return DIR_GLOB_NO_MATCH;

         glob_pattern = range_end + 1;
         ++unverified;
      }
      break;

      case '{':
      {
         int match_len;
         const char* group_start = glob_pattern + 1;
         const char* group_end = group_start;
         while ( group_end != glob_end && *group_end != '}' )
            ++group_end;

         match_len = dir_glob_match_groups( group_start, group_end - 1, unverified );
         if ( match_len < 0 )
            return DIR_GLOB_NO_MATCH;

         glob_pattern = group_end + 1;
         unverified += match_len;
      }
      break;

      default:
      {
         if ( *unverified != *glob_pattern )
            return DIR_GLOB_NO_MATCH;
         ++unverified;
         ++glob_pattern;
      }
      break;
      }
   }

   return *unverified == '\0' ? DIR_GLOB_MATCH : DIR_GLOB_NO_MATCH;
}

DIRUTIL_API enum dir_glob_result dir_glob_match( const char* glob_pattern, const char* path )
{
   return dir_glob_match_impl( glob_pattern, glob_pattern + dir_strlen32( glob_pattern ), path );
}

#endif

/* clang-format on */
