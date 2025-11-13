#ifndef FAVORITES_H
#define FAVORITES_H

#include <stdbool.h>

#define MAX_FAVORITES 100

typedef struct {
    char core_name[64];
    char game_name[256];
    char full_path[512];
    char display_name[320];
} FavoriteGame;

void favorites_init(void);
void favorites_load(void);
void favorites_save(void);
bool favorites_toggle(const char *core_name, const char *game_name, const char *full_path);
bool favorites_is_favorited(const char *core_name, const char *game_name);
const FavoriteGame* favorites_get_list(void);
int favorites_get_count(void);

#endif
