#include "recent_games.h"
#include <stdio.h>
#include <string.h>

// Recent games state
static RecentGame recent_games[MAX_RECENT_GAMES];
static int recent_count = 0;

void recent_games_init(void) {
    recent_count = 0;
    recent_games_load();
}

void recent_games_load(void) {
    FILE *fp = fopen(HISTORY_FILE, "r");
    if (!fp) {
        recent_count = 0;
        return;
    }
    
    recent_count = 0;
    char line[512];
    
    while (fgets(line, sizeof(line), fp) && recent_count < MAX_RECENT_GAMES) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Parse line: "core_name|game_name"
        char *separator = strchr(line, '|');
        if (separator) {
            *separator = '\0';
            strncpy(recent_games[recent_count].core_name, line, sizeof(recent_games[recent_count].core_name) - 1);
            strncpy(recent_games[recent_count].game_name, separator + 1, sizeof(recent_games[recent_count].game_name) - 1);
            
            // Create display name
            snprintf(recent_games[recent_count].display_name, sizeof(recent_games[recent_count].display_name),
                    "%s (%s)", recent_games[recent_count].game_name, recent_games[recent_count].core_name);
            
            recent_count++;
        }
    }
    
    fclose(fp);
}

void recent_games_save(void) {
    FILE *fp = fopen(HISTORY_FILE, "w");
    if (!fp) return;
    
    for (int i = 0; i < recent_count; i++) {
        fprintf(fp, "%s|%s\n", recent_games[i].core_name, recent_games[i].game_name);
    }
    
    fclose(fp);
}

void recent_games_add(const char *core_name, const char *game_name) {
    // Check if game already exists
    int existing_index = -1;
    for (int i = 0; i < recent_count; i++) {
        if (strcmp(recent_games[i].core_name, core_name) == 0 && 
            strcmp(recent_games[i].game_name, game_name) == 0) {
            existing_index = i;
            break;
        }
    }
    
    // If exists, move to top
    if (existing_index >= 0) {
        RecentGame temp = recent_games[existing_index];
        for (int i = existing_index; i > 0; i--) {
            recent_games[i] = recent_games[i - 1];
        }
        recent_games[0] = temp;
    } else {
        // Add new game at top
        if (recent_count < MAX_RECENT_GAMES) {
            recent_count++;
        }
        
        // Shift all games down
        for (int i = recent_count - 1; i > 0; i--) {
            recent_games[i] = recent_games[i - 1];
        }
        
        // Add new game at top
        strncpy(recent_games[0].core_name, core_name, sizeof(recent_games[0].core_name) - 1);
        strncpy(recent_games[0].game_name, game_name, sizeof(recent_games[0].game_name) - 1);
        snprintf(recent_games[0].display_name, sizeof(recent_games[0].display_name),
                "%s (%s)", game_name, core_name);
    }
    
    recent_games_save();
}

const RecentGame* recent_games_get_list(void) {
    return recent_games;
}

int recent_games_get_count(void) {
    return recent_count;
}