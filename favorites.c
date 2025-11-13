#include "favorites.h"
#include <stdio.h>
#include <string.h>

#define FAVORITES_FILE "/mnt/sda1/frogui/favorites.txt"

// Favorites state
static FavoriteGame favorites[MAX_FAVORITES];
static int favorite_count = 0;

void favorites_init(void) {
    favorite_count = 0;
    favorites_load();
}

void favorites_load(void) {
    FILE *fp = fopen(FAVORITES_FILE, "r");
    if (!fp) {
        favorite_count = 0;
        return;
    }

    favorite_count = 0;
    char line[512];

    while (fgets(line, sizeof(line), fp) && favorite_count < MAX_FAVORITES) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;

        // Parse line: "core_name|game_name|full_path"
        char *separator1 = strchr(line, '|');
        if (separator1) {
            *separator1 = '\0';
            char *separator2 = strchr(separator1 + 1, '|');
            if (separator2) {
                *separator2 = '\0';
                strncpy(favorites[favorite_count].core_name, line, sizeof(favorites[favorite_count].core_name) - 1);
                strncpy(favorites[favorite_count].game_name, separator1 + 1, sizeof(favorites[favorite_count].game_name) - 1);
                strncpy(favorites[favorite_count].full_path, separator2 + 1, sizeof(favorites[favorite_count].full_path) - 1);

                // Create display name
                snprintf(favorites[favorite_count].display_name, sizeof(favorites[favorite_count].display_name),
                        "%s (%s)", favorites[favorite_count].game_name, favorites[favorite_count].core_name);

                favorite_count++;
            }
        }
    }

    fclose(fp);
}

void favorites_save(void) {
    FILE *fp = fopen(FAVORITES_FILE, "w");
    if (!fp) return;

    for (int i = 0; i < favorite_count; i++) {
        fprintf(fp, "%s|%s|%s\n", favorites[i].core_name, favorites[i].game_name, favorites[i].full_path);
    }

    fclose(fp);
}

bool favorites_toggle(const char *core_name, const char *game_name, const char *full_path) {
    // Check if already favorited
    int existing_index = -1;
    for (int i = 0; i < favorite_count; i++) {
        if (strcmp(favorites[i].core_name, core_name) == 0 &&
            strcmp(favorites[i].game_name, game_name) == 0) {
            existing_index = i;
            break;
        }
    }

    if (existing_index >= 0) {
        // Remove from favorites
        for (int i = existing_index; i < favorite_count - 1; i++) {
            favorites[i] = favorites[i + 1];
        }
        favorite_count--;
        favorites_save();
        return false; // Removed
    } else {
        // Add to favorites
        if (favorite_count >= MAX_FAVORITES) {
            return false; // List full
        }

        strncpy(favorites[favorite_count].core_name, core_name, sizeof(favorites[favorite_count].core_name) - 1);
        strncpy(favorites[favorite_count].game_name, game_name, sizeof(favorites[favorite_count].game_name) - 1);
        strncpy(favorites[favorite_count].full_path, full_path, sizeof(favorites[favorite_count].full_path) - 1);
        snprintf(favorites[favorite_count].display_name, sizeof(favorites[favorite_count].display_name),
                "%s (%s)", game_name, core_name);
        favorite_count++;
        favorites_save();
        return true; // Added
    }
}

bool favorites_is_favorited(const char *core_name, const char *game_name) {
    for (int i = 0; i < favorite_count; i++) {
        if (strcmp(favorites[i].core_name, core_name) == 0 &&
            strcmp(favorites[i].game_name, game_name) == 0) {
            return true;
        }
    }
    return false;
}

const FavoriteGame* favorites_get_list(void) {
    return favorites;
}

int favorites_get_count(void) {
    return favorite_count;
}
