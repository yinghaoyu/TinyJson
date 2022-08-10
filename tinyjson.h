#ifndef TINYJSON_H
#define TINYJSON_H

#include <stddef.h>  // size_t

// JSON has six type of data
// null, bool, number, string, array, object

typedef enum
{
  TINY_NULL,
  TINY_FALSE,
  TINY_TRUE,
  TINY_NUMBER,
  TINY_STRING,
  TINY_ARRAY,
  TINY_OBJECT
} tiny_type;

#define TINY_KEY_NOT_EXIST ((size_t) -1)

typedef struct tiny_value tiny_value;
typedef struct tiny_member tiny_member;

struct tiny_value
{
  union
  {
    struct
    {
      tiny_member *m;
      size_t size;
      size_t capacity;
    } o;  // object
    struct
    {
      tiny_value *e;
      size_t size;
      size_t capacity;
    } a;  // array
    struct
    {
      char *s;
      size_t len;
    } s;       // string
    double n;  // number
  } u;
  tiny_type type;
};

struct tiny_member
{
  char *k;       // key
  size_t klen;   // key len
  tiny_value v;  // value
};

enum
{
  TINY_PARSE_OK = 0,
  TINY_PARSE_EXPECT_VALUE,  // none charactor
  TINY_PARSE_INVALID_VALUE,
  TINY_PARSE_ROOT_NOT_SINGULAR,  // over than one charactor
  TINY_PARSE_NUMBER_TOO_BIG,
  TINY_PARSE_MISS_QUOTATION_MARK,    // miss match
  TINY_PARSE_INVALID_STRING_ESCAPE,  // escape code error
  TINY_PARSE_INVALID_STRING_CHAR,
  TINY_PARSE_INVALID_UNICODE_HEX,
  TINY_PARSE_INVALID_UNICODE_SURROGATE,
  TINY_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
  TINY_PARSE_MISS_KEY,
  TINY_PARSE_MISS_COLON,
  TINY_PARSE_MISS_COMMA_OR_CURLY_BRACKET,
};

#define tiny_init(v)       \
  do                       \
  {                        \
    (v)->type = TINY_NULL; \
  } while (0)

int tiny_parse(tiny_value *v, const char *json);
char *tiny_stringify(const tiny_value *v, size_t *length);

void tiny_free(tiny_value *v);

tiny_type tiny_get_type(const tiny_value *v);

#define tiny_set_null(v) tiny_free(v)

int tiny_get_boolean(const tiny_value *v);
void tiny_set_boolean(tiny_value *v, int b);

double tiny_get_number(const tiny_value *v);
void tiny_set_number(tiny_value *v, double n);

const char *tiny_get_string(const tiny_value *v);
size_t tiny_get_string_length(const tiny_value *v);
void tiny_set_string(tiny_value *v, const char *s, size_t len);

size_t tiny_get_array_size(const tiny_value *v);
tiny_value *tiny_get_array_element(const tiny_value *v, size_t index);

size_t tiny_get_object_size(const tiny_value *v);
const char *tiny_get_object_key(const tiny_value *v, size_t index);
size_t tiny_get_object_key_length(const tiny_value *v, size_t index);
tiny_value *tiny_get_object_value(const tiny_value *v, size_t index);
size_t tiny_find_object_index(const tiny_value *v, const char *key, size_t klen);
tiny_value *tiny_find_object_value(tiny_value *v, const char *key, size_t klen);
tiny_value *tiny_set_object_value(tiny_value *v, const char *key, size_t klen);
void tiny_remove_object_value(tiny_value *v, size_t index);
void tiny_erase_array_element(tiny_value *v, size_t index, size_t count);
int tiny_is_equal(const tiny_value *lhs, const tiny_value *rhs);
size_t tiny_get_array_capacity(const tiny_value *v);
void tiny_shrink_array(tiny_value *v);
void tiny_move(tiny_value *dst, tiny_value *src);
tiny_value *tiny_pushback_array_element(tiny_value *v);
void tiny_clear_array(tiny_value *v);
void tiny_copy(tiny_value *dst, const tiny_value *src);
void tiny_set_array(tiny_value *v, size_t capacity);
void tiny_swap(tiny_value *lhs, tiny_value *rhs);
void tiny_popback_array_element(tiny_value *v);

#endif
