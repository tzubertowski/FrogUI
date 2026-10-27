#include "i18n.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define I18N_MAX_FILE 32768
#define I18N_MAX_ENTRIES 320
#define I18N_KEY_MAX 80
#define I18N_MAX_LANGUAGES 32
#define I18N_LANGUAGE_MAX 16

typedef struct {
  char key[I18N_KEY_MAX];
  char *value;
} I18nEntry;
static char i18n_data[I18N_MAX_FILE];
static I18nEntry i18n_entries[I18N_MAX_ENTRIES];
static int i18n_entry_count;
static char i18n_language[I18N_LANGUAGE_MAX] = "en_US";
static char i18n_languages[I18N_MAX_LANGUAGES][I18N_LANGUAGE_MAX];
static int i18n_language_total;

static int language_compare(const void *a, const void *b) {
  return strcmp((const char *)a, (const char *)b);
}

int i18n_scan_languages(void) {
  static const char *paths[] = {
      "/mnt/sdcard/frogui/lang/builtin", "frogui/lang/builtin", "lang/builtin"};
  DIR *dir = NULL;
  struct dirent *entry;
  int path_index;

  i18n_language_total = 0;
  for (path_index = 0; path_index < (int)(sizeof(paths) / sizeof(paths[0])); path_index++) {
    dir = opendir(paths[path_index]);
    if (dir)
      break;
  }
  if (!dir) {
    strcpy(i18n_languages[0], "en_US");
    i18n_language_total = 1;
    return 1;
  }

  while ((entry = readdir(dir)) && i18n_language_total < I18N_MAX_LANGUAGES) {
    const char *dot = strrchr(entry->d_name, '.');
    size_t length;
    int valid = 1;
    if (!dot || strcmp(dot, ".json") != 0 || dot == entry->d_name)
      continue;
    length = (size_t)(dot - entry->d_name);
    if (length >= I18N_LANGUAGE_MAX)
      continue;
    for (size_t i = 0; i < length; i++)
      if (!((entry->d_name[i] >= 'a' && entry->d_name[i] <= 'z') ||
            (entry->d_name[i] >= 'A' && entry->d_name[i] <= 'Z') ||
            (entry->d_name[i] >= '0' && entry->d_name[i] <= '9') ||
            entry->d_name[i] == '_')) {
        valid = 0;
        break;
      }
    if (valid) {
      memcpy(i18n_languages[i18n_language_total], entry->d_name, length);
      i18n_languages[i18n_language_total++][length] = '\0';
    }
  }
  closedir(dir);
  if (!i18n_language_total) {
    strcpy(i18n_languages[0], "en_US");
    i18n_language_total = 1;
    return 1;
  }
  qsort(i18n_languages, (size_t)i18n_language_total, sizeof(i18n_languages[0]),
        language_compare);
  for (int i = 0; i < i18n_language_total; i++)
    if (strcmp(i18n_languages[i], "en_US") == 0) {
      char fallback[I18N_LANGUAGE_MAX];
      strcpy(fallback, i18n_languages[0]);
      strcpy(i18n_languages[0], i18n_languages[i]);
      strcpy(i18n_languages[i], fallback);
      break;
    }
  return i18n_language_total;
}

int i18n_language_count(void) { return i18n_language_total; }

const char *i18n_language_code_at(int index) {
  return index >= 0 && index < i18n_language_total ? i18n_languages[index] : NULL;
}

static char *skip_ws(char *p) {
  while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
    p++;
  return p;
}

/* Decode a JSON string in place. Raw UTF-8 keeps packs editable without a
 * third-party parser or an extra rootfs dependency. */
static char *json_string(char **cursor) {
  char *p = skip_ws(*cursor), *out, *start;
  if (*p != '"')
    return NULL;
  p++;
  out = start = p;
  while (*p && *p != '"') {
    if (*p == '\\') {
      p++;
      if (!*p)
        return NULL;
      switch (*p) {
      case 'n':
        *out++ = '\n';
        break;
      case 'r':
        *out++ = '\r';
        break;
      case 't':
        *out++ = '\t';
        break;
      case '"':
        *out++ = '"';
        break;
      case '\\':
        *out++ = '\\';
        break;
      case '/':
        *out++ = '/';
        break;
      default:
        return NULL;
      }
      p++;
    } else
      *out++ = *p++;
  }
  if (*p != '"')
    return NULL;
  *out = '\0';
  *cursor = p + 1;
  return start;
}

static int load_pack(const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return 0;
  size_t n = fread(i18n_data, 1, sizeof(i18n_data) - 1, f);
  fclose(f);
  if (!n || n == sizeof(i18n_data) - 1)
    return 0;
  i18n_data[n] = '\0';
  char *p = skip_ws(i18n_data);
  if (*p++ != '{')
    return 0;
  i18n_entry_count = 0;
  for (;;) {
    p = skip_ws(p);
    if (*p == '}')
      return i18n_entry_count > 0;
    if (i18n_entry_count >= I18N_MAX_ENTRIES)
      return 0;
    char *key = json_string(&p);
    if (!key)
      return 0;
    p = skip_ws(p);
    if (*p++ != ':')
      return 0;
    char *value = json_string(&p);
    if (!value || strlen(key) >= I18N_KEY_MAX)
      return 0;
    strcpy(i18n_entries[i18n_entry_count].key, key);
    i18n_entries[i18n_entry_count++].value = value;
    p = skip_ws(p);
    if (*p == ',') {
      p++;
      continue;
    }
    if (*p == '}')
      return 1;
    return 0;
  }
}

int i18n_init(const char *language) {
  char path[160];
  if (!language || !*language)
    language = "en_US";
  strncpy(i18n_language, language, sizeof(i18n_language) - 1);
  i18n_language[sizeof(i18n_language) - 1] = '\0';
  snprintf(path, sizeof(path), "/mnt/sdcard/frogui/lang/builtin/%s.json",
           i18n_language);
  if (load_pack(path))
    return 1;
  snprintf(path, sizeof(path), "frogui/lang/builtin/%s.json", i18n_language);
  if (load_pack(path))
    return 1;
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
  if (f)
    fclose(f);
  if (i18n_init(language))
    return 1;
  return strcmp(language, "en_US") && i18n_init("en_US");
}

const char *tr(const char *key) {
  for (int i = 0; i < i18n_entry_count; i++)
    if (strcmp(i18n_entries[i].key, key) == 0)
      return i18n_entries[i].value;
  return key;
}

const char *tr_or(const char *key, const char *fallback) {
  for (int i = 0; i < i18n_entry_count; i++)
    if (strcmp(i18n_entries[i].key, key) == 0)
      return i18n_entries[i].value;
  return fallback;
}

const char *get_default_language_font_name(const char *global_fallback) {
  return tr_or("_default.font_name", global_fallback);
}

const char *i18n_current_language(void) { return i18n_language; }

const char *i18n_language_name(void) {
  static char key[32];
  snprintf(key, sizeof(key), "language.%s", i18n_language);
  return tr(key);
}

int i18n_value_count(void) { return i18n_entry_count; }

const char *i18n_key_at(int index) {
  return index >= 0 && index < i18n_entry_count ? i18n_entries[index].key
                                                : NULL;
}
const char *i18n_value_at(int index) {
  return index >= 0 && index < i18n_entry_count ? i18n_entries[index].value
                                                : NULL;
}
