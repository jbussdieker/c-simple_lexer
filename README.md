# Simple C Lexer

[![Build Status](https://travis-ci.org/jbussdieker/c-simple_lexer.png?branch=master)](https://travis-ci.org/jbussdieker/c-simple_lexer)

Reads a comma separated fields from an input a returns each element.

# Usage

```c
int read, size, offset = 0;
const char *example = "one,two,three";
const char *match;
    
while (read = enum_fields(example + offset, &match, &size)) {
    printf("Found a token (%.*s)\n", size, match);
    offset += read;
}
```

## enum_fields

```c
int enum_fields(const char *fv, const char **m, int *s)
```

Find the next field value within a field value list.

### Returns

* 0 for no match
* > 0 to indicate the offset the match was found at

### Parameters

* fv: String representing the field values.
* m: (Output) String to hold the start of the field value that was found.
* s: (Output) The size of the field that was found.
