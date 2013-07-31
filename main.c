#include <stdio.h>
#include <string.h>

int enum_fields(const char *fv, const char **m, int *s) {
    const char *f, *b;
    enum state {
	start,
	skip_space,
	scan,
	scan_quote,
	scan_back,
	done,
	end
    } cur_state = start;

    do {
	switch (cur_state) {
	    case start:
		f = fv;
		cur_state = skip_space;
		break;
	    case skip_space:
		if (*f == '\0')
		    cur_state = end;
		else if (*f == ' ' || *f == ',')
		    f++;
		else if (*f == '"') {
		    b = f;
		    cur_state = scan_quote;
		} else {
		    b = f;
		    cur_state = scan;
		}
		break;
	    case scan:
		b++;
		if (*b == '\0')
		    cur_state = scan_back;
		else if (*b == '"')
		    cur_state = scan_quote;
		else if (*b == ',')
		    cur_state = scan_back;
		break;
	    case scan_quote:
		b++;
		if (*b == '\0')
		    cur_state = scan_back;
		else if (*b == '\\' && b[1] != '\0')
		    b += 2;
		else if (*b == '"')
		    cur_state = scan;
		break;
	    case scan_back:
		b--;
		if (*b != ' ')
		    cur_state = done;
		break;
	    case done:
		*m = f;
		*s = (int)(b-f)+1;
		return b - fv + 1;
		break;
	}
    } while (cur_state != end);

    return 0;
}

int run_enum_fields_test(const char *example, const char **expect, int count) {
    const char *match;
    int size;
    int read = 0;
    int offset = 0;
    int i = 0;

    while (read = enum_fields(example + offset, &match, &size)) {
	if (i >= count) {
	    printf("\nERROR: Too many matches\n\n");
	    return 1;
	} else if (strlen(expect[i]) != size) {
	    printf("\nERROR: Expected (%.*s) == (%s)\n\n", size, match, expect[i]);
	    return 1;
	} else if (memcmp(match, expect[i], size)) {
	    printf("\nERROR: Expected (%.*s) == (%s)\n\n", size, match, expect[i]);
	    return 1;
	}
	offset += read;
	i++;
    }

    if (i < count) {
	printf("\nERROR: Not enough matches. Expected %d more.\n\n", count - i);
	return 1;
    }

    return 0;
}

void print_special(const char *msg) {
    printf("\033[1;30m%s\033[00m", msg);
}

void print_literal(const char *msg) {
    printf("\033[1;31m%s\033[00m", msg);
}

int run_test(const char *example, const char **expect, int count) {
    printf("Testing (");
    print_special("\"");
    print_literal(example);
    print_special("\"");
    printf(") => [");

    int i;
    for (i = 0; i < count; i++) {
	if (i > 0)
	    printf(",");
	print_special("\"");
	print_literal(expect[i]);
	print_special("\"");
    }
    printf("]\n");

    if (run_enum_fields_test(example, expect, count)) {
	printf("[\033[0;31mFAIL\033[00m]\n");
	return 1;
    } else {
	printf("[\033[0;32mPASS\033[00m]\n");
	return 0;
    }
}

int main(int argc, char **argv) {
    int result = 0;

    result |= run_test("a", (const char*[]){"a"}, 1);
    result |= run_test(" a ", (const char*[]){"a"}, 1);

    result |= run_test("a,b", (const char*[]){"a","b"}, 2);
    result |= run_test(" a,b ", (const char*[]){"a","b"}, 2);
    result |= run_test(",a,b,", (const char*[]){"a","b"}, 2);
    result |= run_test(" ,a,b, ", (const char*[]){"a","b"}, 2);

    result |= run_test("hello world, it's me", (const char*[]){"hello world","it's me"}, 2);
    result |= run_test(" the quick brown fox ", (const char*[]){"the quick brown fox"}, 1);
    result |= run_test(" the,quick ,brown, fox ", (const char*[]){"the","quick","brown","fox"}, 4);
    result |= run_test("the,quick,brown,fox", (const char*[]){"the","quick","brown","fox"}, 4);
    result |= run_test("the , quick, brown ,fox", (const char*[]){"the","quick","brown","fox"}, 4);
    result |= run_test("the , , , quick, brown ,fox", (const char*[]){"the","quick","brown","fox"}, 4);
    result |= run_test("a,\"b,c\"", (const char*[]){"a","\"b,c\""}, 2);
    result |= run_test("a=\"b,c\"", (const char*[]){"a=\"b,c\""}, 1);
    result |= run_test("\"a", (const char*[]){"\"a"}, 1);
    result |= run_test("a\"", (const char*[]){"a\""}, 1);
    result |= run_test("a \"escape\\\"qu,ote\"", (const char*[]){"a \"escape\\\"qu,ote\""}, 1);
    result |= run_test("a \"escape\\", (const char*[]){"a \"escape\\"}, 1);

    return result;
}
