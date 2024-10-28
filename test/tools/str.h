#ifndef __VFS_STRING_H__
#define __VFS_STRING_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct vfs_str
{
    char*       str;    /**< C string content. terminal '\0' is always append unless it is NULL. */
    size_t      len;    /**< C string length in bytes, not including terminal '\0'. */
    size_t      cap;    /**< The total buffer length in bytes. */
} vfs_str_t;

/**
 * @brief Static initializer for #vfs_str_t.
 * Any instance of #vfs_str_t must always initialized to it.
 */
#define VFS_STR_INIT            { NULL, 0, 0 }

/**
 * @brief Check if \p s is a static string.
 * @param[in] s - String to check.
 * @return true if \p s  is a static string.
 */
#define VFS_STR_IS_STATIC(s)    ((s)->cap == 0)

/**
 * @brief Check if \p s is empty.
 * @warning Even \p s is empty, it is still need to call #vfs_str_exit() to
 *   free it.
 * @param[in] s - String to check.
 * @return true if \p s is empty.
 */
#define VFS_STR_IS_EMPTY(s)     ((s)->len == 0)

/**
 * @brief Create a new string object with \p data and \p size.
 *
 * The \p data is not deep copied, so it is very cheap.
 *
 * @warning The \p size must be equal to strlen(\p data).
 * @warning It is dangerous to use this function if \p data is not a static string.
 *   If \p data is not a static string, you either ensure the returned string
 *   object's lifetime is longer than \p data, or use #vfs_str_from() instead.
 * @param[in] data - Data.
 * @param[in] size - Size in bytes.
 * @return New string object.
 */
vfs_str_t vfs_str_from_static(const char* data, size_t size);

/**
 * @brief Create a new string object with \p data.
 *
 * The \p data is not deep copied, so it is very cheap.
 *
 * @see #vfs_str_from_static().
 * @param[in] data - Data.
 * @return New string object.
 */
vfs_str_t vfs_str_from_static1(const char* data);

/**
 * @brief Create a new string object with \p data and \p size.
 * @param[in] data - Data.
 * @param[in] size - Size in bytes.
 * @return New string object.
 */
vfs_str_t vfs_str_from(const char* data, size_t size);

/**
 * @brief Create a new string object with \p data.
 * @param[in] data - Data.
 * @return New string object.
 */
vfs_str_t vfs_str_from1(const char* data);

/**
 * @brief Create a copy of the string.
 * @param[in] str - String object.
 * @return Copy of the string.
 */
vfs_str_t vfs_str_dup(const vfs_str_t* str);

/**
 * @brief Create a substring with starting \p offset and \p size.
 * @param[in] str - String object.
 * @param[in] offset - Offset in bytes.
 * @param[in] size - Size in bytes.
 * @return New string object.
 */
vfs_str_t vfs_str_sub(const vfs_str_t* str, size_t offset, size_t size);

/**
 * @brief Create a substring with starting \p offset to the end of string.
 * @param[in] str - String object.
 * @param[in] offset - Offset in bytes.
 * @return New string object.
 */
vfs_str_t vfs_str_sub1(const vfs_str_t* str, size_t offset);

/**
 * @brief Reset content to empty.
 * @param[in,out] str - String object.
 */
void vfs_str_reset(vfs_str_t* str);

/**
 * @brief Destroy the string object.
 * @param[in,out] str - String object.
 */
void vfs_str_exit(vfs_str_t* str);

/**
 * @brief Ensure that the string is dynamic allocated.
 *
 * After this function, #VFS_STR_IS_STATIC() will return false if no empty.
 *
 * @param[in,out] str - String object.
 */
void vfs_str_ensure_dynamic(vfs_str_t* str);

/**
 * @brief Insert data at \p pos.
 * @param[in,out] str - String object.
 * @param[in] pos - Position in bytes.
 * @param[in] data - Data to insert.
 * @param[in] size - Data size in bytes.
 */
void vfs_str_insert(vfs_str_t* str, size_t pos, const char* data, size_t size);

/**
 * @brief Append data to the string.
 * @param[in,out] str - String object.
 * @param[in] data - Data to append.
 * @param[in] size - Data size in bytes.
 */
void vfs_str_append(vfs_str_t* str, const char* data, size_t size);

/**
 * @brief Append data to the string.
 * @param[in,out] str - String object.
 * @param[in] data - Data to append.
 */
void vfs_str_append1(vfs_str_t* str, const char* data);

/**
 * @brief Append data to the string.
 * @param[in,out] str - String object.
 * @param[in] data - Data to append.
 */
void vfs_str_append2(vfs_str_t* str, const vfs_str_t* data);

void vfs_str_append3(vfs_str_t* str, char c);

/**
 * @brief Removes \p n characters from the end of the string.
 * @param[in,out] str - String object.
 * @param[in] n - Number of characters to remove.
 */
void vfs_str_chop(vfs_str_t* str, size_t n);

/**
 * @brief Resizes the string to \p n bytes.
 *
 * If \p n is larger than the current size, the string is not modified, but the
 * capacity is increased.
 *
 * If \p n is smaller than the current size, the string is truncated. The terminal
 * null byte is always present.
 *
 * @param[in] str - String object.
 * @param[in] n - New size in bytes.
 */
void vfs_str_resize(vfs_str_t* str, size_t n);

/**
 * @brief Returns true if \p str starts with \p data with \p size bytes.
 * @param[in] str - String object.
 * @param[in] data - Data to compare.
 * @param[in] size - Data size in bytes.
 * @return true if \p str starts with \p data with \p size bytes.
 */
int vfs_str_startwith(const vfs_str_t* str, const char* data, size_t size);

/**
 * @brief Returns true if \p str starts with \p data.
 * @param[in] str - String object.
 * @param[in] data - Data to compare.
 * @return true if \p str starts with \p data.
 */
int vfs_str_startwith1(const vfs_str_t* str, const char* data);

/**
 * @brief Returns true if \p str starts with \p data.
 * @param[in] str - String object.
 * @param[in] data - Data to compare.
 * @return true if \p str starts with \p data.
 */
int vfs_str_startwith2(const vfs_str_t* str, const vfs_str_t* data);

/**
 * @brief Returns true if \p str ends with \p data with \p size bytes.
 * @param[in] str - String object.
 * @param[in] data - Data to compare.
 * @param[in] size - Data size in bytes.
 * @return true if \p str ends with \p data with \p size bytes.
 */
int vfs_str_endwith(const vfs_str_t* str, const char* data, size_t size);

/**
 * @brief Returns true if \p str ends with \p data.
 * @param[in] str - String object.
 * @param[in] data - Data to compare.
 * @return true if \p str ends with \p data.
 */
int vfs_str_endwith1(const vfs_str_t* str, const char* data);

/**
 * @brief Search for \p data in the string.
 * @param[in] str - String object.
 * @param[in] start - Start index.
 * @param[in] len - Max length in bytes to search.
 * @param[in] data - Data to search for.
 * @param[in] size - Data size in bytes.
 * @return Index of the first occurrence of \p data in the string, or -1 if not
 *   found.
 */
ptrdiff_t vfs_str_search(const vfs_str_t* str, size_t start, size_t len,
    const char* data, size_t size);

/**
 * @brief Search for \p data in the string.
 * @param[in] str - String object.
 * @param[in] start - Start index.
 * @param[in] data - Data to search for.
 * @param[in] size - Data size in bytes.
 * @return Index of the first occurrence of \p data in the string, or -1 if not
 *   found.
 */
ptrdiff_t vfs_str_search1(const vfs_str_t* str, size_t start,
	const char* data, size_t size);

/**
 * @brief Search for \p data in the string.
 * @param[in] str - String object.
 * @param[in] data - Data to search for.
 * @param[in] size - Data size in bytes.
 * @return Index of the first occurrence of \p data in the string, or -1 if not
 *   found.
 */
ptrdiff_t vfs_str_search2(const vfs_str_t* str, const char* data, size_t size);

/**
 * @brief Search for \p data in the string.
 * @param[in] str - String object.
 * @param[in] data - Data to search for.
 * @return Index of the first occurrence of \p data in the string, or -1 if not
 *   found.
 */
ptrdiff_t vfs_str_search3(const vfs_str_t* str, const char* data);

/**
 * @brief Search for \p data in the string.
 * @param[in] str - String object.
 * @param[in] start - Start index.
 * @param[in] data - Data to search for.
 * @return Index of the first occurrence of \p data in the string, or -1 if not
 *   found.
 */
ptrdiff_t vfs_str_search4(const vfs_str_t* str, size_t start, const char* data);

/**
 * @brief Compare \p str and \p data with \p size bytes.
 * @param[in] str - String object.
 * @param[in] data - Data to compare.
 * @param[in] size - Data size in bytes.
 * @return 0 if \p str and \p data are equal.
 */
int vfs_str_cmp(const vfs_str_t* str, const char* data, size_t size);

/**
 * @brief Compare \p str and \p data with \p size bytes.
 * @param[in] str - String object.
 * @param[in] data - Data to compare.
 * @return 0 if \p str and \p data are equal.
 */
int vfs_str_cmp1(const vfs_str_t* str, const char* data);

/**
 * @brief Compare \p s1 and \p s2.
 * @param[in] s1 - String object.
 * @param[in] s2 - String object.
 * @return 0 if \p s1 and \p s2 are equal.
 */
int vfs_str_cmp2(const vfs_str_t* s1, const vfs_str_t* s2);

/**
 * @brief Remove trailing \p c from the string.
 * @param[in,out] str - String object.
 * @param[in] c - Character to remove.
 */
void vfs_str_remove_trailing(vfs_str_t* str, char c);

/**
 * @brief Remove leading \p c from the string.
 * @param[in,out] str - String object.
 * @param[in] c - Character to remove.
 */
void vfs_str_remove_leading(vfs_str_t* str, char c);

#ifdef __cplusplus
}
#endif
#endif
