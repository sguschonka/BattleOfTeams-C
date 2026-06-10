#include "team.h"
#include "random.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include "battle.h"

#ifdef _WIN32
    #include <direct.h>
    #include <conio.h>
    #define getch _getch
#else
    #include <sys/types.h>
    #include <termios.h>
    #include <unistd.h>
    static int getch(void) {
        struct termios oldt, newt;
        int ch;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
    }
#endif

void ensure_team_folder() {
    struct stat st;
    if (stat(TEAM_FOLDER, &st) != 0) {
        #ifdef _WIN32
            _mkdir(TEAM_FOLDER);
        #else
            mkdir(TEAM_FOLDER, 0777);
        #endif
    }
}

void read_line(FILE *f, char *buffer, int size) {
    if (fgets(buffer, size, f) == NULL) {
        buffer[0] = '\0';
        return;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n')
        buffer[len-1] = '\0';
}

int get_next_team_number() {
    ensure_team_folder();
    int n = 1;
    char filename[256];
    while (1) {
        snprintf(filename, sizeof(filename), "%s/team%d.txt", TEAM_FOLDER, n);
        FILE *f = fopen(filename, "r");
        if (!f) break;
        fclose(f);
        n++;
    }
    return n;
}

void list_teams() {
    ensure_team_folder();
    printf("\nСписок сохранённых команд:\n");
    int count = 0;
    int n = 1;
    char filename[256];
    while (1) {
        snprintf(filename, sizeof(filename), "%s/team%d.txt", TEAM_FOLDER, n);
        FILE *f = fopen(filename, "r");
        if (!f) break;
        fclose(f);
        Team *team = load_team_by_number(n);
        if (team) {
            printf("%d. %s (team%d.txt)\n", ++count, team->name, n);
            destroy_team(team);
        } else {
            printf("%d. <неизвестно> (team%d.txt)\n", ++count, n);
        }
        n++;
    }
    if (count == 0) printf("Нет сохранённых команд.\n");
}

// Сохранение команды в JSON-файл
void save_team_json(const Team *team, const char *filename) {
    ensure_team_folder();
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", TEAM_FOLDER, filename);
    FILE *f = fopen(fullpath, "w");
    if (!f) {
        printf("Ошибка сохранения: %s\n", fullpath);
        return;
    }
    fprintf(f, "{\n");
    fprintf(f, "  \"name\": \"%s\",\n", team->name);
    fprintf(f, "  \"size\": %d,\n", team->size);
    fprintf(f, "  \"fighters\": [\n");
    for (int i = 0; i < team->size; i++) {
        fprintf(f, "    {\n");
        fprintf(f, "      \"id\": %d,\n", team->fighters[i].id);
        fprintf(f, "      \"name\": \"%s\",\n", team->fighters[i].name);
        fprintf(f, "      \"strength\": %d,\n", team->fighters[i].strength);
        fprintf(f, "      \"base_strength\": %d,\n", team->fighters[i].base_strength);
        fprintf(f, "      \"kills\": %d,\n", team->fighters[i].kills);
        fprintf(f, "      \"alive\": %s\n", team->fighters[i].alive ? "true" : "false");
        fprintf(f, "    }%s\n", (i < team->size - 1) ? "," : "");
    }
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);
    printf("Команда сохранена в %s\n", filename);
}

// Загрузка команды из JSON-файла
Team* load_team_json(const char *filename) {
    char fullpath[256];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", TEAM_FOLDER, filename);
    FILE *f = fopen(fullpath, "r");
    if (!f) {
        printf("Файл %s не найден.\n", fullpath);
        return NULL;
    }
    Team *team = (Team*)malloc(sizeof(Team));
    if (!team) {
        fclose(f);
        return NULL;
    }
    char line[512];
    // Читаем name
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "\"name\":")) {
            sscanf(line, " \"name\": \"%[^\"]\"", team->name);
            break;
        }
    }
    // Читаем size
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "\"size\":")) {
            sscanf(strchr(line, ':') + 1, "%d", &team->size);
            break;
        }
    }
    team->fighters = (Fighter*)malloc(team->size * sizeof(Fighter));
    team->alive_count = 0;
    int i = 0;
    while (fgets(line, sizeof(line), f) && i < team->size) {
        if (strstr(line, "\"id\":")) {
            sscanf(strchr(line, ':') + 1, "%d", &team->fighters[i].id);
            // name
            fgets(line, sizeof(line), f);
            sscanf(line, " \"name\": \"%[^\"]\"", team->fighters[i].name);
            // strength
            fgets(line, sizeof(line), f);
            sscanf(strchr(line, ':') + 1, "%d", &team->fighters[i].strength);
            // base_strength
            fgets(line, sizeof(line), f);
            sscanf(strchr(line, ':') + 1, "%d", &team->fighters[i].base_strength);
            // kills
            fgets(line, sizeof(line), f);
            sscanf(strchr(line, ':') + 1, "%d", &team->fighters[i].kills);
            // alive
            fgets(line, sizeof(line), f);
            team->fighters[i].alive = strstr(line, "true") != NULL;
            if (team->fighters[i].alive) team->alive_count++;
            i++;
        }
    }
    fclose(f);
    strncpy(team->filename, filename, sizeof(team->filename)-1);
    team->filename[sizeof(team->filename)-1] = '\0';
    return team;
}

Team* load_team_by_number(int num) {
    char filename[256];
    snprintf(filename, sizeof(filename), "team%d.txt", num);
    return load_team_json(filename);
}

Team* select_or_create_team(int player_index) {
    int choice;
    printf("\n=== Настройка команды для игрока %d ===\n", player_index);
    printf("1. Выбрать существующую команду\n");
    printf("2. Создать новую команду\n");
    printf("Ваш выбор: ");
    scanf("%d", &choice);
    clear_input_buffer();

    if (choice == 1) {
        list_teams();
        printf("Введите номер команды из списка (или 0 для создания новой): ");
        int idx;
        scanf("%d", &idx);
        clear_input_buffer();
        if (idx > 0) {
            int n = 1, count = 0;
            char filename[256];
            while (1) {
                snprintf(filename, sizeof(filename), "%s/team%d.txt", TEAM_FOLDER, n);
                FILE *f = fopen(filename, "r");
                if (!f) break;
                count++;
                if (count == idx) {
                    fclose(f);
                    Team *team = load_team_by_number(n);
                    if (!validate_team_strength(team)) {
                        printf("Ошибка: Суммарная сила команды превышает лимит (%d).\n", MAX_TOTAL_STRENGTH);
                        break;
                    }
                    if (team) {
                        // Сброс состояния для новой игры: все бойцы живы, сила = base_strength
                        for (int i = 0; i < team->size; i++) {
                            team->fighters[i].alive = true;
                            team->fighters[i].strength = team->fighters[i].base_strength;
                        }
                        team->alive_count = team->size;

                        char redist;
                        printf("Хотите перераспределить очки силы? (y/n): ");
                        scanf(" %c", &redist);
                        clear_input_buffer();
                        if (redist == 'y' || redist == 'Y')
                            redistribute_strength(team);
                        return team;
                    }
                    break;
                }
                fclose(f);
                n++;
            }
        }
        printf("Переход к созданию новой команды.\n");
    }

    int num_fighters;
    printf("Введите количество бойцов: ");
    scanf("%d", &num_fighters);
    clear_input_buffer();
    Team *team = create_team(num_fighters);
    if (!team) return NULL;
    input_team(team);
    
    // Сохраняем в JSON с автоматическим именем
    int next_num = get_next_team_number();
    char filename[256];
    snprintf(filename, sizeof(filename), "team%d.txt", next_num);
    save_team_json(team, filename);
    strcpy(team->filename, filename);
    
    char redist;
    printf("Хотите перераспределить очки силы? (y/n): ");
    scanf(" %c", &redist);
    clear_input_buffer();
    if (redist == 'y' || redist == 'Y')
        redistribute_strength(team);
    return team;
}

// Создаёт пустую команду с заданным числом бойцов
Team* create_team(int num_fighters) {
    Team *team = (Team*)malloc(sizeof(Team));
    if (!team) return NULL;
    team->size = num_fighters;
    team->alive_count = num_fighters;
    team->fighters = (Fighter*)malloc(num_fighters * sizeof(Fighter));
    team->filename[0] = '\0';
    for (int i = 0; i < num_fighters; i++) {
        team->fighters[i].id = i;
        team->fighters[i].strength = 0;
        team->fighters[i].base_strength = 0;
        team->fighters[i].alive = true;
        team->fighters[i].kills = 0;
        strcpy(team->fighters[i].name, "");
    }
    return team;
}

// Освобождает память команды
void destroy_team(Team *team) {
    if (team) {
        free(team->fighters);
        free(team);
    }
}

// Ввод команды с консоли с распределением 100 очков силы
void input_team(Team *team) {
    input_string("Введите название команды: ", team->name, MAX_NAME_LEN);
    
    int remaining = MAX_TOTAL_STRENGTH;
    printf("У вас есть %d очков силы, которые нужно распределить между бойцами.\n", remaining);
    printf("Каждый боец должен получить хотя бы 1 очко.\n");
    
    for (int i = 0; i < team->size; i++) {
        int min_needed = team->size - i;
        int max_possible = remaining - min_needed;
        
        printf("Боец %d:\n", i + 1);
        char prompt[100];
        snprintf(prompt, sizeof(prompt), "  Имя: ");
        input_string(prompt, team->fighters[i].name, MAX_NAME_LEN);
        
        int strength;
        do {
            printf("  Сила (1..%d, осталось очков: %d): ", max_possible + 1, remaining);
            scanf("%d", &strength);
            clear_input_buffer();
            if (strength < 1 || strength > remaining - min_needed + 1) {
                printf("Ошибка: сила должна быть от 1 до %d (чтобы хватило на остальных бойцов).\n", remaining - min_needed + 1);
            } else if (strength > remaining) {
                printf("Не хватает очков. Доступно только %d.\n", remaining);
            } else {
                break;
            }
        } while (1);
        
        team->fighters[i].strength = strength;
        team->fighters[i].base_strength = strength;
        team->fighters[i].alive = true;
        remaining -= strength;
    }
    if (remaining > 0) {
        printf("Осталось нераспределённых очков: %d. Добавим первому бойцу.\n", remaining);
        team->fighters[0].strength += remaining;
    }
}

// Перераспределение 100 очков силы между существующими бойцами
void redistribute_strength(Team *team) {
    printf("Перераспределение очков силы команды %s\n", team->name);
    int remaining = MAX_TOTAL_STRENGTH;
    printf("Всего очков: %d. Каждый боец должен получить хотя бы 1.\n", MAX_TOTAL_STRENGTH);
    
    for (int i = 0; i < team->size; i++) {
        team->fighters[i].strength = 0;
    }
    
    for (int i = 0; i < team->size; i++) {
        int min_needed = team->size - i;
        int max_possible = remaining - min_needed;
        int new_strength;
        
        printf("Боец %d (%s)\n", i + 1, team->fighters[i].name);
        do {
            printf("  Новая сила (1..%d, осталось очков: %d): ", max_possible + 1, remaining);
            scanf("%d", &new_strength);
            clear_input_buffer();
            if (new_strength < 1 || new_strength > remaining - min_needed + 1) {
                printf("Ошибка: сила должна быть от 1 до %d.\n", remaining - min_needed + 1);
            } else if (new_strength > remaining) {
                printf("Недостаточно очков. Доступно: %d.\n", remaining);
            } else {
                break;
            }
        } while (1);
        
        team->fighters[i].strength = new_strength;
        team->fighters[i].base_strength = new_strength;
        remaining -= new_strength;
    }
    if (remaining > 0) {
        printf("Осталось нераспределённых очков: %d. Добавим первому бойцу.\n", remaining);
        team->fighters[0].strength += remaining;
    }
}

// Выводит состояние команды на экран
void print_team(const Team *team) {
    printf("Команда %s:\n", team->name);
    for (int i = 0; i < team->size; i++) {
        if (team->fighters[i].alive) {
            printf("  %d. %s: сила %d\n", i + 1, team->fighters[i].name, team->fighters[i].strength);
        } else {
            printf("  %d. %s: погиб\n", i + 1, team->fighters[i].name);
        }
    }
    printf("Живых: %d\n", team->alive_count);
}

// Возвращает количество живых бойцов
int get_alive_count(const Team *team) {
    return team->alive_count;
}

// Возвращает индекс случайного живого бойца
int get_random_alive_fighter_index(const Team *team) {
    if (team->alive_count == 0) return -1;
    int random_index = random_range(0, team->alive_count - 1);
    int count = 0;
    for (int i = 0; i < team->size; i++) {
        if (team->fighters[i].alive) {
            if (count == random_index) return i;
            count++;
        }
    }
    return -1;
}

// Возвращает указатель на бойца по индексу
Fighter* get_fighter(const Team *team, int index) {
    if (index < 0 || index >= team->size) return NULL;
    return &team->fighters[index];
}

// Помечает бойца как погибшего
void kill_fighter(Team *team, int index) {
    if (index >= 0 && index < team->size && team->fighters[index].alive) {
        team->fighters[index].alive = false;
        team->alive_count--;
    }
}

// Изменяет силу бойца (delta может быть отрицательной). Если сила <= 0, боец погибает
void change_strength(Team *team, int index, int delta) {
    if (index >= 0 && index < team->size && team->fighters[index].alive) {
        team->fighters[index].strength += delta;
        if (team->fighters[index].strength <= 0) {
            team->fighters[index].strength = 0;
            kill_fighter(team, index);
        }
    }
}

// Безопасный ввод строки с консоли (позволяет пробелы)
void input_string(const char *prompt, char *buffer, int size) {
    printf("%s", prompt);
    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
        return;
    }
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n')
        buffer[len-1] = '\0';
}

// Очищает входной буфер после scanf (убирает символ новой строки)
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Самый сильный живой
int get_best_attacker_index(const Team *team) {
    if (!team || team->size <= 0) return -1;

    int best_idx = -1;
    int best_val = -1;

    for (int i = 0; i < team->size; i++) {
        const Fighter *f = &team->fighters[i];
        if (f->alive && f->strength > best_val) {
            best_val = f->strength;
            best_idx = i;
        }
    }
    return best_idx;
}

// Самый слабый живой
int get_weakest_alive_index(const Team *team) {
    if (!team || team->size <= 0) return -1;

    int weakest_idx = -1;
    int weakest_val = INT_MAX;

    for (int i = 0; i < team->size; i++) {
        const Fighter *f = &team->fighters[i];
        if (f->alive && f->strength < weakest_val) {
            weakest_val = f->strength;
            weakest_idx = i;
        }
    }
    return weakest_idx;
}

static void write_string(FILE *f, const char *s) {
    uint16_t len = (uint16_t)strlen(s);
    fwrite(&len, sizeof(len), 1, f);
    if (len > 0) fwrite(s, 1, len, f);
}

// Чтение строки: читаем длину, потом символы
static void read_string(FILE *f, char *buf, int bufsize) {
    uint16_t len;
    if (fread(&len, sizeof(len), 1, f) != 1) {
        buf[0] = '\0';
        return;
    }
    if (len >= bufsize) len = bufsize - 1;
    fread(buf, 1, len, f);
    buf[len] = '\0';
}

void save_team_to_stream(const Team *team, FILE *f) {
    write_string(f, team->name);
    fwrite(&team->size, sizeof(team->size), 1, f);
    fwrite(&team->alive_count, sizeof(team->alive_count), 1, f);
    for (int i = 0; i < team->size; i++) {
        const Fighter *p = &team->fighters[i];
        write_string(f, p->name);
        fwrite(&p->strength, sizeof(p->strength), 1, f);
        fwrite(&p->kills, sizeof(p->kills), 1, f);
        uint8_t alive = p->alive ? 1 : 0;
        fwrite(&alive, sizeof(alive), 1, f);
    }
}

Team* load_team_from_stream(FILE *f) {
    Team *team = (Team*)malloc(sizeof(Team));
    if (!team) return NULL;

    read_string(f, team->name, MAX_NAME_LEN);
    if (fread(&team->size, sizeof(team->size), 1, f) != 1) {
        free(team);
        return NULL;
    }
    if (fread(&team->alive_count, sizeof(team->alive_count), 1, f) != 1) {
        free(team);
        return NULL;
    }
    team->fighters = (Fighter*)malloc(team->size * sizeof(Fighter));
    for (int i = 0; i < team->size; i++) {
        Fighter *p = &team->fighters[i];
        p->id = i;
        read_string(f, p->name, MAX_NAME_LEN);
        fread(&p->strength, sizeof(p->strength), 1, f);
        fread(&p->kills, sizeof(p->kills), 1, f);
        uint8_t alive;
        fread(&alive, sizeof(alive), 1, f);
        p->alive = alive ? true : false;
    }
    team->filename[0] = '\0';  // не сохраняем имя файла в стриме
    return team;
}

int validate_team_strength(const Team *team) {
    if (!team) return 0;
    int total = 0;
    for (int i = 0; i < team->size; i++) {
        total += team->fighters[i].base_strength;
    }
    return total <= MAX_TOTAL_STRENGTH;
}

int select_fighter_menu(const Team *team, const char *role_marker) {
    if (!team || team->size == 0) return -1;
    int current = 0;
    // перейти к первому живому
    while (current < team->size && !team->fighters[current].alive) current++;
    if (current == team->size) return -1;

    while (1) {
        clear_screen();
        printf("=== %s ===\n", role_marker);
        printf("Выберите бойца (стрелки, Enter, 0-выход):\n");
        for (int i = 0; i < team->size; i++) {
            const Fighter *f = &team->fighters[i];
            if (!f->alive) continue;
            if (i == current)
                printf(" > %d. %s (сила %d) %s\n", i+1, f->name, f->strength, role_marker);
            else
                printf("   %d. %s (сила %d)\n", i+1, f->name, f->strength);
        }
        printf("   0. Выход\n");

        int key = _getch();
        if (key == 0 || key == 224) {       // стрелка
            key = _getch();
            if (key == 72) {                // вверх
                int prev = current - 1;
                while (prev >= 0 && !team->fighters[prev].alive) prev--;
                if (prev >= 0) current = prev;
            } else if (key == 80) {         // вниз
                int next = current + 1;
                while (next < team->size && !team->fighters[next].alive) next++;
                if (next < team->size) current = next;
            }
        } else if (key == 13) {             // Enter
            if (team->fighters[current].alive) {
                printf("Выбран боец: %s\n", team->fighters[current].name);
                return current;
            }
        } else if (key == '0') {
            return -1;                      // выход
        }
    }
}