#include <stdio.h>
#include <string.h>

#define M_WORD		1
#define M_SUBSTRING	2
#define M_BEGINNING	3
#define M_CASE		4
#define M_NOT		5
#define M_PARAMETER	6

int enum_matchers(const char *matcher, int *type, const char **param, int *s) {
	const char *p = matcher;
	const char *e;

	if (*p != ';')
		return 0;

	p++;

	if (strncmp(p, "w=\"", 3) == 0) {
		p += 3;
		for (e = p; *e && (*e != '\"' || e[-1] == '\\'); e++)
			continue;
		if (!*e)
			return -1;
		*param = p;
		*s = (int)(e-p);
		*type = M_WORD;
		p = e + 1;
	} else if (strncmp(p, "s=\"", 3) == 0) {
		p += 3;
		for (e = p; *e && (*e != '\"' || e[-1] == '\\'); e++)
			continue;
		if (!*e)
			return -1;
		*param = p;
		*s = (int)(e-p);
		*type = M_SUBSTRING;
		p = e + 1;
	} else if (strncmp(p, "b=\"", 3) == 0) {
		p += 3;
		for (e = p; *e && (*e != '\"' || e[-1] == '\\'); e++)
			continue;
		if (!*e)
			return -1;
		*param = p;
		*s = (int)(e-p);
		*type = M_BEGINNING;
		p = e + 1;
	} else if (strncmp(p, "p=\"", 3) == 0) {
		p += 3;
		for (e = p; *e && (*e != '\"' || e[-1] == '\\'); e++)
			continue;
		if (!*e)
			return -1;
		*param = p;
		*s = (int)(e-p);
		*type = M_PARAMETER;
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
					b++;
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

int cmp_func(const char *str1, const char *str2, int size, int case_sensitive) {
      if (case_sensitive == 1)
	    return strncmp(str1, str2, size);
      else
	    return strncasecmp(str1, str2, size);
}

int word_matcher(const char *p, const char *fv, int ps, int case_sensitive) {
	int read, size, offset = 0;
	const char *match;

	while (read = enum_fields(fv + offset, &match, &size)) {
		if (ps == size) {
			if (cmp_func(match, p, size, case_sensitive) == 0) {
				return 1;
			}
		}
		offset += read;
	}
	return 0;
}

int substring_matcher(const char *p, const char *fv, int ps, int case_sensitive) {
	int read, size, offset = 0;
	const char *match;

	while (read = enum_fields(fv + offset, &match, &size)) {
		if (strlen(p) <= size) {
			const char *s;
			for (s = match; s <= match + size - ps; s++) {
				if (cmp_func(s, p, ps, case_sensitive) == 0) {
					return 1;
				}
			}
		}
		offset += read;
	}
	return 0;
}

int beginning_substring_matcher(const char *p, const char *fv, int ps, int case_sensitive) {
	int read, size, offset = 0;
	const char *match;

	while (read = enum_fields(fv + offset, &match, &size)) {
		if (ps <= size) {
			if (cmp_func(match, p, ps, case_sensitive) == 0) {
				return 1;
			}
		}
		offset += read;
	}
	return 0;
}

int parameter_prefix_matcher(const char *p, const char *fv, int ps, int case_sensitive) {
	int read, size, offset = 0;
	const char *match;

	while (read = enum_fields(fv + offset, &match, &size)) {
		if (ps <= size) {
			if (cmp_func(match, p, ps, case_sensitive) == 0) {
				const char *r;
				for (r = match + ps; r < match + size && *r == ' '; r++)
					continue;
				if (*r == ';' || r == match + size)
					return 1;
			}
		}
		offset += read;
	}
	return 0;
}

int unescape(char *dst, const char *src, int len) {
	int i, j;
	i = j = 0;
	while (i < len) {
		switch (src[i]) {
			case '\\':
				switch(src[++i]) {
					case '\"':
						dst[j++] = '\"';
						i++;
						break;
					default:
						dst[j++] = '\\';
						dst[j] = src[i];
						break;
				}
				break;
			default:
				dst[j] = src[i];
				i++;
				j++;
				break;
		}
	}
	return j;
}

int matcher(const char *m, const char *f) {
    int mr, fr; // read sizes
    int mo = 0, fo = 0; // offsets
    int fs, ms; // sizes
    int type;
    const char *match, *field;
    int not_flag = 0;
    int case_flag = 0;

    while (mr = enum_matchers(m + mo, &type, &match, &ms)) {
	int matched = 1;
	char *m = malloc(ms+1);
	ms = unescape(m,match,ms);
	switch(type) {
	    case M_WORD:
		if (!(word_matcher(m, f, ms, case_flag) ^ not_flag))
		    matched = 0;
		break;
	    case M_SUBSTRING:
		if (!(substring_matcher(m, f, ms, case_flag) ^ not_flag))
		    matched = 0;
		break;
	    case M_BEGINNING:
		if (!(beginning_substring_matcher(m, f, ms, case_flag) ^ not_flag))
		    matched = 0;
		break;
	    case M_CASE:
		case_flag = 1;
		break;
	    case M_NOT:
		not_flag = 1;
		break;
	}
	free(m);
	if (!matched)
	    return 0;
	mo += mr;
    }
    return 1;
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
	    printf("\nERROR: Expected (%.*s) length %d == (%s) length %zu\n\n", size, match, size, expect[i], strlen(expect[i]));
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
	if (read < 0)
	    break;
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
	    case M_PARAMETER:
		printf(":parameter");
		break;
	    default:
		printf(":unknown");
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

int run_matcher_function_test(int (*functionPtr)(const char *, const char *, int, int), const char *p, const char *fv, int case_sensitive, int expect) {
    if (fancy)
	printf("[....] ");

    if (functionPtr == &word_matcher) {
	printf("(word_matcher): ");
    } else if (functionPtr == &substring_matcher) {
	printf("(substring_matcher): ");
    } else if (functionPtr == &beginning_substring_matcher) {
	printf("(beginning_substring_matcher): ");
    } else if (functionPtr == &parameter_prefix_matcher) {
	printf("(parameter_prefix_matcher): ");
    }

    printf("Testing that ");
    print_special("\"");
    print_literal(p);
    print_special("\"");
    printf(" %s ", expect == 1 ? "matches" : "doesn't match");
    print_special("\"");
    print_literal(fv);
    print_special("\"");
    printf(" %s", case_sensitive == 0 ? "" : "(CASE SENSITIVE)");
    if (fancy)
	printf("\r");
    else
	printf("\n");

    if (functionPtr(p, fv, strlen(p), case_sensitive) != expect) {
	printf("[\033[0;31mFAIL\033[00m]\n");
	return 1;
    } else {
	printf("[\033[0;32mPASS\033[00m]\n");
	return 0;
    }
}

char *escape(char * s, char * t) {
    int i, j;
    i = j = 0;
    
    while ( t[i] ) {
        
        /*  Translate the special character, if we have one  */
        
        switch( t[i] ) {
        case '\n':
            s[j++] = '\\';
            s[j] = 'n';
            break;
            
        case '\t':
            s[j++] = '\\';
            s[j] = 't';
            break;
            
        case '\a':
            s[j++] = '\\';
            s[j] = 'a';
            break;
            
        case '\b':
            s[j++] = '\\';
            s[j] = 'b';
            break;
            
        case '\f':
            s[j++] = '\\';
            s[j] = 'f';
            break;
            
        case '\r':
            s[j++] = '\\';
            s[j] = 'r';
            break;
            
        case '\v':
            s[j++] = '\\';
            s[j] = 'v';
            break;
            
        case '\\':
            s[j++] = '\\';
            s[j] = '\\';
            break;
            
        case '\"':
            s[j++] = '\\';
            s[j] = '\"';
            break;
            
        default:
            
            /*  This is not a special character, so just copy it  */
            
            s[j] = t[i];
            break;
        }
        ++i;
        ++j;
    }
    s[j] = t[i];    /*  Don't forget the null character  */
    return s;
}

void gen_varnish_test(char *m, char *v, int expect) {
    static testnum = 0;
    testnum++;
    char filename[256];
    char buf[256];

    sprintf(filename, "x%05d.vtc", testnum);
    FILE *fp = fopen(filename, "w+");
    if (fp == NULL) {
	printf("ERROR: Couldn't open file\n");
	return;
    }
    fprintf(fp, "varnishtest \"Test Key matcher functionality\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "server s1 {\n");
    fprintf(fp, "	rxreq\n");
    fprintf(fp, "	txresp -hdr \"Key: Foobar%s\"\n", escape(buf, m));
    fprintf(fp, "	rxreq\n");
    fprintf(fp, "	txresp -hdr \"Key: Foobar%s\"\n", escape(buf, m));
    fprintf(fp, "} -start\n");
    fprintf(fp, "\n");
    fprintf(fp, "varnish v1 -vcl+backend {} -start\n");
    fprintf(fp, "\n");
    fprintf(fp, "client c1 {\n");
    fprintf(fp, "	txreq -hdr \"Foobar: %s\"\n", escape(buf, v));
    fprintf(fp, "	rxresp\n");
    fprintf(fp, "	expect resp.status == 200\n");
    fprintf(fp, "	expect resp.http.X-Varnish == \"1001\"\n");
    fprintf(fp, "\n");
    fprintf(fp, "	txreq -hdr \"Foobar: %s\"\n", escape(buf, v));
    fprintf(fp, "	rxresp\n");
    fprintf(fp, "	expect resp.status == 200\n");
    if (expect == 1)
	fprintf(fp, "	expect resp.http.X-Varnish == \"1003 1002\"\n");
    else
	fprintf(fp, "	expect resp.http.X-Varnish == \"1003\"\n");
    fprintf(fp, "} -run\n");

    fclose(fp);
}

int run_test(char *m, char *v, int expect) {
    gen_varnish_test(m, v, expect);

    if (fancy)
	printf("[....] ");

    printf("Testing that ");
    print_special("\"");
    print_literal(m);
    print_special("\"");
    printf(" %s ", expect == 1 ? "matches" : "doesn't match");
    print_special("\"");
    print_literal(v);
    print_special("\"");
    if (fancy)
	printf("\r");
    else
	printf("\n");

    if (matcher(m, v) != expect) {
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

    printf("\n * ENUM MATCHERS *\n");
    result |= run_matcher_test(";w=\"foo\"", (int[]){M_WORD}, (const char*[]){"foo"}, 1);
    result |= run_matcher_test(";s=\"foo\"", (int[]){M_SUBSTRING}, (const char*[]){"foo"}, 1);
    result |= run_matcher_test(";b=\"foo\"", (int[]){M_BEGINNING}, (const char*[]){"foo"}, 1);
    result |= run_matcher_test(";c", (int[]){M_CASE}, (const char*[]){""}, 1);
    result |= run_matcher_test(";n", (int[]){M_NOT}, (const char*[]){""}, 1);
    result |= run_matcher_test(";p=\"foo\"", (int[]){M_PARAMETER}, (const char*[]){"foo"}, 1);
    result |= run_matcher_test(";c;w=\"foo\"", (int[]){M_CASE, M_WORD}, (const char*[]){"", "foo"}, 2);
    result |= run_matcher_test(";n;s=\"foo\"", (int[]){M_NOT, M_SUBSTRING}, (const char*[]){"", "foo"}, 2);
    result |= run_matcher_test(";w=\"foo=\\\"bar\\\"\"", (int[]){M_WORD}, (const char*[]){"foo=\\\"bar\\\""}, 1);
    result |= run_matcher_test(";n;c;w=\"a\";s=\"b\";b=\"c\"", (int[]){M_NOT, M_CASE, M_WORD, M_SUBSTRING, M_BEGINNING}, (const char*[]){"", "", "a", "b", "c"}, 5);

    result |= run_matcher_test(";w=\"a", (int[]){}, (const char*[]){}, 0);
    result |= run_matcher_test(";w=\"a\";", (int[]){M_WORD}, (const char*[]){"a"}, 1);
    result |= run_matcher_test(";", (int[]){}, (const char*[]){}, 0);

    printf("\n * ENUM FIELDS * \n");
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
    result |= run_field_test("\\\"a\\\"", (const char*[]){"\\\"a\\\""}, 1);

    printf("\n * MATCHER FUNCTIONS * \n");
    result |= run_matcher_function_test(&word_matcher, "foo", "foo", 0, 1);
    result |= run_matcher_function_test(&word_matcher, "foo", "Foo", 0, 1);
    result |= run_matcher_function_test(&word_matcher, "foo", "Foo", 1, 0);
    result |= run_matcher_function_test(&word_matcher, "foo", "foo,bar", 0, 1);
    result |= run_matcher_function_test(&word_matcher, "foo", "bar,foo,bar", 0, 1);
    result |= run_matcher_function_test(&word_matcher, "foo", "foobar", 0, 0);
    result |= run_matcher_function_test(&word_matcher, "\"foo,bar\"", "bar,\"foo,bar\",bar", 0, 1);

    result |= run_matcher_function_test(&substring_matcher, "foobar", "foobar", 0, 1);
    result |= run_matcher_function_test(&substring_matcher, "foo", "foobar", 0, 1);
    result |= run_matcher_function_test(&substring_matcher, "ooba", "foobar", 0, 1);
    result |= run_matcher_function_test(&substring_matcher, "bar", "foobar", 0, 1);

    result |= run_matcher_function_test(&beginning_substring_matcher, "foobar", "foobar", 0, 1);
    result |= run_matcher_function_test(&beginning_substring_matcher, "foo", "foobar", 0, 1);
    result |= run_matcher_function_test(&beginning_substring_matcher, "ooba", "foobar", 0, 0);
    result |= run_matcher_function_test(&beginning_substring_matcher, "bar", "foobar", 0, 0);

    result |= run_matcher_function_test(&parameter_prefix_matcher, "foobar", "foobar", 0, 1);
    result |= run_matcher_function_test(&parameter_prefix_matcher, "foobar", "foobar; true", 0, 1);
    result |= run_matcher_function_test(&parameter_prefix_matcher, "foobar", "foobar  ; true", 0, 1);
    result |= run_matcher_function_test(&parameter_prefix_matcher, "foo", "foobar; false", 0, 0);
    result |= run_matcher_function_test(&parameter_prefix_matcher, "ooba", "foobar; x=2", 0, 0);
    result |= run_matcher_function_test(&parameter_prefix_matcher, "bar", "foobar", 0, 0);

    printf("\n * MATCHERS * \n");
    result |= run_test(";w=\"a\"", "a", 1);
    result |= run_test(";b=\"a\"", "Apple", 1);
    result |= run_test(";b=\"a\"", "ardvark", 1);
    result |= run_test(";c;b=\"a\"", "Apple", 0);
    result |= run_test(";c;b=\"a\"", "ardvark", 1);
    result |= run_test(";w=\"a\";w=\"b\";w=\"c\"", "c,a,b", 1);
    result |= run_test(";w=\"a\"", "apple,a", 1);
    result |= run_test(";w=\"a\"", "b", 0);
    result |= run_test(";w=\"a\"", "a,b", 1);
    result |= run_test(";w=\"a\"", "b,a", 1);
    result |= run_test(";n;b=\"a\"", "car", 1);
    result |= run_test(";n;b=\"a\"", "duck", 1);
    result |= run_test(";c;w=\"a\"", "A", 0);
    result |= run_test(";n;w=\"a\"", "b", 1);
    result |= run_test(";w=\"a\";n;w=\"b\"", "a, c", 1);
    result |= run_test(";w=\"a\";n;w=\"b\"", "a, b", 0);
    result |= run_test(";w=\"\\\"a\\\"\"", "\"a\"", 1);
    result |= run_test(";w=\"\\\"a\\\"\"", "\"a\"", 1);
    result |= run_test(";w=\"\\a\"", "\\a", 1);
    result |= run_test(";p=\"text/html\"", "text/html", 1);
    result |= run_test(";p=\"text/html\"", "text/html; q=0.5", 1);
    result |= run_test(";p=\"text/html\"", "text/html;q=0.1", 1);
    result |= run_test(";p=\"text/html\"", "text/html; foo=\"bar\"", 1);

    result |= run_test(";w=\"foo\";p=\"test\";b=\"win\";c;s=\"iNnInG\";n;w=\"bar\"", "foo,test;bar,wiNnInG", 1);

    return result;
}
#endif
