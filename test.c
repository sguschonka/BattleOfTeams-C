#include "test.h"
#include "team.h"
#include "battle.h"
#include "random.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>

static FILE *log_file = NULL;

static void log_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    if (log_file) {
        vfprintf(log_file, fmt, args);
    }
    va_end(args);
}

static void open_log() {
    log_file = fopen("log.txt", "a");
    if (!log_file) {
        // не выводим ошибку в консоль, чтобы не мешать игре
    }
}

static void close_log() {
    if (log_file) fclose(log_file);
}

static void assert_test(int condition, const char *test_name, const char *file, int line) {
    if (condition) {
        log_printf("[OK] %s (файл %s, строка %d)\n", test_name, file, line);
    } else {
        log_printf("[FAILED] %s (файл %s, строка %d)\n", test_name, file, line);
    }
}

static void test_create_team() {
    log_printf("\n=== Тестирование create_team ===\n");
    Team *t = create_team(5);
    assert_test(t != NULL, "create_team возвращает не NULL", __FILE__, __LINE__);
    assert_test(t->size == 5, "Размер команды = 5", __FILE__, __LINE__);
    assert_test(t->alive_count == 5, "alive_count = 5", __FILE__, __LINE__);
    assert_test(t->fighters != NULL, "Массив бойцов создан", __FILE__, __LINE__);
    destroy_team(t);
    Team *t0 = create_team(0);
    assert_test(t0 != NULL, "create_team(0) не падает", __FILE__, __LINE__);
    destroy_team(t0);
}

static void test_change_strength() {
    log_printf("\n=== Тестирование change_strength ===\n");
    Team *t = create_team(1);
    t->fighters[0].strength = 5;
    t->fighters[0].alive = true;
    change_strength(t, 0, 3);
    assert_test(t->fighters[0].strength == 8, "Увеличение силы на 3", __FILE__, __LINE__);
    assert_test(t->alive_count == 1, "Боец жив", __FILE__, __LINE__);
    change_strength(t, 0, -8);
    assert_test(t->fighters[0].strength == 0, "Сила стала 0", __FILE__, __LINE__);
    assert_test(t->fighters[0].alive == false, "Боец погиб", __FILE__, __LINE__);
    assert_test(t->alive_count == 0, "alive_count уменьшился", __FILE__, __LINE__);
    change_strength(t, 0, 10);
    assert_test(t->fighters[0].strength == 0, "Сила не изменилась", __FILE__, __LINE__);
    destroy_team(t);
}

static void test_choose_target() {
    log_printf("\n=== Тестирование выбора цели (бот атакует слабейшего) ===\n");
    Team *att = create_team(2);
    Team *def = create_team(2);
    att->fighters[0].strength = 10; att->fighters[0].alive = true;
    att->fighters[1].strength = 5;  att->fighters[1].alive = true;
    def->fighters[0].strength = 8;  def->fighters[0].alive = true;
    def->fighters[1].strength = 2;  def->fighters[1].alive = true;
    int weakest = get_weakest_alive_index(def);
    assert_test(weakest == 1, "Слабейший боец имеет индекс 1", __FILE__, __LINE__);
    destroy_team(att);
    destroy_team(def);
}

void run_all_tests() {
    open_log();
    log_printf("\n========== ЗАПУСК ТЕСТОВ %s ==========\n", __DATE__);
    srand(1);
    test_create_team();
    test_change_strength();
    test_choose_target();
    log_printf("\n========== ТЕСТЫ ЗАВЕРШЕНЫ ==========\n");
    close_log();
}