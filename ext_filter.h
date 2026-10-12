/*
 * ext_filter.h - per-system-folder extension whitelist for the ROM browser.
 *
 * Hides companion data files (e.g. PS1 .bin track blobs) that clutter the
 * browser while keeping the loadable index files (.cue/.m3u/...) visible.
 * Managed from the SELECT core picker when opened inside a filtered system.
 */
#ifndef EXT_FILTER_H
#define EXT_FILTER_H

#include <stdbool.h>
#include <stddef.h>

/* Maximum folders with a stored whitelist. */
#define EXT_FILTER_MAX_FOLDERS 64
/* Max extensions per folder. */
#define EXT_FILTER_MAX_EXTS    16
/* Max chars per extension (without dot). */
#define EXT_FILTER_EXT_LEN     8

/* Load persisted filters from /mnt/sdcard/frogui/ext_filters.txt, applying
 * built-in defaults first (a stored line overrides that folder's defaults).
 * Call once at init, before any scan. */
void ext_filter_load(void);

/* True if `file_ext` (the file's extension, isolated by the caller, with or
 * without a leading dot, any case) does NOT match `folder_name`'s active
 * whitelist and should be hidden from the browser. An empty extension is
 * hidden while the whitelist is active. The folder resolution is cached
 * (one lookup per scanned directory, not per file). */
bool ext_filter_should_hide(const char *folder_name, const char *file_ext);

/* True if the folder has an active (enabled, non-empty) whitelist. */
bool ext_filter_folder_active(const char *folder_name);

/* True if the folder has any stored whitelist, enabled or not. */
bool ext_filter_has_list(const char *folder_name);

/* Append one extension (no dot, case-insensitive) to the folder's list. */
void ext_filter_add(const char *folder_name, const char *ext);

/* Enable/disable the folder's whitelist without losing its extension list. */
void ext_filter_set_enabled(const char *folder_name, bool enabled);
bool ext_filter_get_enabled(const char *folder_name);

/* Per-extension queries used by the picker UI. */
bool ext_filter_has_ext(const char *folder_name, const char *ext);
int  ext_filter_ext_count(const char *folder_name);
/* Copy extension #idx (lowercase, no dot) into out. Returns 0 if absent. */
int  ext_filter_get_ext_at(const char *folder_name, int idx, char *out, size_t n);

/* Add/remove an extension (no dot) from the folder's list. Returns true if
 * the list changed. Adding to an empty list enables the filter. */
bool ext_filter_toggle_ext(const char *folder_name, const char *ext);

/* Add an extension marked as a built-in default; such rows can be recognized
 * and shown alongside user-added ones with the same toggle behavior. */
void ext_filter_add_builtin(const char *folder_name, const char *ext);

/* True if `ext` in this folder's list came from the built-in defaults
 * (false for user-typed extensions). */
bool ext_filter_ext_is_builtin(const char *folder_name, const char *ext);

#endif
