#include <stdio.h>
#include <stdlib.h>
#include "team.h"
#include "battle.h"
#include "random.h"
#include "test.h"
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
#else
    #include <sys/types.h>
#endif

#define SAVE_FOLDER "Save"
#define SAVE_FILE "Save/savegame.dat"

void ensure_save_folder() {
    struct stat st;
    if (stat(SAVE_FOLDER, &st) != 0) {
        #ifdef _WIN32
            _mkdir(SAVE_FOLDER);
        #endif
    }
}

void print_main_menu() {
    printf("\n========== ГЛАВНОЕ МЕНЮ ==========\n");
    printf("1. Начать новую игру\n");
    printf("2. Продолжить сохранённую игру\n");
    printf("3. Показать ТОП-5 сильнейших бойцов\n");
    printf("4. Выйти\n");
    printf("Ваш выбор: ");
}

void save_game(Team *team1, Team *team2, int round) {
    ensure_save_folder();
    FILE *f = fopen(SAVE_FILE, "wb");
    if (!f) {
        printf("Ошибка сохранения игры.\n");
        return;
    }
    // пишем раунд (int) и две команды бинарно
    fwrite(&round, sizeof(round), 1, f);
    save_team_to_stream(team1, f);
    save_team_to_stream(team2, f);
    fclose(f);
}

int load_game(Team **team1, Team **team2) {
    FILE *f = fopen(SAVE_FILE, "rb");
    if (!f) return -1;

    int round;
    if (fread(&round, sizeof(round), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    *team1 = load_team_from_stream(f);
    *team2 = load_team_from_stream(f);
    fclose(f);

    if (!*team1 || !*team2) return -1;
    return round;
}

void delete_save() {
    remove(SAVE_FILE);
}

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(866);
    SetConsoleCP(866);
#endif

    init_random();
    run_all_tests();
    ensure_save_folder();

    int main_choice;
    while (1) {
        print_main_menu();
        scanf("%d", &main_choice);
        clear_input_buffer();

        if (main_choice == 1) {
            start_new_game();
        }
        else if (main_choice == 2) {
            continue_saved_game();
        }
        else if (main_choice == 3) {
            show_top_fighters_menu();
        }
        else if (main_choice == 4) {
            printf("Ты уже уходишь? =(\n");
            break;
        }
        else {
            printf("Неверный выбор.\n");
            printf("Нажмите Enter для продолжения...");
            getchar();
        }
    }
    return 0;
}