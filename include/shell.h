#ifndef SHELL_H
#define SHELL_H

/*
 * İşletim Sistemleri Proje Ödevi - 2024 Güz
 * Grup Üyeleri:
 *   - Süleyman Samet Kaya - B221210103 - suleyman.kaya3@ogr.sakarya.edu.tr
 *   - İbrahim Güldemir - B221210052 - ibrahim.guldemir@ogr.sakarya.edu.tr
 *   - İsmail Konak - G221210046 - ismail.konak1@ogr.sakarya.edu.tr
 *   - Tuğra Yavuz - B221210064 - tugra.yavuz@ogr.sakarya.edu.tr
 *   - İsmail Alper Karadeniz - B221210065 - alper.karadeniz@ogr.sakarya.edu.tr
 *
 * Dosya: shell.h
 * Açıklama: Bu dosya, basit bir Linux kabuğunun C dilinde implementasyonunu içerir.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARG_COUNT      100
#define MAX_ARKA_PLAN_SUREC 10

/* Arka planda çalışan süreçlerin bilgilerini tutmak için yapı */
typedef struct {
    pid_t pid;
    char komut[MAX_COMMAND_LENGTH];
} ArkaPlanSurec;

/* Fonksiyon tanımları */
void handle_sigchld(int sig);
void print_prompt();
void execute_single_command(char *cmdLine);
void execute_command_with_input_redirection(char *cmd);
void execute_command_with_output_redirection(char **args, char *output_file, int append_flag);
void parse_and_execute_output_redirection(char *input);
void arka_plan_surec_ekle(pid_t pid, const char* komut);
void arka_plan_surec_kaldir(pid_t pid);
void arka_plan_surecleri_kontrol();
void execute_with_pipe(char *input_buffer);
void list_jobs();
void increment_command();
void execute_multiple_commands(char *input_buffer);

#endif