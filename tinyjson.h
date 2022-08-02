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
    } o;  // object
    struct
    {
      tiny_value *e;
      size_t size;
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

#endif
