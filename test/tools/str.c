#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include "str.h"

#define ALPHABET_LEN 256

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// BAD-CHARACTER RULE.
// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurrence of c in pat.
//
// If c does not occur in pat, then delta1[c] = patlen.
// If c is at string[i] and c != pat[patlen-1], we can safely shift i
//   over by delta1[c], which is the minimum distance needed to shift
//   pat forward to get string[i] lined up with some character in pat.
// c == pat[patlen-1] returning zero is only a concern for BMH, which
//   does not have delta2. BMH makes the value patlen in such a case.
//   We follow this choice instead of the original 0 because it skips
//   more. (correctness?)
//
// This algorithm runs in alphabet_len+patlen time.
static void _make_delta1(ptrdiff_t* delta1, const uint8_t* pat, size_t patlen)
{
    for (size_t i = 0; i < ALPHABET_LEN; i++)
    {
        delta1[i] = patlen;
    }
    for (size_t i = 0; i < patlen; i++)
    {
        delta1[pat[i]] = patlen-1 - i;
    }
}

// true if the suffix of word starting from word[pos] is a prefix
// of word
static int _is_prefix(const uint8_t* word, size_t wordlen, ptrdiff_t pos)
{
    ptrdiff_t suffixlen = wordlen - pos;
    // could also use the strncmp() library function here
    // return ! strncmp(word, &word[pos], suffixlen);
    for (ptrdiff_t i = 0; i < suffixlen; i++)
    {
        if (word[i] != word[pos + i])
        {
            return 0;
        }
    }
    return 1;
}

// length of the longest suffix of word ending on word[pos].
// _suffix_length("dddbcabc", 8, 4) = 2
static size_t _suffix_length(const uint8_t* word, size_t wordlen, ptrdiff_t pos)
{
    ptrdiff_t i;
    // increment suffix length i to the first mismatch or beginning
    // of the word
    for (i = 0; (word[pos - i] == word[wordlen - 1 - i]) && (i <= pos); i++);
    return i;
}

// GOOD-SUFFIX RULE.
// delta2 table: given a mismatch at pat[pos], we want to align
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with
// pat[patlen-1].
//
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
//
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDBYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since _suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
static void _make_delta2(ptrdiff_t* delta2, const uint8_t* pat, size_t patlen)
{
    ptrdiff_t p;
    size_t last_prefix_index = 1;

    // first loop
    for (p = patlen - 1; p >= 0; p--)
    {
        if (_is_prefix(pat, patlen, p + 1))
        {
            last_prefix_index = p + 1;
        }
        delta2[p] = last_prefix_index + (patlen - 1 - p);
    }

    // second loop
    for (p = 0; p < (ptrdiff_t)patlen - 1; p++)
    {
        size_t slen = _suffix_length(pat, patlen, p);
        if (pat[p - slen] != pat[patlen - 1 - slen])
        {
            delta2[patlen - 1 - slen] = patlen - 1 - p + slen;
        }
    }
}

// Returns pointer to first match.
// See also glibc memmem() (non-BM) and std::boyer_moore_searcher (first-match).
static const uint8_t* _boyer_moore(const uint8_t* string, size_t stringlen,
    const uint8_t* pat, size_t patlen)
{
    ptrdiff_t delta1[ALPHABET_LEN];
    ptrdiff_t* delta2 = malloc(sizeof(ptrdiff_t) * patlen);
    assert(delta2 != NULL);

    _make_delta1(delta1, pat, patlen);
    _make_delta2(delta2, pat, patlen);

    const uint8_t* ret = NULL;
    // The empty pattern must be considered specially
    if (patlen == 0)
    {
        ret = string;
        goto finish;
    }

    size_t i = patlen - 1;        // str-idx
    while (i < stringlen)
    {
        ptrdiff_t j = patlen - 1; // pat-idx
        while (j >= 0 && (string[i] == pat[j]))
        {
            --i;
            --j;
        }
        if (j < 0)
        {
            ret = &string[i+1];
            goto finish;
        }

        ptrdiff_t shift = max(delta1[string[i]], delta2[j]);
        i += shift;
    }

finish:
    free(delta2);
    return ret;
}

static void _vfs_str_ensure_capacity(vfs_str_t* str, size_t n)
{
    if (str->cap > n)
    {
        return;
    }

    vfs_str_ensure_dynamic(str);

    str->cap = n + 1;
    char* newstr = realloc(str->str, str->cap);
    if (newstr == NULL)
    {
        abort();
    }
    str->str = newstr;
}

void vfs_str_ensure_dynamic(vfs_str_t* str)
{
    if (str->cap != 0 || str->str == NULL)
    {
        return;
    }

    str->cap = str->len + 1;
    char* new_copy = malloc(str->cap);
    if (new_copy == NULL)
    {
        abort();
    }
    memcpy(new_copy, str->str, str->len);
    str->str = new_copy;
    str->str[str->len] = '\0';
}

void vfs_str_insert(vfs_str_t* str, size_t pos, const char* data, size_t size)
{
    if (pos > str->len)
    {
        abort();
    }
    if (pos == str->len)
    {
        vfs_str_append(str, data, size);
        return;
    }

    const size_t required_sz = str->len + size;
    _vfs_str_ensure_capacity(str, required_sz + 1);

    size_t move_sz = str->len - pos;
    memmove(str->str + pos + size, str->str + pos, move_sz);
    memcpy(str->str + pos, data, size);
    str->len = required_sz;
    str->str[str->len] = '\0';
}

vfs_str_t vfs_str_from_static(const char* data, size_t size)
{
    vfs_str_t str = { (char*)data, size, 0 };
    return str;
}

vfs_str_t vfs_str_from_static1(const char* data)
{
    size_t size = strlen(data);
    return vfs_str_from_static(data, size);
}

vfs_str_t vfs_str_from(const char* data, size_t size)
{
    vfs_str_t str = VFS_STR_INIT;
    vfs_str_append(&str, data, size);
    return str;
}

vfs_str_t vfs_str_from1(const char* data)
{
    if (data == NULL)
    {
        return (vfs_str_t)VFS_STR_INIT;
    }

    size_t size = strlen(data);
    return vfs_str_from(data, size);
}

void vfs_str_reset(vfs_str_t* str)
{
    if (str->cap != 0)
    {
        str->str[0] = '\0';
        str->len = 0;
        return;
    }

    str->str = NULL;
    str->len = 0;
}

void vfs_str_exit(vfs_str_t* str)
{
    if (str->cap != 0)
    {
        free(str->str);
    }
    str->str = NULL;
    str->len = 0;
    str->cap = 0;
}

void vfs_str_append(vfs_str_t* str, const char* data, size_t size)
{
    size_t required_sz = str->len + size;
    if (str->cap >= required_sz + 1)
    {
        goto do_copy;
    }

    /* Append must happen on dynamic allocated string. */
    vfs_str_ensure_dynamic(str);

    char* new_ptr = realloc(str->str, required_sz + 1);
    if (new_ptr == NULL)
    {
        abort();
    }
    str->str = new_ptr;
    str->cap = required_sz + 1;

do_copy:
    memcpy(str->str + str->len, data, size);
    str->len = required_sz;
    str->str[str->len] = '\0';
}

void vfs_str_append1(vfs_str_t* str, const char* data)
{
    size_t data_sz = strlen(data);
    vfs_str_append(str, data, data_sz);
}

void vfs_str_append2(vfs_str_t* str, const vfs_str_t* data)
{
    vfs_str_append(str, data->str, data->len);
}

void vfs_str_append3(vfs_str_t* str, char c)
{
    char buf[2] = { c, '\0' };
    vfs_str_append(str, buf, 1);
}

vfs_str_t vfs_str_dup(const vfs_str_t* str)
{
    vfs_str_t copy = VFS_STR_INIT;

    /* Always deep copy. */
    copy.len = str->len;
    copy.cap = copy.len + 1;
    if ((copy.str = malloc(copy.cap)) == NULL)
    {
        abort();
    }
    memcpy(copy.str, str->str, copy.len);
    copy.str[copy.len] = '\0';

    return copy;
}

vfs_str_t vfs_str_sub(const vfs_str_t* str, size_t offset, size_t size)
{
    vfs_str_t newstr = VFS_STR_INIT;
    if (offset >= str->len)
    {
        return newstr;
    }

    size_t max_size = str->len - offset;
    newstr.len = min(max_size, size);
    newstr.cap = newstr.len + 1;
    if ((newstr.str = malloc(newstr.cap)) == NULL)
    {
        abort();
    }
    memcpy(newstr.str, str->str + offset, newstr.len);
    newstr.str[newstr.len] = '\0';

    return newstr;
}

vfs_str_t vfs_str_sub1(const vfs_str_t* str, size_t offset)
{
    return vfs_str_sub(str, offset, SIZE_MAX);
}

void vfs_str_chop(vfs_str_t* str, size_t n)
{
    if (str->len <= n)
    {
        vfs_str_reset(str);
        return;
    }

    vfs_str_ensure_dynamic(str);

    str->len -= n;
    str->str[str->len] = '\0';
}

void vfs_str_resize(vfs_str_t* str, size_t n)
{
    _vfs_str_ensure_capacity(str, n);

    if (n < str->len)
    {
        str->len = n;
        str->str[str->len] = '\0';
    }
}

int vfs_str_startwith(const vfs_str_t* str, const char* data, size_t size)
{
    if (size > str->len)
    {
        return 0;
    }

    if (str->str == NULL && size != 0)
    {
        return 0;
    }

    return !memcmp(str->str, data, size);
}

int vfs_str_startwith1(const vfs_str_t* str, const char* data)
{
    size_t size = strlen(data);
    return vfs_str_startwith(str, data, size);
}

int vfs_str_startwith2(const vfs_str_t* str, const vfs_str_t* data)
{
    return vfs_str_startwith(str, data->str, data->len);
}

int vfs_str_endwith(const vfs_str_t* str, const char* data, size_t size)
{
    if (size > str->len)
    {
        return 0;
    }

    if (str->str == NULL && size != 0)
    {
        return 0;
    }

    size_t offset = str->len - size;
    return !memcmp(str->str + offset, data, size);
}

int vfs_str_endwith1(const vfs_str_t* str, const char* data)
{
    size_t size = strlen(data);
    return vfs_str_endwith(str, data, size);
}

ptrdiff_t vfs_str_search(const vfs_str_t* str, size_t start, size_t len,
    const char* data, size_t size)
{
    if (start + len > str->len || len == 0)
    {
        return -EINVAL;
    }
    const uint8_t* start_pos = (uint8_t*)str->str + start;

    const uint8_t* addr = _boyer_moore(start_pos, len, (uint8_t*)data, size);
    if (addr == NULL)
    {
        return -1;
    }

    ptrdiff_t offset = addr - (uint8_t*)str->str;
    return offset;
}

ptrdiff_t vfs_str_search1(const vfs_str_t* str, size_t start,
    const char* data, size_t size)
{
    if (start > str->len)
    {
        return -EINVAL;
    }

    size_t len = str->len - start;
    return vfs_str_search(str, start, len, data, size);
}

ptrdiff_t vfs_str_search2(const vfs_str_t* str, const char* data, size_t size)
{
    return vfs_str_search1(str, 0, data, size);
}

ptrdiff_t vfs_str_search3(const vfs_str_t* str, const char* data)
{
    size_t size = strlen(data);
    return vfs_str_search2(str, data, size);
}

ptrdiff_t vfs_str_search4(const vfs_str_t* str, size_t start, const char* data)
{
    size_t size = strlen(data);
    return vfs_str_search1(str, start, data, size);
}

int vfs_str_cmp(const vfs_str_t* str, const char* data, size_t size)
{
    int ret;
    size_t cmp_size = min(str->len, size);

    if ((ret = memcmp(str->str, data, cmp_size)) != 0)
    {
        return ret;
    }

    if (str->len == size)
    {
        return 0;
    }
    return str->len < size ? -1 : 1;
}

int vfs_str_cmp1(const vfs_str_t* str, const char* data)
{
    size_t size = strlen(data);
    return vfs_str_cmp(str, data, size);
}

int vfs_str_cmp2(const vfs_str_t* s1, const vfs_str_t* s2)
{
    return vfs_str_cmp(s1, s2->str, s2->len);
}

void vfs_str_remove_trailing(vfs_str_t* str, char c)
{
    if (str->str == NULL)
    {
        return;
    }

    vfs_str_ensure_dynamic(str);

    while (str->len > 0 && str->str[str->len - 1] == c)
    {
        str->len--;
        str->str[str->len] = '\0';
    }
}

void vfs_str_remove_leading(vfs_str_t* str, char c)
{
    if (str->str == NULL)
    {
        return;
    }

    vfs_str_ensure_dynamic(str);

    size_t pos = 0;
    for (; pos < str->len; pos++)
    {
        if (str->str[pos] != c)
        {
            break;
        }
    }
    if (pos >= str->len)
    {
        vfs_str_reset(str);
        return;
    }
    if (pos == 0)
    {
        return;
    }

    size_t left_sz = str->len - pos;
    memmove(str->str, str->str + pos, left_sz);
    str->len = left_sz;
    str->str[str->len] = '\0';
}
