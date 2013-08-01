#include <stdio.h>
#include <string.h>

#define M_WORD		1
#define M_SUBSTRING	2
#define M_BEGINNING	3
#define M_CASE		4
#define M_NOT		5

int enum_matchers(const char *matcher, int *type, const char **param, int *s) {
	const char *p = matcher;
	const char *e;

	if (*p != ';')
		return 0;

	p++;

	if (strncmp(p, "w=\"", 3) == 0) {
		p += 3;
		for (e = p; *e != '\"' || e[-1] == '\\'; e++)
			continue;
		*param = p;
		*s = (int)(e-p);
		*type = M_WORD;
		p = e + 1;
	} else if (strncmp(p, "s=\"", 3) == 0) {
		p += 3;
		for (e = p; *e != '\"' || e[-1] == '\\'; e++)
			continue;
		*param = p;
		*s = (int)(e-p);
		*type = M_SUBSTRING;
		p = e + 1;
	} else if (strncmp(p, "b=\"", 3) == 0) {
		p += 3;
		for (e = p; *e != '\"' || e[-1] == '\\'; e++)
			continue;
		*param = p;
		*s = (int)(e-p);
		*type = M_BEGINNING;
		p = e + 1;
	} else if (*p == 'c') {
		*s = 0;
		*type = M_CASE;
		p++;
	} else if (*p == 'n') {
		*s = 0;
		*type = M_NOT;
		p++;
	} else {
		// Invalid matcher
		return -1;
	}
	return p - matcher;
}

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

#ifdef TESTS
int fancy = 0;

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
	printf("\nERROR: Not enough matches. Expected %d but got %d.\n\n", count, i);
	return 1;
    }

    return 0;
}

int run_enum_matchers_test(const char *example, int *expect_type, const char **expect_param, int count) {
    const char *match;
    int size;
    int type;
    int read = 0;
    int offset = 0;
    int i = 0;

    while (read = enum_matchers(example + offset, &type, &match, &size)) {
	if (i >= count) {
	    printf("\nERROR: Too many matches\n\n");
	    return 1;
	} else if (expect_type[i] != type) {
	    printf("\nERROR: Expected type (%d) == (%d)\n\n", type, expect_type[i]);
	    return 1;
	} else if (strlen(expect_param[i]) != size) {
	    printf("\nERROR: Expected (%.*s) == (%s)\n\n", size, match, expect_param[i]);
	    return 1;
	} else if (memcmp(match, expect_param[i], size)) {
	    printf("\nERROR: Expected (%.*s) == (%s)\n\n", size, match, expect_param[i]);
	    return 1;
	}
	offset += read;
	i++;
    }

    if (i < count) {
	printf("\nERROR: Not enough matches. Expected %d but got %d.\n\n", count, i);
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

int run_matcher_test(const char *example, int *expect_type, const char **expect_param, int count) {
    if (fancy)
	printf("[....] ");

    printf("Testing (");
    print_special("\"");
    print_literal(example);
    print_special("\"");
    printf(") => [");

    int i;
    for (i = 0; i < count; i++) {
	if (i > 0)
	    printf(", ");
	switch (expect_type[i]) {
	    case M_WORD:
		printf(":word");
		break;
	    case M_SUBSTRING:
		printf(":substring");
		break;
	    case M_BEGINNING:
		printf(":beginning");
		break;
	    case M_CASE:
		printf(":case");
		break;
	    case M_NOT:
		printf(":not");
		break;
	}
        printf(" => ");
	print_special("\"");
	print_literal(expect_param[i]);
	print_special("\"");
    }

    printf("]");
    if (fancy)
	printf("\r");
    else
	printf("\n");

    if (run_enum_matchers_test(example, expect_type, expect_param, count)) {
	printf("[\033[0;31mFAIL\033[00m]\n");
	return 1;
    } else {
	printf("[\033[0;32mPASS\033[00m]\n");
	return 0;
    }
}

int run_field_test(const char *example, const char **expect, int count) {
    if (fancy)
	printf("[....] ");

    printf("Testing (");
    print_special("\"");
    print_literal(example);
    print_special("\"");
    printf(") => [");

    int i;
    for (i = 0; i < count; i++) {
	if (i > 0)
	    printf(", ");
	print_special("\"");
	print_literal(expect[i]);
	print_special("\"");
    }

    printf("]");
    if (fancy)
	printf("\r");
    else
	printf("\n");

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

    fancy = argc == 2 ? 0 : 1;

    printf("\n * MATCHER TESTS *\n");
    result |= run_matcher_test(";w=\"foo\"", (int[]){M_WORD}, (const char*[]){"foo"}, 1);
    result |= run_matcher_test(";s=\"foo\"", (int[]){M_SUBSTRING}, (const char*[]){"foo"}, 1);
    result |= run_matcher_test(";b=\"foo\"", (int[]){M_BEGINNING}, (const char*[]){"foo"}, 1);
    result |= run_matcher_test(";c", (int[]){M_CASE}, (const char*[]){""}, 1);
    result |= run_matcher_test(";n", (int[]){M_NOT}, (const char*[]){""}, 1);

    result |= run_matcher_test(";c;w=\"foo\"", (int[]){M_CASE, M_WORD}, (const char*[]){"", "foo"}, 2);
    result |= run_matcher_test(";n;s=\"foo\"", (int[]){M_NOT, M_SUBSTRING}, (const char*[]){"", "foo"}, 2);

    result |= run_matcher_test(";w=\"foo=\\\"bar\\\"\"", (int[]){M_WORD}, (const char*[]){"foo=\\\"bar\\\""}, 1);

    result |= run_matcher_test(";n;c;w=\"a\";s=\"b\";b=\"c\"", (int[]){M_NOT, M_CASE, M_WORD, M_SUBSTRING, M_BEGINNING}, (const char*[]){"", "", "a", "b", "c"}, 5);

    printf("\n * FIELD TESTS * \n");
    result |= run_field_test("a", (const char*[]){"a"}, 1);
    result |= run_field_test(" a ", (const char*[]){"a"}, 1);

    result |= run_field_test("a,b", (const char*[]){"a","b"}, 2);
    result |= run_field_test(" a,b ", (const char*[]){"a","b"}, 2);
    result |= run_field_test(",a,b,", (const char*[]){"a","b"}, 2);
    result |= run_field_test(" ,a,b, ", (const char*[]){"a","b"}, 2);

    result |= run_field_test("hello world, it's me", (const char*[]){"hello world","it's me"}, 2);
    result |= run_field_test(" the quick brown fox ", (const char*[]){"the quick brown fox"}, 1);
    result |= run_field_test(" the,quick ,brown, fox ", (const char*[]){"the","quick","brown","fox"}, 4);
    result |= run_field_test("the,quick,brown,fox", (const char*[]){"the","quick","brown","fox"}, 4);
    result |= run_field_test("the , quick, brown ,fox", (const char*[]){"the","quick","brown","fox"}, 4);
    result |= run_field_test("the , , , quick, brown ,fox", (const char*[]){"the","quick","brown","fox"}, 4);
    result |= run_field_test("a,\"b,c\"", (const char*[]){"a","\"b,c\""}, 2);
    result |= run_field_test("a=\"b,c\"", (const char*[]){"a=\"b,c\""}, 1);
    result |= run_field_test("\"a", (const char*[]){"\"a"}, 1);
    result |= run_field_test("a\"", (const char*[]){"a\""}, 1);
    result |= run_field_test("a \"escape\\\"qu,ote\"", (const char*[]){"a \"escape\\\"qu,ote\""}, 1);
    result |= run_field_test("a \"escape\\", (const char*[]){"a \"escape\\"}, 1);

    result |= run_field_test("1, 2, a=\"b,c\"", (const char*[]){"1","2","a=\"b,c\""}, 3);

    return result;
}
#endif
