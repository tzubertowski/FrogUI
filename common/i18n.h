#ifndef TREEFROG_I18N_H
#define TREEFROG_I18N_H

/* Shared FrogUI/PicoArch runtime language-pack loader. Packs are flat UTF-8
 * JSON at /mnt/sdcard/frogui/lang/builtin/<locale>.json. */
int i18n_init(const char *language);
int i18n_init_from_settings(void);
const char *tr(const char *key);
const char *tr_or(const char *key, const char *fallback);
const char *i18n_current_language(void);
const char *i18n_language_name(void);
int i18n_value_count(void);
const char *i18n_key_at(int index);
const char *i18n_value_at(int index);

#endif
