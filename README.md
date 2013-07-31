# Simple C Lexer

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
