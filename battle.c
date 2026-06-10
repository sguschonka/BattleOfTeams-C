#include "battle.h"
#include "random.h"
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_MS(x) Sleep(x)
#else
    #include <unistd.h>
    #define SLEEP_MS(x) usleep((x)*1000)
#endif

typedef struct {
    int index;
    int strength;
    int kills;
    char name[MAX_NAME_LEN];
} TopFighter;

int fight_round(Team *attacker_team, Team *defender_team, bool is_bot) {
    if (get_alive_count(attacker_team) == 0 || get_alive_count(defender_team) == 0) {
        return (get_alive_count(defender_team) == 0) ? 1 : 0;
    }

    int attacker_idx;
    Fighter *attacker = NULL;

    // Выбор атакующего
    if (is_bot) {
        attacker_idx = get_best_attacker_index(attacker_team);
        if (attacker_idx == -1) return 0;
        attacker = get_fighter(attacker_team, attacker_idx);
        printf("Бот (команда %s) выбирает бойца %d (%s)\n",
            attacker_team->name, attacker_idx + 1, attacker->name);
    } else {
        printf("\nКоманда %s, выберите атакующего бойца:\n", attacker_team->name);
        attacker_idx = select_fighter_menu(attacker_team, "[<<A]");
        if (attacker_idx == -1) {
            return -1;  // выход
        }
        attacker = get_fighter(attacker_team, attacker_idx);
    }

    int defender_idx;
    Fighter *defender = NULL;

    // Выбор защитника
    if (is_bot) {
        defender_idx = get_weakest_alive_index(defender_team);
        SLEEP_MS(2000);
        if (defender_idx == -1) return 1;
        defender = get_fighter(defender_team, defender_idx);
    } else {
        printf("\nВыберите цель для атаки в команде %s:\n", defender_team->name);
        defender_idx = select_fighter_menu(defender_team, "[<<D]");
        if (defender_idx == -1) {
            return -1;  // выход
        }
        defender = get_fighter(defender_team, defender_idx);
    }
    printf("%s (боец %d, %s) [<<A] атакует %s (боец %d, %s) [<<D]\n",
        attacker_team->name, attacker_idx + 1, attacker->name,
        defender_team->name, defender_idx + 1, defender->name);
    printf("Атакующий сила: %d, Защитник сила: %d\n", attacker->strength, defender->strength);

    int attack_val = random_range(0, attacker->strength);
    int defense_val = random_range(0, defender->strength);
    int min_val = (attack_val < defense_val) ? attack_val : defense_val;

    printf("Атака: %d, Защита: %d, Min: %d\n", attack_val, defense_val, min_val);

    bool attacker_won = false;
    if (attack_val > defense_val) {
        attacker_won = true;
    } else if (defense_val > attack_val) {
        attacker_won = false;
    } else {
        attacker_won = random_range(0, 1) == 0;
    }

    bool defender_died = false;
    bool attacker_died = false;

    if (attacker_won) {
        printf("Атакующий победил!\n");
        change_strength(attacker_team, attacker_idx, min_val);
        change_strength(defender_team, defender_idx, -min_val);
        printf("\nНажмите Enter для следующего раунда...");
        getchar();
        clear_screen();
        
        // Проверяем, убил ли атакующий противника
        if (!defender->alive && defender->strength <= 0) {
            defender_died = true;
        }
    } else {
        printf("Защитник победил!\n");
        change_strength(defender_team, defender_idx, min_val);
        change_strength(attacker_team, attacker_idx, -min_val);
        printf("\nНажмите Enter для следующего раунда...");
        getchar();
        clear_screen();
        
        // Проверяем, убил ли защитник атакующего
        if (!attacker->alive && attacker->strength <= 0) {
            attacker_died = true;
        }
    }

    // === НАЧИСЛЕНИЕ УБИЙСТВА ===
    if (attacker_won && defender_died) {
        attacker->kills++;
        printf("?? %s укакошил %s!\n", attacker->name, defender->name);
    }
    else if (!attacker_won && attacker_died) {
        defender->kills++;
        printf("?? %s укакошил %s!\n", defender->name, attacker->name);
    }

    return (get_alive_count(defender_team) == 0) ? 1 : 0;
    getchar();
    clear_screen();
}

// Новая игра
bool start_new_game(void) {
    clear_screen();
    int game_mode;
    printf("\nВыберите режим игры:\n");
    printf("1. Игрок против игрока\n");
    printf("2. Игрок против бота\n");
    printf("Ваш выбор: ");
    scanf("%d", &game_mode);
    clear_input_buffer();

    Team *team1 = select_or_create_team(1);
    if (!team1) return false;

    Team *team2 = select_or_create_team(2);
    if (!team2) {
        destroy_team(team1);
        return false;
    }

    return play_battle(team1, team2, game_mode);
}

// Продолжение игры, сохраненной в бинарнике
bool continue_saved_game(void) {
    clear_screen();
    Team *team1 = NULL, *team2 = NULL;
    int round = load_game(&team1, &team2);

    if (round == -1 || team1 == NULL || team2 == NULL) {
        printf("Нет сохранённой игры. Начните новую.\n");
        if (team1) destroy_team(team1);
        if (team2) destroy_team(team2);
        return false;
    }

    printf("\n=== ПРОДОЛЖЕНИЕ БИТВЫ (раунд %d) ===\n", round);
    print_team(team1);
    print_team(team2);

    int game_mode;
    printf("Выберите режим игры (1 - PvP, 2 - PvE): ");
    scanf("%d", &game_mode);
    clear_input_buffer();

    return play_battle(team1, team2, game_mode);
}

// Основной цикл битвы
bool play_battle(Team *team1, Team *team2, int game_mode) {
    if (!team1 || !team2) return false;

    clear_screen();  // очищаем перед началом

    int round = 0;
    int battle_result;

    while (get_alive_count(team1) > 0 && get_alive_count(team2) > 0) {
        round++;

        // Ход команды 1 (игрок)
        print_team(team1);
        print_team(team2);
        printf("\n--- Ход команды %s ---\n", team1->name);
        battle_result = fight_round(team1, team2, false);
        if (battle_result == -1) {
            save_game(team1, team2, round);
            printf("Игра сохранена. Возврат в главное меню.\n");
            destroy_team(team1);
            destroy_team(team2);
            return true;
        }
        if (battle_result == 1 || get_alive_count(team1) == 0) {
            printf("%s победили!\n", team1->name);
            break;
        }

        bool is_bot = (game_mode == 2);

        print_team(team1);
        print_team(team2);
        printf("\n--- Ход команды %s ---\n", team2->name);
        battle_result = fight_round(team2, team1, is_bot);

        if (battle_result == -1) {
            save_game(team1, team2, round);
            printf("Игра сохранена. Возврат в главное меню.\n");
            destroy_team(team1);
            destroy_team(team2);
            return true;
        }
        if (battle_result == 1 || get_alive_count(team1) == 0) {
            printf("%s победили!\n", team2->name);
            break;
        }

        save_game(team1, team2, round);  // автосохранение
    }

    printf("\n=== ИТОГОВОЕ СОСТОЯНИЕ ===\n");
    print_team(team1);
    print_team(team2);
    printf("\nНажмите Enter завершения игры...");
    getchar();
    clear_screen();

    if (strlen(team1->filename) > 0 && team1->filename[0] != '\0') 
        save_team_json(team1, team1->filename);
    if (strlen(team2->filename) > 0 && team2->filename[0] != '\0') 
        save_team_json(team2, team2->filename);


    destroy_team(team1);
    destroy_team(team2);
    delete_save();

    return true;
}

// Сортировка вставками (Insertion Sort) по убыванию силы, затем по убийствам
void insertion_sort_top(TopFighter *arr, int n) {
    for (int i = 1; i < n; i++) {
        TopFighter key = arr[i];
        int j = i - 1;

        // Сдвигаем все элементы, которые меньше текущего ключа
        while (j >= 0 && 
              (arr[j].strength < key.strength || 
              (arr[j].strength == key.strength && arr[j].kills < key.kills))) {
            
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

void print_top_5_fighters(const Team *team) {
    if (!team || team->size == 0) {
        printf("Команда пуста.\n");
        return;
    }

    // Создаём массив для всех бойцов (не только живых)
    TopFighter *top = malloc(team->size * sizeof(TopFighter));
    if (!top) return;

    int count = 0;
    for (int i = 0; i < team->size; i++) {
        // Добавляем всех бойцов, независимо от alive
        top[count].index = i;
        top[count].strength = team->fighters[i].base_strength;   // базовая сила
        top[count].kills = team->fighters[i].kills;
        strcpy(top[count].name, team->fighters[i].name);
        count++;
    }

    if (count == 0) {
        printf("В команде %s нет бойцов.\n", team->name);
        free(top);
        return;
    }

    // Сортировка по убыванию base_strength, затем по kills
    insertion_sort_top(top, count);
    int limit = (count < 5) ? count : 5;

    printf("\n=== ТОП-%d БОЙЦОВ КОМАНДЫ %s ===\n\n", limit, team->name);
    printf("%-3s %-25s %6s %8s\n", "№", "Имя", "Сила", "Убийств");
    printf("───────────────────────────────────────────────\n");

    for (int i = 0; i < limit; i++) {
        printf("%-3d %-25.25s %6d %8d\n",
            i + 1,
            top[i].name,
            top[i].strength,
            top[i].kills);
    }
    printf("\n");
    free(top);
}

// Показ ТОП-5 бойцов
void show_top_fighters_menu() {
    clear_screen();
    ensure_team_folder();   // на всякий случай
    list_teams();

    // Проверяем, есть ли хотя бы одна команда
    char filename[256];
    snprintf(filename, sizeof(filename), "%s/team1.txt", TEAM_FOLDER);
    FILE *test = fopen(filename, "r");
    if (!test) {
        printf("Сначала создайте хотя бы одну команду.\n");
        return;
    }
    fclose(test);

    printf("\nВведите номер команды для просмотра ТОПа (0 - отмена): ");
    int choice;
    scanf("%d", &choice);
    clear_input_buffer();

    if (choice <= 0) return;

    Team *team = load_team_by_number(choice);
    if (!team) {
        printf("Не удалось загрузить команду №%d.\n", choice);
        return;
    }

    print_top_5_fighters(team);
    destroy_team(team);
}

// Очистка экрана
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}