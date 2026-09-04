#include "i18n.h"

#include <stdio.h>
#include <string.h>

#define I18N_MAX_FILE 32768
#define I18N_MAX_ENTRIES 320
#define I18N_KEY_MAX 80

typedef struct { char key[I18N_KEY_MAX]; char *value; } I18nEntry;
static char i18n_data[I18N_MAX_FILE];
static I18nEntry i18n_entries[I18N_MAX_ENTRIES];
static int i18n_entry_count;
static char i18n_language[16] = "en_US";

static char *skip_ws(char *p) {
	while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
	return p;
}

/* Decode a JSON string in place. Raw UTF-8 keeps packs editable without a
 * third-party parser or an extra rootfs dependency. */
static char *json_string(char **cursor) {
	char *p = skip_ws(*cursor), *out, *start;
	if (*p != '"') return NULL;
	p++; out = start = p;
	while (*p && *p != '"') {
		if (*p == '\\') {
			p++;
			if (!*p) return NULL;
			switch (*p) {
			case 'n': *out++ = '\n'; break;
			case 'r': *out++ = '\r'; break;
			case 't': *out++ = '\t'; break;
			case '"': *out++ = '"'; break;
			case '\\': *out++ = '\\'; break;
			case '/': *out++ = '/'; break;
			default: return NULL;
			}
			p++;
		} else *out++ = *p++;
	}
	if (*p != '"') return NULL;
	*out = '\0';
	*cursor = p + 1;
	return start;
}

static int load_pack(const char *path) {
	FILE *f = fopen(path, "rb");
	if (!f) return 0;
	size_t n = fread(i18n_data, 1, sizeof(i18n_data) - 1, f);
	fclose(f);
	if (!n || n == sizeof(i18n_data) - 1) return 0;
	i18n_data[n] = '\0';
	char *p = skip_ws(i18n_data);
	if (*p++ != '{') return 0;
	i18n_entry_count = 0;
	for (;;) {
		p = skip_ws(p);
		if (*p == '}') return i18n_entry_count > 0;
		if (i18n_entry_count >= I18N_MAX_ENTRIES) return 0;
		char *key = json_string(&p);
		if (!key) return 0;
		p = skip_ws(p);
		if (*p++ != ':') return 0;
		char *value = json_string(&p);
		if (!value || strlen(key) >= I18N_KEY_MAX) return 0;
		strcpy(i18n_entries[i18n_entry_count].key, key);
		i18n_entries[i18n_entry_count++].value = value;
		p = skip_ws(p);
		if (*p == ',') { p++; continue; }
		if (*p == '}') return 1;
		return 0;
	}
}

int i18n_init(const char *language) {
	char path[160];
	if (!language || !*language) language = "en_US";
	strncpy(i18n_language, language, sizeof(i18n_language) - 1);
	i18n_language[sizeof(i18n_language) - 1] = '\0';
	snprintf(path, sizeof(path), "/mnt/sdcard/frogui/lang/builtin/%s.json", i18n_language);
	if (load_pack(path)) return 1;
	snprintf(path, sizeof(path), "frogui/lang/builtin/%s.json", i18n_language);
	if (load_pack(path)) return 1;
	snprintf(path, sizeof(path), "lang/builtin/%s.json", i18n_language);
	return load_pack(path);
}

int i18n_init_from_settings(void) {
	char line[64], language[16] = "en_US";
	FILE *f = fopen("/mnt/sdcard/frogui/settings.txt", "r");
	while (f && fgets(line, sizeof(line), f))
		if (strncmp(line, "language=", 9) == 0) {
			sscanf(line + 9, "%15[A-Za-z_]", language);
			break;
		}
	if (f) fclose(f);
	if (i18n_init(language)) return 1;
	return strcmp(language, "en_US") && i18n_init("en_US");
}

const char *tr(const char *key) {
	for (int i = 0; i < i18n_entry_count; i++)
		if (strcmp(i18n_entries[i].key, key) == 0) return i18n_entries[i].value;
	return key;
}

const char *tr_or(const char *key, const char *fallback) {
	for (int i = 0; i < i18n_entry_count; i++)
		if (strcmp(i18n_entries[i].key, key) == 0) return i18n_entries[i].value;
	return fallback;
}

const char *i18n_current_language(void) { return i18n_language; }
const char *i18n_language_name(void) { return tr("language.en_US"); }
