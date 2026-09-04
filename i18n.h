#ifndef FROGUI_I18N_H
#define FROGUI_I18N_H

/* Runtime language packs live on the SD card.  Keys are stable API; values are
 * plain UTF-8 JSON strings that can be changed without rebuilding FrogUI. */
int i18n_init(const char *language);
const char *tr(const char *key);
const char *tr_or(const char *key, const char *fallback);
const char *i18n_current_language(void);
const char *i18n_language_name(void);

#endif
