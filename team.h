#ifndef TEAM_H
#define TEAM_H

#include <stdio.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_NAME_LEN 50
#define MAX_FILENAME_LEN 50
#define TEAM_FOLDER "Teams"
#define MAX_TOTAL_STRENGTH 100

typedef struct {
    int id;
    char name[MAX_NAME_LEN];
    int strength;          // текущая сила
    int base_strength;     // стартовая сила
    bool alive;
    int kills;
} Fighter;

typedef struct {
    char name[MAX_NAME_LEN];
    Fighter *fighters;
    int size;
    int alive_count;
    char filename[MAX_FILENAME_LEN];   // путь к файлу
} Team;

Team* create_team(int num_fighters);
void destroy_team(Team *team);
void input_team(Team *team);
void print_team(const Team *team);
int get_alive_count(const Team *team);
int get_random_alive_fighter_index(const Team *team);
Fighter* get_fighter(const Team *team, int index);
void kill_fighter(Team *team, int index);
void change_strength(Team *team, int index, int delta);
void redistribute_strength(Team *team);

void ensure_team_folder();
int get_next_team_number();
void list_teams();
void save_team_json(const Team *team, const char *filename);   // новое имя
Team* load_team_json(const char *filename);                   // загрузка из JSON
Team* load_team_by_number(int num);
Team* select_or_create_team(int player_index);

void clear_input_buffer();
void read_line(FILE *f, char *buffer, int size);
void input_string(const char *prompt, char *buffer, int size);

int get_best_attacker_index(const Team *team);
int get_weakest_alive_index(const Team *team);

void save_team_to_stream(const Team *team, FILE *f);
Team* load_team_from_stream(FILE *f);

int validate_team_strength(const Team *team);

// Интерактивный выбор бойца стрелками.
// Возвращает индекс выбранного бойца (0..size-1) или -1, если пользователь вышел.
// role_marker: строка, показываемая у подсвеченного пункта (например, "[<<A]" или "[<<D]").
int select_fighter_menu(const Team *team, const char *role_marker);

#endif