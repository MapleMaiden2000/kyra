#include "kyra/core/platform/filesystem/filesystem.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#if KYRA_PLATFORM_WINDOWS
    #include <direct.h>
    #include <windows.h>
    
    // Platform-specific 64-bit file operations
    #ifdef _WIN32
        #define kyra_fseek _fseeki64
        #define kyra_ftell _ftelli64
    #else
        #define kyra_fseek fseek
        #define kyra_ftell ftell
    #endif
#elif KYRA_PLATFORM_LINUX
    #include <unistd.h>
    #include <sys/stat.h>
#endif 


// Helper functions ----------------------------------------------------- // 

static ConstStr _platform_filesystem_configure_op_mode(FilesystemIOMode io_mode, FilesystemFileMode file_mode) {
    switch (io_mode) {
		case FILESYSTEM_IO_MODE_READ:
            switch (file_mode) {
                case FILESYSTEM_FILE_MODE_TEXT:
                    return "r";
                case FILESYSTEM_FILE_MODE_BINARY:
                    return "rb";
            }
            break;
		
		case FILESYSTEM_IO_MODE_WRITE:
            switch (file_mode) {
                case FILESYSTEM_FILE_MODE_TEXT:
                    return "w";
                case FILESYSTEM_FILE_MODE_BINARY:
                    return "wb";
            }
            break;
		
		case FILESYSTEM_IO_MODE_READ_WRITE:
            switch (file_mode) {
                case FILESYSTEM_FILE_MODE_TEXT:
                    return "r+";
                case FILESYSTEM_FILE_MODE_BINARY:
                    return "rb+";
            }
            break;
		
		case FILESYSTEM_IO_MODE_APPEND:
            switch (file_mode) {
                case FILESYSTEM_FILE_MODE_TEXT:
                  return "a";
                case FILESYSTEM_FILE_MODE_BINARY:
                    return "a+b";
            }
            break;
		
		// Invalid mode
		default:
		    return 0;
	}
	
	return 0;
} 


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API FilesystemResult platform_filesystem_file_open(ConstStr path, const FilesystemIOMode io_mode, const FilesystemFileMode file_mode, File *out_file) {
    if (!path) return FILESYSTEM_ERROR_FILEPATH_NULL;
    if (!out_file) return FILESYSTEM_ERROR_REF_OUT_FILE_NULL;
    if (out_file->active) return FILESYSTEM_ERROR_FILE_ALREADY_OPEN;

    // Configure operation mode
    ConstStr op_mode = _platform_filesystem_configure_op_mode(io_mode, file_mode);
    if (!op_mode) return FILESYSTEM_ERROR_INVALID_OPERATION_MODE;

    // Open file
    out_file->stream = fopen(path, op_mode);
    if (!out_file->stream) return FILESYSTEM_ERROR_FAILED_TO_OPEN_FILE;

    // Configure file properties
    out_file->io_mode = io_mode;
    out_file->file_mode = file_mode;
    out_file->active = true;

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_file_close(File *file) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;

    // Close file
    if (fclose(file->stream) != 0) return FILESYSTEM_ERROR_FAILED_TO_CLOSE_FILE;

    // Reset file properties
    memset(file, 0, sizeof(File));

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_file_exists(ConstStr path, Bool *out_exists) {
    if (!path) return FILESYSTEM_ERROR_FILEPATH_NULL;

    // Check if file of specified path exists
    FILE *stream = fopen(path, "r");
    if (out_exists) *out_exists = (stream != NULL); 
    if (stream) fclose(stream);

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_file_size(File *file, ByteSize *out_size) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;
    
    FILE *stream = file->stream;
    ByteSize file_size = 0;
    
    // Store current position for restoration
    Int64 current = kyra_ftell(stream);

    // Seek to end
    kyra_fseek(stream, 0, SEEK_END);
    file_size = kyra_ftell(stream);

    // Restore to previous position
    kyra_fseek(stream, current, SEEK_SET);

    // Save to ref
    if (out_size) *out_size = file_size;

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_read_line(File *file, ByteSize *out_read_bytes, Str *out_read_buffer) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;
    if (!*out_read_buffer) return FILESYSTEM_ERROR_REF_OUT_READ_BUFFER_NULL;
 
    // Read line
    if (fgets(*out_read_buffer, KYRA_LINE_MAX_LENGTH, file->stream) == NULL)
		return FILESYSTEM_ERROR_FAILED_TO_READ_LINE;

    // Save to ref
    if (out_read_bytes) *out_read_bytes = strlen(*out_read_buffer);

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_read_all(File *file, ByteSize *out_read_bytes, Str *out_read_buffer) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;
    if (!*out_read_buffer) return FILESYSTEM_ERROR_REF_OUT_READ_BUFFER_NULL;

    // Get file size
    ByteSize file_size = 0;
    FilesystemResult file_size_result = platform_filesystem_file_size(file, &file_size); 
    if (file_size_result != FILESYSTEM_SUCCESS) return file_size_result;
    
    // Return if file is empty
    if (file_size == 0) {
        *out_read_bytes = 0;
        return FILESYSTEM_SUCCESS;
    }

    // Read all
    ByteSize bytes_read = fread(*out_read_buffer, sizeof(Char), file_size, file->stream);
    if (bytes_read != file_size) return FILESYSTEM_ERROR_FAILED_TO_READ_ALL;

    // Save to ref
    if (out_read_bytes) *out_read_bytes = bytes_read;

    // Null-terminate if file mode is text
    if (file->file_mode == FILESYSTEM_FILE_MODE_TEXT)
        (*out_read_buffer)[bytes_read] = '\0';

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_read_data(File *file, const ByteSize bytes_to_read, VoidPtr *out_read_buffer) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;
    if (!*out_read_buffer) return FILESYSTEM_ERROR_REF_OUT_READ_BUFFER_NULL;
    
    // Return if no bytes to read
    if (bytes_to_read == 0) return FILESYSTEM_SUCCESS;

    // Read data
    ByteSize bytes_read = fread(*out_read_buffer, 1, bytes_to_read, file->stream);
    if (bytes_read != bytes_to_read) return FILESYSTEM_ERROR_FAILED_TO_READ_DATA;

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_read_binary_uint32(File *file, ByteSize *out_read_count, UInt32 **out_read_buffer) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;
    if (!*out_read_buffer) return FILESYSTEM_ERROR_REF_OUT_READ_BUFFER_NULL;

    // Get file size
    ByteSize file_size = 0;
    FilesystemResult file_size_result = platform_filesystem_file_size(file, &file_size); 
    if (file_size_result != FILESYSTEM_SUCCESS) return file_size_result;

    if (file_size % sizeof(UInt32) != 0) return FILESYSTEM_ERROR_FILE_SIZE_INVALID_TO_READ;

    // Allocate buffer to contain file data
    UInt32 *buffer = malloc(file_size);
    if (!buffer) return FILESYSTEM_ERROR_FAILED_TO_ALLOCATE_BUFFER;

    // Read binary data (uint32)
    ByteSize count = file_size / sizeof(UInt32);
    ByteSize read_count = fread(buffer, sizeof(UInt32), count, file->stream);

    if (read_count != count) {
        // Free the data buffer if fread returned mismatching size (indicated incorrect reading)
        free(buffer);
        
        return FILESYSTEM_ERROR_FAILED_TO_READ_BINARY_DATA; 
    }

    // Save to refs
    {
        if (out_read_count) *out_read_count = count;
        if (out_read_buffer) *out_read_buffer = buffer;
    }

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_write_line(File *file, ConstStr line) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;
    if (!line) return FILESYSTEM_ERROR_LINE_NULL;

    // Write line
    if (fprintf(file->stream, "%s\n", line) < 0) return FILESYSTEM_ERROR_FAILED_TO_WRITE_LINE;

    // Flush the file stream to ensure line is written to file
    if (fflush(file->stream) != 0) return FILESYSTEM_ERROR_FAILED_TO_FLUSH_FILE;

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_write_data(File *file, const ByteSize bytes_to_write, const VoidPtr data) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;
    if (!data) return FILESYSTEM_ERROR_DATA_NULL;

    // Return if if no bytes to write
    if (bytes_to_write == 0) return FILESYSTEM_SUCCESS;

    // Write data
    if (fwrite(data, sizeof(Char), bytes_to_write, file->stream) != bytes_to_write)
        return FILESYSTEM_ERROR_FAILED_TO_WRITE_DATA;

    // Flush the file stream to ensure line is written to file
    if (fflush(file->stream) != 0) return FILESYSTEM_ERROR_FAILED_TO_FLUSH_FILE;
    
    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_write_binary_uint32(File *file, UInt32 value) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;

    // Write binary data (uint32)
    if (fwrite((VoidPtr)&value, sizeof(UInt32), 1, file->stream) != 1)
        return FILESYSTEM_ERROR_FAILED_TO_WRITE_BINARY_DATA;

    // Flush the file stream to ensure line is written to file
    if (fflush(file->stream) != 0) return FILESYSTEM_ERROR_FAILED_TO_FLUSH_FILE;

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_extract_filename(ConstStr path, const ByteSize capacity, Str *out_filename) {
    if (!path) return FILESYSTEM_ERROR_FILEPATH_NULL;
    if (capacity == 0) return FILESYSTEM_ERROR_CAPACITY_ZERO;
    if (!*out_filename) return FILESYSTEM_ERROR_REF_OUT_FILENAME_NULL;
    
    // Locate the start of the filename
    ConstStr last_bwd = strrchr(path, '\\');    // Very last backward slash in the path
	ConstStr last_fwd = strrchr(path, '/');     // Very last forward slash in the path
    ConstStr start = (last_bwd > last_fwd) ? last_bwd : last_fwd;   // Get the one appear last in the path
    
    // If no slashes, indicating lack of directory, assign the entire "path" to be the start
    // Otherwise, assign the position after the slash to be the start
    start = (start == NULL) ? path : start + 1; 

    // Locate the start of the extension
    ConstStr ext = strrchr(start, '.');

    // Post-extension removal length
    // If no fullstop, indicating lack of extension, assign full length
    ByteSize length = (!ext) ? strlen(start) : (ByteSize)(ext - start);    

    // Check if filename fits in output buffer
    if (length >= capacity) return FILESYSTEM_ERROR_CAPACITY_INSUFFICIENT_TO_EXTRACT;

    // Save to ref
    snprintf(*out_filename, capacity, "%.*s", length, start); 

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_extract_extension(ConstStr path, const ByteSize capacity, Str *out_extension) {
    if (!path) return FILESYSTEM_ERROR_FILEPATH_NULL;
    if (capacity == 0) return FILESYSTEM_ERROR_CAPACITY_ZERO;
    if (!*out_extension) return FILESYSTEM_ERROR_REF_OUT_EXTENSION_NULL;

    // Locate the last occurring directory separator
    ConstStr last_bwd = strrchr(path, '\\');    // Very last backward slash in the path
	ConstStr last_fwd = strrchr(path, '/');     // Very last forward slash in the path
    ConstStr last_sep = (last_bwd > last_fwd) ? last_bwd : last_fwd;   // Get the one appear last in the path

    // Locate the dot (only after the last separator)
    ConstStr start = (!last_sep) ? path : last_sep + 1;
    
    // Extension
    ConstStr ext = strrchr(start, '.');
    if (!ext) return FILESYSTEM_ERROR_NO_EXTENSION_FOUND;

    // Check if extension fits in output buffer 
	ByteSize length = strlen(ext);
    if (length >= capacity) return FILESYSTEM_ERROR_CAPACITY_INSUFFICIENT_TO_EXTRACT;

    // Save to ref
    snprintf(*out_extension, capacity, "%.*s", length, ext);

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_extract_directory(ConstStr path, const ByteSize capacity, Str *out_directory) {
    if (!path) return FILESYSTEM_ERROR_FILEPATH_NULL;
    if (capacity == 0) return FILESYSTEM_ERROR_CAPACITY_ZERO;
    if (!*out_directory) return FILESYSTEM_ERROR_REF_OUT_DIRECTORY_NULL;

    // Locate the last occurring directory separator
    ConstStr last_bwd = strrchr(path, '\\');    // Very last backward slash in the path
	ConstStr last_fwd = strrchr(path, '/');     // Very last forward slash in the path
    ConstStr last_sep = (last_bwd > last_fwd) ? last_bwd : last_fwd;   // Get the one appear last in the path

    // If no directory separator, indicating directory is current
    if (!last_sep) {
        // Save as '.' which means 'current directory'
        if (capacity < 2) return FILESYSTEM_ERROR_CAPACITY_INSUFFICIENT_TO_EXTRACT;
        snprintf(*out_directory, capacity, ".");

        return FILESYSTEM_SUCCESS;
    }

    // Otherwise...
    // Calculate the length of the directory
    ByteSize length = (ByteSize)(last_sep - path);
    
    // Check if directory fits in output buffer
    if (length >= capacity) return FILESYSTEM_ERROR_CAPACITY_INSUFFICIENT_TO_EXTRACT;

    // Save to ref
    snprintf(*out_directory, capacity, "%.*s", length, path);

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_seek_to_position(File *file, const Int64 position) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;

    // Prevents targeting before start-of-file
	if (position < 0) return FILESYSTEM_ERROR_INVALID_SEEK_POSITION;

    // Seek to target position
    if (kyra_fseek(file->stream, position, SEEK_SET) != 0) return FILESYSTEM_ERROR_FAILED_TO_SEEK_FILE;

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_seek_from_current(File *file, const Int64 delta) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;

    // Seek from current position
    if (kyra_fseek(file->stream, delta, SEEK_CUR) != 0) return FILESYSTEM_ERROR_FAILED_TO_SEEK_FILE;

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_get_seek_position(File *file, Int64 *out_position) {
    if (!file) return FILESYSTEM_ERROR_FILE_NULL;
    if (!file->active || !file->stream) return FILESYSTEM_ERROR_FILE_NOT_OPEN;

    // Save to ref
    if (out_position) *out_position = kyra_ftell(file->stream);

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_create_directory(ConstStr path) {
	if (!path) return FILESYSTEM_ERROR_FILEPATH_NULL;

    // Create directory
    Int32 create_dir = 0;
    #if KYRA_PLATFORM_WINDOWS
		create_dir = _mkdir(path);
		if (create_dir != 0) return FILESYSTEM_ERROR_FAILED_TO_CREATE_DIRECTORY;
	#elif KYRA_PLATFORM_LINUX
		create_dir = mkdir(path, 0755);
		if (create_dir != 0) return FILESYSTEM_ERROR_FAILED_TO_CREATE_DIRECTORY;
	#endif

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_remove_directory(ConstStr path) {
	if (!path) return FILESYSTEM_ERROR_FILEPATH_NULL;

    // Remove directory
	Int32 remove_dir = 0;
    #if KYRA_PLATFORM_WINDOWS
		remove_dir = _rmdir(path);
		if (remove_dir != 0) return FILESYSTEM_ERROR_FAILED_TO_REMOVE_DIRECTORY;
	#elif KYRA_PLATFORM_LINUX
		remove_dir = rmdir(path);
		if (remove_dir != 0) return FILESYSTEM_ERROR_FAILED_TO_REMOVE_DIRECTORY;
	#endif

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API FilesystemResult platform_filesystem_directory_exists(ConstStr path, Bool *out_exists) {
	if (!path) return FILESYSTEM_ERROR_FILEPATH_NULL;

    #if KYRA_PLATFORM_WINDOWS
		DWORD file_attributes = GetFileAttributes(path);
		
        // Check if the file exists and is a directory
		if (file_attributes == INVALID_FILE_ATTRIBUTES) {
			if (out_exists) *out_exists = false;
		} else {
			if (out_exists) *out_exists = (file_attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
		}
	
	#elif KYRA_PLATFORM_LINUX
		struct stat st = {0};

		// Check if the file exists and is a directory
		if (stat(path, &st) == 0) {
			if (out_exists) *out_exists = (st.st_mode & S_IFDIR) != 0;
		} else {
			if (out_exists) *out_exists = false;
		}
	#endif

    return FILESYSTEM_SUCCESS;
}

KYRA_ENGINE_API ConstStr platform_filesystem_result_to_string(const FilesystemResult result) {
    switch (result) {
        case FILESYSTEM_SUCCESS:                  					return "FILESYSTEM_SUCCESS";
	    
        case FILESYSTEM_ERROR_REF_OUT_FILE_NULL:					return "FILESYSTEM_ERROR_REF_OUT_FILE_NULL";
	    case FILESYSTEM_ERROR_FILEPATH_NULL:						return "FILESYSTEM_ERROR_FILEPATH_NULL";
	    case FILESYSTEM_ERROR_FILE_ALREADY_OPEN:					return "FILESYSTEM_ERROR_FILE_ALREADY_OPEN";
	    case FILESYSTEM_ERROR_FILE_NULL:							return "FILESYSTEM_ERROR_FILE_NULL";
	    case FILESYSTEM_ERROR_FILE_NOT_OPEN:						return "FILESYSTEM_ERROR_FILE_NOT_OPEN";
	    case FILESYSTEM_ERROR_LINE_NULL:							return "FILESYSTEM_ERROR_LINE_NULL";
	    case FILESYSTEM_ERROR_DATA_NULL:							return "FILESYSTEM_ERROR_DATA_NULL";
	    case FILESYSTEM_ERROR_CAPACITY_ZERO:						return "FILESYSTEM_ERROR_CAPACITY_ZERO";
	    case FILESYSTEM_ERROR_CAPACITY_INSUFFICIENT_TO_EXTRACT:		return "FILESYSTEM_ERROR_CAPACITY_INSUFFICIENT_TO_EXTRACT";
	    case FILESYSTEM_ERROR_INVALID_OPERATION_MODE:				return "FILESYSTEM_ERROR_INVALID_OPERATION_MODE";
	    case FILESYSTEM_ERROR_FAILED_TO_OPEN_FILE:					return "FILESYSTEM_ERROR_FAILED_TO_OPEN_FILE";
	    case FILESYSTEM_ERROR_FAILED_TO_CLOSE_FILE:					return "FILESYSTEM_ERROR_FAILED_TO_CLOSE_FILE";
	    case FILESYSTEM_ERROR_FAILED_TO_READ_LINE:					return "FILESYSTEM_ERROR_FAILED_TO_READ_LINE";
	    case FILESYSTEM_ERROR_FAILED_TO_READ_ALL:					return "FILESYSTEM_ERROR_FAILED_TO_READ_ALL";
	    case FILESYSTEM_ERROR_FAILED_TO_READ_DATA:					return "FILESYSTEM_ERROR_FAILED_TO_READ_DATA";
	    case FILESYSTEM_ERROR_FAILED_TO_ALLOCATE_BUFFER:			return "FILESYSTEM_ERROR_FAILED_TO_ALLOCATE_BUFFER";
	    case FILESYSTEM_ERROR_FAILED_TO_READ_BINARY_DATA:			return "FILESYSTEM_ERROR_FAILED_TO_READ_BINARY_DATA";
	    case FILESYSTEM_ERROR_FAILED_TO_WRITE_LINE:					return "FILESYSTEM_ERROR_FAILED_TO_WRITE_LINE";
	    case FILESYSTEM_ERROR_FAILED_TO_FLUSH_FILE:					return "FILESYSTEM_ERROR_FAILED_TO_FLUSH_FILE";
	    case FILESYSTEM_ERROR_FAILED_TO_WRITE_DATA:					return "FILESYSTEM_ERROR_FAILED_TO_WRITE_DATA";
	    case FILESYSTEM_ERROR_FAILED_TO_WRITE_BINARY_DATA:			return "FILESYSTEM_ERROR_FAILED_TO_WRITE_BINARY_DATA";
	    case FILESYSTEM_ERROR_FAILED_TO_SEEK_FILE:					return "FILESYSTEM_ERROR_FAILED_TO_SEEK_FILE";
	    case FILESYSTEM_ERROR_FAILED_TO_CREATE_DIRECTORY:			return "FILESYSTEM_ERROR_FAILED_TO_CREATE_DIRECTORY";
	    case FILESYSTEM_ERROR_FAILED_TO_REMOVE_DIRECTORY:			return "FILESYSTEM_ERROR_FAILED_TO_REMOVE_DIRECTORY";
	    case FILESYSTEM_ERROR_FILE_SIZE_INVALID_TO_READ:			return "FILESYSTEM_ERROR_FILE_SIZE_INVALID_TO_READ";
	    case FILESYSTEM_ERROR_REF_OUT_READ_BUFFER_NULL:				return "FILESYSTEM_ERROR_REF_OUT_READ_BUFFER_NULL";
	    case FILESYSTEM_ERROR_REF_OUT_FILENAME_NULL:				return "FILESYSTEM_ERROR_REF_OUT_FILENAME_NULL";
	    case FILESYSTEM_ERROR_REF_OUT_EXTENSION_NULL:				return "FILESYSTEM_ERROR_REF_OUT_EXTENSION_NULL";
	    case FILESYSTEM_ERROR_REF_OUT_DIRECTORY_NULL:				return "FILESYSTEM_ERROR_REF_OUT_DIRECTORY_NULL";
	    case FILESYSTEM_ERROR_NO_EXTENSION_FOUND:					return "FILESYSTEM_ERROR_NO_EXTENSION_FOUND";
	    case FILESYSTEM_ERROR_INVALID_SEEK_POSITION:				return "FILESYSTEM_ERROR_INVALID_SEEK_POSITION";
        
        default:                                                    return "UNKNOWN_FILESYSTEM_RESULT";
    }
}
