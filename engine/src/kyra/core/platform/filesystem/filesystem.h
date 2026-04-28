#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/filesystem.h"


// API functions -------------------------------------------------------- //

KYRA_ENGINE_API FilesystemResult    platform_filesystem_file_open(ConstStr path, const FilesystemIOMode io_mode, const FilesystemFileMode file_mode, File *out_file);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_file_close(File *file);

KYRA_ENGINE_API FilesystemResult    platform_filesystem_file_exists(ConstStr path, Bool *out_exists);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_file_size(File *file, ByteSize *out_size);

KYRA_ENGINE_API FilesystemResult    platform_filesystem_read_line(File *file, ByteSize *out_read_bytes, Str *out_read_buffer);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_read_all(File *file, ByteSize *out_read_bytes, Str *out_read_buffer);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_read_data(File *file, const ByteSize bytes_to_read, VoidPtr *out_read_buffer);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_read_binary_uint32(File *file, ByteSize *out_read_count, UInt32 **out_read_buffer);

KYRA_ENGINE_API FilesystemResult    platform_filesystem_write_line(File *file, ConstStr line);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_write_data(File *file, const ByteSize bytes_to_write, const VoidPtr data);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_write_binary_uint32(File *file, UInt32 value);

KYRA_ENGINE_API FilesystemResult    platform_filesystem_extract_filename(ConstStr path, const ByteSize capacity, Str *out_filename);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_extract_extension(ConstStr path, const ByteSize capacity, Str *out_extension);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_extract_directory(ConstStr path, const ByteSize capacity, Str *out_directory);

KYRA_ENGINE_API FilesystemResult    platform_filesystem_seek_to_position(File *file, const Int64 position);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_seek_from_current(File *file, const Int64 delta);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_get_seek_position(File *file, Int64 *out_position);

KYRA_ENGINE_API FilesystemResult    platform_filesystem_create_directory(ConstStr path);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_remove_directory(ConstStr path);
KYRA_ENGINE_API FilesystemResult    platform_filesystem_directory_exists(ConstStr path, Bool *out_exists);

KYRA_ENGINE_API ConstStr            platform_filesystem_result_to_string(FilesystemResult result);


