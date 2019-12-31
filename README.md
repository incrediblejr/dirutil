# dirutil
A collection of directory related functions, like walk, create and delete folders and helper functions for 'tidying' up paths and extracting filename and extension parts of a path.

Also included is a Unix style glob-pattern matcher, with added support for ** from ant (vs a path).

# notes
This is a single-header(*), ANSI C conversion of [dirutil](https://github.com/wc-duck/dirutil) by [wc-duck](https://github.com/wc-duck) with fixes and added features.

The added features/changes include:

1) all paths returned to user is 'tidy', which means that the path separator is controlled/specified by user, no mixed path separators and no running path separators (no 'double' //).

2) function for making a path 'tidy' ('dir_path_tidy')

3) functions for extracting filename and extension from path ('dir_path_filename'+'dir_path_extension')

4) more control of the directory walk and optional file/directory-filtering, by glob-patterns, at the library level instead of user having to do the filtering in the callback.

   the following flags/options can be now be used to guide and control the directory walk:
      - 'DIR_WALK_SINGLE_DIRECTORY' (only invoke user callback for files/folders in the input/root-directory)
      - 'DIR_WALK_ONLY_DIRECTORIES' (only invoke user callback for directories)
      - 'DIR_WALK_ONLY_FILES' (only invoke user callback for files)
      - 'DIR_WALK_IGNORE_DOT_DIRECTORIES' (ignore directories that starts with a dot (.))
      - 'DIR_WALK_IGNORE_DOT_FILES' (ignore files that starts with a dot (.))
      - 'DIR_WALK_ROOT_RELATIVE_PATHS' (paths in user callback have input/root-directory part stripped)
      - flag to specify the path-separator for paths returned to user in callback
      - optional directory and file glob patterns, to do the filtering on the library level and not in user callback.

5) handling of both forward and back-slash path separators in the glob-matcher's input-path and handling of glob patters that ends with '**'

(*) which means that dirutil.h header provides both the interface and implementation.

# examples

## Print directory recursively.

```c
   /* as the header provides both the interface and the implementation means that you have to define DIRUTIL_IMPLEMENTATION in *one* file that includes the header */
   #define DIRUTIL_IMPLEMENTATION
   #include "dirutil.h"
   #include <stdio.h>

   int dir_walk_print( const char* path, unsigned int path_len, enum dir_item_type type, void* userdata )
   {
      printf( "%s %s\n", type == DIR_ITEM_FILE ? "FILE:" : "DIR: ", path );
      return 0;
   }

   int main( int argc, const char** argv )
   {
      return dir_walk( argc > 1 ? argv[1] : ".", DIR_WALK_DEPTH_FIRST, dir_walk_print, 0 ) == DIR_ERROR_OK;
   }
```

## Print files matching glob-pattern.

```c
   #define DIRUTIL_IMPLEMENTATION
   #include "dirutil.h"
   #include <stdio.h>
   #include <assert.h>

   int dir_walk_print( const char* path, unsigned int path_len, enum dir_item_type type, void* userdata )
   {
      assert(type == DIR_ITEM_FILE); /* will never trigger as we specified the DIR_WALK_ONLY_FILES-flag */
      unsigned filename_len;
      const char* filename = dir_path_filename( path, path_len, &filename_len );
      unsigned pathpart_len = path_len - filename_len - 1; /* -1 to remove the trailing path separator */

      printf( "path:%.*s - file:%s\n", pathpart_len, path, filename );

      return 0;
   }

   int main( int argc, const char** argv )
   {
      const char* dir = argc > 1 ? argv[1] : ".";
      const char* glob_pattern_folders = 0; /* for clarity */
      const char* glob_pattern_files = "*.md";
      unsigned dir_walk_flags = DIR_WALK_ONLY_FILES | DIR_WALK_DEPTH_FIRST;

      return dir_walkex( dir, dir_walk_flags, glob_pattern_folders, glob_pattern_files, dir_walk_print, 0 ) == DIR_ERROR_OK;
   }
```
