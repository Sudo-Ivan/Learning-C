#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"

FILE *log_file;

void log_event(const char *event) {
    time_t now;
    time(&now);
    fprintf(log_file, "[%s] %s\n", ctime(&now), event);
    fflush(log_file);
}

typedef struct {
    char name[50];
    int health;
    int items;
    int attack;
    int potions;
    int gold;
} Player;

typedef struct {
    char name[50];
    int health;
    int attack;
    int is_hostile;
    int gold;
} Enemy;

typedef struct {
    char description[100];
    int has_item;
    Enemy *enemy;
    int temperature;
    int is_dark;
} Room;

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void init_rooms(Room rooms[], int count) {
    char *descriptions[] = {
        "Dark Cave - Damp walls surround you",
        "Forest Clearing - Sunlight filters through the trees",
        "Ancient Temple - Golden artifacts glitter in the darkness",
        "Underground Lake - Crystal clear water stretches into darkness",
        "Volcanic Chamber - Steam rises from cracks",
        "Ice Cavern - Frost covers the walls"
    };
    
    for(int i = 0; i < count; i++) {
        strcpy(rooms[i].description, descriptions[i % 6]);
        rooms[i].has_item = rand() % 2;
        rooms[i].temperature = rand() % 40 - 10;
        rooms[i].is_dark = rand() % 2;
        
        if(rand() % 3 == 0) {
            rooms[i].enemy = malloc(sizeof(Enemy));
            strcpy(rooms[i].enemy->name, "Goblin");
            rooms[i].enemy->health = 30 + rand() % 20;
            rooms[i].enemy->attack = 5 + rand() % 10;
            rooms[i].enemy->is_hostile = rand() % 2;
        } else {
            rooms[i].enemy = NULL;
        }
    }
}

void print_status(Player *player, Room *current_room) {
    clear_screen();
    printf("\n" CYAN "=== Adventure Game ===" RESET "\n");
    printf("\nLocation: " YELLOW "%s" RESET, current_room->description);
    printf("\nConditions: %s, " BLUE "%d°C" RESET, 
        current_room->is_dark ? RED "Dark" RESET : GREEN "Lit" RESET,
        current_room->temperature);
    printf("\nHealth: " RED "%d" RESET " | Attack: " YELLOW "%d" RESET " | Items: " GREEN "%d" RESET "\n", 
        player->health, player->attack, player->items);
    
    if(current_room->enemy) {
        printf("\nEnemy Present: " RED "%s" RESET " (%s) - Health: %d\n", 
            current_room->enemy->name,
            current_room->enemy->is_hostile ? RED "Hostile" RESET : GREEN "Peaceful" RESET,
            current_room->enemy->health);
    }
    
    printf("\nCommands: " CYAN "move, search, fight, heal, inventory, quit" RESET "\n");
}

void combat_round(Player *player, Enemy *enemy) {
    if(enemy->is_hostile) {
        int player_damage = rand() % player->attack;
        int enemy_damage = rand() % enemy->attack;
        
        enemy->health -= player_damage;
        printf("You deal %d damage!\n", player_damage);
        
        if(enemy->health > 0) {
            player->health -= enemy_damage;
            printf("Enemy deals %d damage!\n", enemy_damage);
        }
    } else {
        printf("\n" YELLOW "This enemy is peaceful!" RESET "\n");
        log_event("Player attempted to fight peaceful enemy");
    }
}

void handle_enemy_death(Player *player, Enemy *enemy) {
    int gold_drop = 5 + rand() % 15;
    int potion_drop = rand() % 2;
    
    printf("\n" GREEN "Enemy defeated!" RESET);
    printf("\n" YELLOW "You found %d gold!" RESET, gold_drop);
    player->gold += gold_drop;
    player->attack += 2;
    
    if(potion_drop) {
        printf("\n" GREEN "You found a healing potion!" RESET);
        player->potions++;
    }
    
    char log_msg[100];
    sprintf(log_msg, "Enemy defeated. Loot: %d gold, %d potions", gold_drop, potion_drop);
    log_event(log_msg);
    
    free(enemy);
}

void show_inventory(Player *player) {
    clear_screen();
    printf("\n" CYAN "=== Inventory ===" RESET);
    printf("\nGold: " YELLOW "%d" RESET, player->gold);
    printf("\nHealing Potions: " GREEN "%d" RESET, player->potions);
    printf("\nAttack Power: " RED "%d" RESET, player->attack);
    printf("\nItems: " MAGENTA "%d" RESET, player->items);
    printf("\n\nPress Enter to continue...");
    getchar();
    getchar();
    log_event("Player checked inventory");
}

void heal_player(Player *player) {
    if(player->potions > 0) {
        int heal_amount = 30 + rand() % 20;
        player->health += heal_amount;
        player->potions--;
        printf("\n" GREEN "Healed %d health!" RESET "\n", heal_amount);
        
        char log_msg[100];
        sprintf(log_msg, "Player healed for %d health", heal_amount);
        log_event(log_msg);
    } else {
        printf("\n" RED "No potions to use for healing!" RESET "\n");
        log_event("Player attempted to heal without potions");
    }
}

int main() {
    srand(time(NULL));
    Player player = {"Hero", 100, 0, 15, 2, 0};
    const int room_count = 8;
    Room rooms[room_count];
    int current_room_id = 0;
    char command[20];
    
    init_rooms(rooms, room_count);
    
    log_file = fopen("game-event.log", "a");
    if (!log_file) {
        printf("Error opening log file!\n");
        return 1;
    }
    
    log_event("Game started");
    
    while (player.health > 0) {
        print_status(&player, &rooms[current_room_id]);
        
        printf("\nWhat would you like to do? ");
        scanf("%s", command);
        
        if (strcmp(command, "quit") == 0) {
            log_event("Player quit the game");
            break;
        }
        else if (strcmp(command, "move") == 0) {
            int old_room = current_room_id;
            printf("Choose room (0-%d): ", room_count - 1);
            scanf("%d", &current_room_id);
            if (current_room_id < 0 || current_room_id >= room_count) {
                current_room_id = 0;
            }
            
            char log_msg[100];
            sprintf(log_msg, "Player moved from room %d to %d", old_room, current_room_id);
            log_event(log_msg);
            
            if(rooms[current_room_id].temperature < 0) {
                player.health -= 5;
                printf("\n" RED "The cold damages you!" RESET "\n");
                log_event("Player took cold damage");
            }
        }
        else if (strcmp(command, "search") == 0) {
            if(rooms[current_room_id].is_dark) {
                printf("\n" YELLOW "Too dark to search effectively..." RESET "\n");
                log_event("Player searched dark room");
                if(rand() % 2 == 0) {
                    player.health -= 5;
                    printf(RED "You hurt yourself in the darkness!\n" RESET);
                    log_event("Player hurt themselves in darkness");
                }
            }
            else if (rooms[current_room_id].has_item) {
                printf("\n" GREEN "You found an item!" RESET "\n");
                player.items++;
                player.attack += 2;
                rooms[current_room_id].has_item = 0;
                log_event("Player found an item");
            } else {
                printf("\nNothing here...\n");
                log_event("Player searched empty room");
            }
            getchar();
            getchar();
        }
        else if (strcmp(command, "fight") == 0) {
            if(rooms[current_room_id].enemy && rooms[current_room_id].enemy->health > 0) {
                log_event("Combat started");
                combat_round(&player, rooms[current_room_id].enemy);
                
                if(rooms[current_room_id].enemy->health <= 0) {
                    handle_enemy_death(&player, rooms[current_room_id].enemy);
                    rooms[current_room_id].enemy = NULL;
                }
                
                if(player.health <= 0) {
                    printf("\n" RED "You have been defeated!" RESET "\n");
                    log_event("Player was defeated");
                    break;
                }
            } else {
                printf("\nNo enemy to fight!\n");
                log_event("Player attempted to fight with no enemy present");
            }
            
            while(getchar() != '\n'); // Clear input buffer
            printf("\nPress Enter to continue...");
            getchar();
        }
        else if (strcmp(command, "heal") == 0) {
            heal_player(&player);
            getchar();
            getchar();
        }
        else if (strcmp(command, "inventory") == 0) {
            show_inventory(&player);
        }
        
        for(int i = 0; i < room_count; i++) {
            if(!rooms[i].enemy && rand() % 20 == 0) {
                rooms[i].enemy = malloc(sizeof(Enemy));
                strcpy(rooms[i].enemy->name, "Goblin");
                rooms[i].enemy->health = 30 + rand() % 20;
                rooms[i].enemy->attack = 5 + rand() % 10;
                rooms[i].enemy->is_hostile = rand() % 2;
                rooms[i].enemy->gold = 5 + rand() % 15;
                
                if(i == current_room_id) {
                    printf("\n" RED "A new enemy has appeared!" RESET "\n");
                    log_event("New enemy spawned in player's room");
                }
            }
        }
    }
    
    printf("\n" YELLOW "=== Game Over ===" RESET);
    printf("\nFinal Stats:");
    printf("\nGold Collected: " YELLOW "%d" RESET, player.gold);
    printf("\nItems Found: " GREEN "%d" RESET, player.items);
    printf("\nFinal Attack Power: " RED "%d" RESET "\n", player.attack);
    
    char final_msg[100];
    sprintf(final_msg, "Game Over! Gold: %d, Items: %d, Attack: %d", 
            player.gold, player.items, player.attack);
    log_event(final_msg);
    
    for(int i = 0; i < room_count; i++) {
        if(rooms[i].enemy) {
            free(rooms[i].enemy);
        }
    }
    fclose(log_file);
    return 0;
} 