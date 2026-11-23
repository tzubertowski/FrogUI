#ifndef FROGUI_SETTINGS_H
#define FROGUI_SETTINGS_H

/* Theme profile hooks. Changing these values does not write settings; callers
 * apply a complete profile and then use the normal settings save path. */
const char *frogui_settings_layout_get(void);
void frogui_settings_layout_set(const char *value);
int frogui_pillboxes_get(void); /* decorative pills only; selection pills remain */
void frogui_pillboxes_set(int enabled);
int frogui_prominimal_style_active(void);

#endif
