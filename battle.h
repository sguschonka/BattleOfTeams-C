#ifndef BATTLE_H
#define BATTLE_H

#include "team.h"
#include <stdbool.h>

// Проводит один раунд боя.
// Параметры:
//   attacker_team, defender_team - команды
//   is_bot - управляется ли атакующая команда ботом
//   silent - если true, не выводить сообщения в консоль (используется при тестировании)
// Возвращает:
//   1 - атакующая команда уничтожила защитника (победа)
//   0 - бой продолжается
//  -1 - игрок выбрал выход (ввёл 0) во время выбора бойца или цели
int fight_round(Team *attacker_team, Team *defender_team, bool is_bot);

bool start_new_game(void);
bool continue_saved_game(void);
bool play_battle(Team *team1, Team *team2, int game_mode);

void save_game(Team *team1, Team *team2, int round);
int  load_game(Team **team1, Team **team2);
void delete_save(void);

void print_top_5_fighters(const Team *team);
void show_top_fighters_menu(void);

// Очистка экрана (кроссплатформенная)
void clear_screen(void);

#endif