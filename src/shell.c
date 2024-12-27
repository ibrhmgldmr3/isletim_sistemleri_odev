/*
 * İşletim Sistemleri Proje Ödevi - 2024 Güz
 * Grup Üyeleri:
 *   - Süleyman Samet Kaya - B221210103 - suleyman.kaya3@ogr.sakarya.edu.tr
 *   - İbrahim Güldemir - B221210052 - ibrahim.guldemir@ogr.sakarya.edu.tr
 *   - İsmail Konak - G221210046 - ismail.konak1@ogr.sakarya.edu.tr
 *   - Tuğra Yavuz - B221210064 - tugra.yavuz@ogr.sakarya.edu.tr
 *   - İsmail Alper Karadeniz - B221210065 - alper.karadeniz@ogr.sakarya.edu.tr
 *
 * Dosya: kabuk.c
 * Açıklama: Bu dosya, basit bir Linux kabuğunun C dilinde implementasyonunu içerir.
 */

#include "shell.h"

/* Bu değişkenler, arka plan süreç sayısını ve quit komutunun girilip girilmediğini izlemek için */
volatile sig_atomic_t backgroundCount = 0;
volatile sig_atomic_t quitRequested = 0;

ArkaPlanSurec arka_plan_surecleri[MAX_ARKA_PLAN_SUREC];
int arka_plan_surec_sayisi = 0;

/* SIGCHLD sinyali geldiğinde çalışan sinyal işleyici */
void handle_sigchld(int sig) {
    (void)sig;
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        backgroundCount--;
        arka_plan_surec_kaldir(pid);
        printf("[PID %d] terminated with exit code %d\n", pid, WEXITSTATUS(status));
        fflush(stdout);
    }
}

/* Ekrana prompt basma fonksiyonu */
void print_prompt() {
    printf("> ");
    fflush(stdout);
}

/* jobs komutunu işleyen fonksiyon */
void list_jobs() {
    arka_plan_surecleri_kontrol();
    for (int i = 0; i < arka_plan_surec_sayisi; i++) {
        printf("[%d] %d %s\n", i + 1, arka_plan_surecleri[i].pid, arka_plan_surecleri[i].komut);
    }
}

/* "increment" komutunu işleyen fonksiyon */
void increment_command() {
    int num;
    if (scanf("%d", &num) == 1) {
        printf("%d\n", num + 1);
    } else {
        printf("increment: Geçersiz giriş.\n");
    }
    fflush(stdin);
}

/* Çıkış yönlendirmesi (>, >>) içeren komutları işler */
void execute_command_with_output_redirection(char **args, char *output_file, int append_flag) {
    pid_t pid = fork();
    if (pid == 0) {
        // Child process

        // Dosyayı aç (yazma modunda, oluştur)
        int fd;
        if (append_flag) {
            // Ekleme modunda aç
            fd = open(output_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
        } else {
            // Üzerine yazma modunda aç
            fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }
        
        if (fd < 0) {
            perror("Failed to open file");
            exit(EXIT_FAILURE);
        }

        // Standart çıktıyı dosyaya yönlendir
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            close(fd);
            exit(EXIT_FAILURE);
        }
        close(fd);

        // Komutu çalıştır
        execvp(args[0], args);
        perror("Execution failed");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Parent process
        wait(NULL);
    } else {
        perror("Fork failed");
    }
}

/* Komutu parse edip, çıkış yönlendirmesi varsa işler */
void parse_and_execute_output_redirection(char *input) {
    char *output_file;
    int append_flag = 0;

    // ">>" kontrolü
    char *append_ptr = strstr(input, ">>");
    if (append_ptr != NULL) {
        output_file = append_ptr + 2;
        append_flag = 1;
        *append_ptr = '\0'; // Komut kısmını ayır
    } else {
        // ">" kontrolü
        char *redirect_ptr = strstr(input, ">");
        if (redirect_ptr != NULL) {
            output_file = redirect_ptr + 1;
            *redirect_ptr = '\0'; // Komut kısmını ayır
        } else {
            // Yönlendirme yok
            printf("Invalid command format. Use: command > output_file or command >> output_file\n");
            return;
        }
    }

    // Boşlukları temizle (komut kısmı için)
    size_t len = strlen(input);
    while (len > 0 && (input[len - 1] == ' ' || input[len - 1] == '\t' || input[len - 1] == '\n')) {
        input[--len] = '\0';
    }

    // Boşlukları temizle (dosya adı için)
    while (*output_file == ' ') output_file++;
    len = strlen(output_file);
    while (len > 0 && (output_file[len - 1] == ' ' || output_file[len - 1] == '\t' || output_file[len - 1] == '\n')) {
        output_file[--len] = '\0';
    }

    char *args[128];
    int i = 0;
    char *token = strtok(input, " ");
    while (token != NULL && i < 128 - 1) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    execute_command_with_output_redirection(args, output_file, append_flag);
}

/* Pipe'ları işleyen fonksiyon */
void execute_with_pipe(char *input_buffer) {
    int pipe_count = 0;
    for (char *p = input_buffer; *p; p++) {
        if (*p == '|') {
            pipe_count++;
        }
    }

    int num_commands = pipe_count + 1;
    char *commands[num_commands];
    int i = 0;

    // Komutları ayır
    commands[i] = strtok(input_buffer, "|");
    while (commands[i] != NULL && i < num_commands - 1) {
        i++;
        commands[i] = strtok(NULL, "|");
    }

    int pipes[pipe_count][2];
    pid_t pids[num_commands];

    // Pipe'ları oluştur
    for (i = 0; i < pipe_count; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < num_commands; i++) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pids[i] == 0) {
            // Child process

            // "echo" komutu için özel işlem
            if (strncmp(commands[i], "echo ", 5) == 0) {
                char *arg = commands[i] + 5;
                while (*arg == ' ') arg++;  // Boşlukları atla

                // Sonraki komuta yönlendir
                if (i < num_commands - 1) {
                    if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                        perror("dup2 stdout");
                        exit(EXIT_FAILURE);
                    }
                }

                // *** DEĞİŞİKLİK BURADA: write ile pipe'a yaz ***
                char newline = '\n';
                write(STDOUT_FILENO, arg, strlen(arg));
                write(STDOUT_FILENO, &newline, 1); // Yeni satır ekle

                // Tüm pipe'ları kapat
                for (int j = 0; j < pipe_count; j++) {
                    close(pipes[j][0]);
                    close(pipes[j][1]);
                }

                exit(EXIT_SUCCESS);
            }

            // Standart girdiyi yönlendir (ilk komut hariç)
            if (i > 0) {
                if (dup2(pipes[i - 1][0], STDIN_FILENO) == -1) {
                    perror("dup2 stdin");
                    exit(EXIT_FAILURE);
                }
            }

            // Standart çıktıyı yönlendir (son komut hariç)
            if (i < num_commands - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2 stdout");
                    exit(EXIT_FAILURE);
                }
            }

            // Tüm pipe'ları kapat
            for (int j = 0; j < pipe_count; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Komutu argümanlarına ayır
            char *args[MAX_ARG_COUNT];
            int j = 0;
            char *token = strtok(commands[i], " \t\n"); // Boşluk, tab ve yeni satır ile ayır
            while (token != NULL && j < MAX_ARG_COUNT - 1) {
                args[j++] = token;
                token = strtok(NULL, " \t\n");
            }
            args[j] = NULL;

            // Komutu çalıştır
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process

    // Tüm pipe'ları kapat
    for (i = 0; i < pipe_count; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Tüm child process'lerin bitmesini bekle
    for (i = 0; i < num_commands; i++) {
        wait(NULL);
    }
}

/* Tekli komutu ön/arka plan olarak çalıştırır */
void execute_single_command(char *cmdLine) {
    char *args[MAX_ARG_COUNT];
    int arg_count = 0;
    int background = 0;
    char cmdLineCopy[MAX_COMMAND_LENGTH]; // jobs için - orijinal komut satırını kopyala

    /* Komut satırının sonundaki '\n' karakterini temizle */
    size_t len = strlen(cmdLine);
    if (len > 0 && cmdLine[len - 1] == '\n') {
        cmdLine[len - 1] = '\0';
        len--;
    }

    /* jobs için - orijinal komut satırını kopyala */
    strncpy(cmdLineCopy, cmdLine, MAX_COMMAND_LENGTH - 1);
    cmdLineCopy[MAX_COMMAND_LENGTH - 1] = '\0';

    /*
       Eğer son karakter '&' ise ve öncesinde boşluk varsa arka plan isteği var.
       '&' karakterini ve gerekirse son boşlukları kaldır.
    */
    char *bg_ptr = strstr(cmdLine, " &");
    if (bg_ptr != NULL && *(bg_ptr + 2) == '\0') {
        background = 1;
        *bg_ptr = '\0'; // '&' ve öncesindeki boşluğu kaldır
        len = bg_ptr - cmdLine;
    }

    /* Boşluklara göre tokenize et */
    char *token = strtok(cmdLine, " \t");
    while (token != NULL && arg_count < MAX_ARG_COUNT - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL;

    /* Boş komut girildiyse hiçbir şey yapma */
    if (arg_count == 0) {
        return;
    }

    /* Built-in komutlar kontrol ediliyor */
    if (strcmp(args[0], "quit") == 0) {
      if (backgroundCount > 0) {
          printf("Arka planda çalışan süreçler var. Onların bitmesini bekleyin ya da hepsini öldürün.\n");
          return;
      } else {
          exit(0);
      }
    }

    if (strcmp(args[0], "jobs") == 0) {
        list_jobs();
        return;
    }

    if (strcmp(args[0], "increment") == 0) {
        increment_command();
        return;
    }

    /* fork ile child süreci oluştur */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork hatası");
        return;
    } else if (pid == 0) {
        /* Çocuk süreç: komutu exec ile çalıştır */
        if (execvp(args[0], args) == -1) {
            perror("execvp hatası");
            exit(EXIT_FAILURE);
        }
    } else {
        /* Ebeveyn süreç */
        if (!background) {
            /* Ön planda çalıştırma: child bitene kadar bekle */
            int status;
            waitpid(pid, &status, 0);
        } else {
            /* Arka plan çalıştırma: child'ı beklemeden devam et, sayıcıyı artır, jobs listesine ekle */
            backgroundCount++;
            arka_plan_surec_ekle(pid, cmdLineCopy);
            printf("[PID %d] running in background\n", pid);
            fflush(stdout);
        }
    }
}

/* Giriş yönlendirmesi (<) içeren komutları işler */
void execute_command_with_input_redirection(char *cmd) {
    char *args[MAX_ARG_COUNT];
    char *input_file;
    char *command;
    int arg_count = 0;
    char cmd_copy[MAX_COMMAND_LENGTH];

    // Orijinal komut satırını kopyala, strtok değişiklik yapıyor
    strncpy(cmd_copy, cmd, MAX_COMMAND_LENGTH);
    cmd_copy[MAX_COMMAND_LENGTH - 1] = '\0';

    // Komut ve dosya adını ayır
    command = strtok(cmd_copy, "<");
    input_file = strtok(NULL, "<");

    // Boşlukları temizle
    if (command != NULL) {
        while (*command == ' ') command++;
        size_t len = strlen(command);
        while (len > 0 && (command[len - 1] == ' ' || command[len - 1] == '\t' || command[len - 1] == '\n')) {
            command[--len] = '\0';
        }
    }

    if (input_file != NULL) {
        while (*input_file == ' ') input_file++;
        size_t len = strlen(input_file);
        while (len > 0 && (input_file[len - 1] == ' ' || input_file[len - 1] == '\t' || input_file[len - 1] == '\n')) {
            input_file[--len] = '\0';
        }
    }

    if (input_file == NULL) {
        printf("Giriş dosyası belirtilmemiş.\n");
        return;
    }

    // Komutu argümanlarına ayır
    char *token = strtok(command, " \t");
    while (token != NULL && arg_count < MAX_ARG_COUNT - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL;
    
    if (strcmp(args[0], "increment") == 0) {
        // Dosyayı aç
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            printf("Giriş dosyası bulunamadı.\n");
            return;
        }
        
        // Orijinal standart girdi dosya tanımlayıcısını sakla
        int stdin_copy = dup(STDIN_FILENO);

        // Standart girdiyi (STDIN) dosyaya yönlendir
        if (dup2(fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(fd);
            close(stdin_copy);
            exit(EXIT_FAILURE);
        }
        close(fd);
        increment_command();

        // Standart girdiyi tekrar terminale yönlendir
        dup2(stdin_copy, STDIN_FILENO);
        close(stdin_copy);

        // Standart girdi hata ve EOF durumunu temizle
        clearerr(stdin);
        
    } else {
        // Dosyayı aç
        int fd = open(input_file, O_RDONLY);
        if (fd == -1) {
            printf("Giriş dosyası bulunamadı.\n");
            return;
        }

        // Child process oluştur
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            close(fd);
            return;
        } else if (pid == 0) {
            // Child process'te standart girdiyi dosyaya yönlendir
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2");
                close(fd);
                exit(EXIT_FAILURE);
            }
            close(fd);

            // Komutu çalıştır
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            // Parent process'te bekle
            int status;
            waitpid(pid, &status, 0);
            close(fd);
        }
    }
}

/* Yeni bir arka plan sürecini listeye ekleyen fonksiyon */
void arka_plan_surec_ekle(pid_t pid, const char* komut) {
    if (arka_plan_surec_sayisi < MAX_ARKA_PLAN_SUREC) {
        arka_plan_surecleri[arka_plan_surec_sayisi].pid = pid;
        strncpy(arka_plan_surecleri[arka_plan_surec_sayisi].komut, komut, MAX_COMMAND_LENGTH - 1);
        arka_plan_surecleri[arka_plan_surec_sayisi].komut[MAX_COMMAND_LENGTH - 1] = '\0';
        arka_plan_surec_sayisi++;
    } else {
        fprintf(stderr, "Çok fazla arka plan süreci var!\n");
    }
}

/* PID'i verilen bir arka plan sürecini listeden kaldıran fonksiyon */
void arka_plan_surec_kaldir(pid_t pid) {
    for (int i = 0; i < arka_plan_surec_sayisi; i++) {
        if (arka_plan_surecleri[i].pid == pid) {
            for (int j = i; j < arka_plan_surec_sayisi - 1; j++) {
                arka_plan_surecleri[j] = arka_plan_surecleri[j + 1];
            }
            arka_plan_surec_sayisi--;
            break;
        }
    }
}

/* Biten arka plan süreçlerini kontrol eden ve kullanıcıya bilgi veren fonksiyon */
void arka_plan_surecleri_kontrol() {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int exit_status = WEXITSTATUS(status);
        printf("[%d] tamamlandı (retval: %d)\n", pid, exit_status);
        arka_plan_surec_kaldir(pid);
    }
}

/* Noktalı virgül (;) ile ayrılmış birden fazla komutu çalıştıran fonksiyon */
void execute_multiple_commands(char *input_buffer) {
    char *command;
    pid_t pid;
    int status;

    // Komutları ayır
    command = strtok(input_buffer, ";");
    while (command != NULL) {
        // Her bir komut için yeni bir child process oluştur
        pid = fork();

        if (pid == 0) {
            // Child process:

            // Komutu işle (pipe, yönlendirme veya tek komut)
            if (strchr(command, '|') != NULL) {
                execute_with_pipe(command);
            } else if (strchr(command, '<') != NULL) {
                execute_command_with_input_redirection(command);
            } else if (strstr(command, ">>") != NULL || strchr(command, '>') != NULL) {
                parse_and_execute_output_redirection(command);
            } else {
                execute_single_command(command);
            }
            exit(0); // Child process'in işi bitti, çıkış yap
        } else if (pid > 0) {
            // Parent process: Child process'in bitmesini bekle
            waitpid(pid, &status, 0);
        } else {
            // Fork hatası
            perror("fork");
            return;
        }

        command = strtok(NULL, ";");
    }
}

int main() {
    char input_buffer[MAX_COMMAND_LENGTH];
    struct sigaction sa;

    /* SIGCHLD sinyali için işleyici ayarla */
    sa.sa_handler = &handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction hatası");
        exit(1);
    }

    while (1) {
        print_prompt();

        /* Kullanıcıdan komut satırını oku */
        if (fgets(input_buffer, MAX_COMMAND_LENGTH, stdin) == NULL) {
            if (errno == EINTR) {
                continue; // Sinyal kesmesi, girdiyi tekrar al
            } else {
                break; // EOF veya başka bir hata, döngüden çık
            }
        }

        /* Noktalı virgül (;) ile ayrılmış birden fazla komut varsa */
        if (strchr(input_buffer, ';') != NULL) {
            execute_multiple_commands(input_buffer);
        }
        /* Tek bir komut satırı varsa (pipe, yönlendirme veya tek komut) */
        else {
            /* Komut satırında pipe varsa */
            if (strchr(input_buffer, '|') != NULL) {
                execute_with_pipe(input_buffer);
            } 
            /* Komut satırında giriş yönlendirme varsa */
            else if (strchr(input_buffer, '<') != NULL) {
                execute_command_with_input_redirection(input_buffer);
            }
            /* Komut satırında çıkış yönlendirme varsa */
            else if (strstr(input_buffer, ">>") != NULL || strchr(input_buffer, '>') != NULL) {
                parse_and_execute_output_redirection(input_buffer);
            }
            /* Tek bir komut varsa */
            else {
                execute_single_command(input_buffer);
            }
        }
    }

    return 0;
}