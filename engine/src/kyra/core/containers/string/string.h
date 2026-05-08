#pragma once

#include "kyra/defines/shared.h"
#include "kyra/defines/core/containers.h"


// API functions ------------------------------------------------------- //

KYRA_ENGINE_API ContainerResult     container_string_construct(ConstStr value, String *out_string);
KYRA_ENGINE_API ContainerResult     container_string_construct_empty(String *out_string);
KYRA_ENGINE_API ContainerResult     container_string_construct_reserved(const ByteSize capacity, String *out_string);
KYRA_ENGINE_API ContainerResult     container_string_construct_from_chars(const Char c, const ByteSize count, String *out_string);
KYRA_ENGINE_API ContainerResult     container_string_construct_formatted(String *out_string, ConstStr format, ...);

KYRA_ENGINE_API ContainerResult     container_string_destruct(String *string);

KYRA_ENGINE_API ContainerResult     container_string_clear(String *string);
KYRA_ENGINE_API ContainerResult     container_string_set(String *string, ConstStr value);

KYRA_ENGINE_API ContainerResult     container_string_append(String *string, ConstStr value);
KYRA_ENGINE_API ContainerResult     container_string_append_chars(String *string, const Char c, const ByteSize count);
KYRA_ENGINE_API ContainerResult     container_string_append_formatted(String *string, ConstStr format, ...);

KYRA_ENGINE_API ContainerResult     container_string_detach(String *string);
KYRA_ENGINE_API ContainerResult     container_string_detach_ranged(String *string, const ByteSize range);

KYRA_ENGINE_API ContainerResult     container_string_insert(String *string, const ByteSize index, ConstStr substr);
KYRA_ENGINE_API ContainerResult     container_string_insert_chars(String *string, const ByteSize index, const Char c, const ByteSize count);
KYRA_ENGINE_API ContainerResult     container_string_insert_formatted(String *string, const ByteSize index, ConstStr format, ...);

KYRA_ENGINE_API ContainerResult     container_string_remove(String *string, ConstStr substr, const Bool remove_all);
KYRA_ENGINE_API ContainerResult     container_string_remove_chars(String *string, const Char c, const Bool remove_all);
KYRA_ENGINE_API ContainerResult     container_string_remove_at(String *string, const ByteSize index, const ByteSize count);

KYRA_ENGINE_API ContainerResult     container_string_replace_char(String *string, const Char old_char, const Char new_char);
KYRA_ENGINE_API ContainerResult     container_string_replace_substring(String *string, ConstStr old_substr, ConstStr new_substr);

KYRA_ENGINE_API ContainerResult     container_string_to_lower(String *string);
KYRA_ENGINE_API ContainerResult     container_string_to_upper(String *string);

KYRA_ENGINE_API ContainerResult     container_string_trim_left(String *string);
KYRA_ENGINE_API ContainerResult     container_string_trim_right(String *string);
KYRA_ENGINE_API ContainerResult     container_string_trim(String *string);

KYRA_ENGINE_API ContainerResult     container_string_filter_char(String *string, const Char c, const Bool keep);

KYRA_ENGINE_API ContainerResult     container_string_substring(const String string, const ByteSize index, const ByteSize size, String *out_new_substring);

KYRA_ENGINE_API ContainerResult     container_string_equals(const String left, ConstStr right, Bool *out_result);
KYRA_ENGINE_API ContainerResult     container_string_equals_string(const String left, const String right, Bool *out_result);

KYRA_ENGINE_API ContainerResult     container_string_search(const String string, ConstStr substr, Int32 *out_index);
KYRA_ENGINE_API ContainerResult     container_string_search_string(const String string, const String substr, Int32 *out_index);

KYRA_ENGINE_API ContainerResult     container_string_contains(const String string, ConstStr substr, Bool *out_result);
KYRA_ENGINE_API ContainerResult     container_string_contains_string(const String string, const String substr, Bool *out_result);

KYRA_ENGINE_API ContainerResult     container_string_begins_with(const String string, ConstStr prefix, Bool *out_result);
KYRA_ENGINE_API ContainerResult     container_string_begins_with_string(const String string, const String prefix, Bool *out_result);

KYRA_ENGINE_API ContainerResult     container_string_ends_with(const String string, ConstStr suffix, Bool *out_result);
KYRA_ENGINE_API ContainerResult     container_string_ends_with_string(const String string, const String suffix, Bool *out_result);

KYRA_ENGINE_API ContainerResult     container_string_to_cstr(const String string, ConstStr *out_cstr);
KYRA_ENGINE_API ContainerResult     container_string_size(const String string, ByteSize *out_size);
KYRA_ENGINE_API ContainerResult     container_string_capacity(const String string, ByteSize *out_capacity);
KYRA_ENGINE_API ContainerResult     container_string_is_empty(const String string, Bool *out_result);

KYRA_ENGINE_API ConstStr            container_string_get_cstr(const String string);
KYRA_ENGINE_API ByteSize            container_string_get_size(const String string);
KYRA_ENGINE_API ByteSize            container_string_get_capacity(const String string);
KYRA_ENGINE_API Bool                container_string_get_is_empty(const String string);

KYRA_ENGINE_API ConstStr            container_string_result_to_string(const ContainerResult result);


