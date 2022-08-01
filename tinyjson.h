#ifndef TINYJSON_H
#define TINYJSON_H

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

typedef struct
{
  double n;
  tiny_type type;
} tiny_value;

enum
{
  TINY_PARSE_OK = 0,
  TINY_PARSE_EXPECT_VALUE,  // none charactor
  TINY_PARSE_INVALID_VALUE,
  TINY_PARSE_ROOT_NOT_SINGULAR,  // over than one charactor
  TINY_PARSE_NUMBER_TOO_BIG
};

int tiny_parse(tiny_value *v, const char *json);

tiny_type tiny_get_type(const tiny_value *v);

double tiny_get_number(const tiny_value *v);

#endif
