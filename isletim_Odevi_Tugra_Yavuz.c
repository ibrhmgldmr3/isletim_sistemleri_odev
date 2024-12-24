#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARG_COUNT      100

/* Fonksiyon Prototipleri */
void handle_sigchld(int sig);
void print_prompt();
void execute_single_command(char *cmdLine);

/* Bu değişkenler, arka plan süreç sayısını ve quit komutunun girilip girilmediğini izlemek için */
volatile sig_atomic_t backgroundCount = 0; 
volatile sig_atomic_t quitRequested   = 0;

/* 
   SIGCHLD sinyali geldiğinde (arka planda bir child öldüğünde), 
   waitpid ile ölenleri toplayıp ekrana bilgi yazıyoruz.
*/
void handle_sigchld(int sig) {
    (void)sig; // Kullanılmadı uyarısını engellemek için
    int status;
    pid_t pid;

    /* WNOHANG ile dönen tüm child'ları bekle; 
       her ölen arka plan sürecinde counter'ı azalt */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        backgroundCount--;
        printf("[PID %d] terminated with exit code %d\n", pid, WEXITSTATUS(status));
        fflush(stdout);
        print_prompt(); // Arka plan süreci tamamlandığında prompt yazdır
    }
}

/* Ekrana prompt basma fonksiyonu */
void print_prompt() {
    printf("> ");
    fflush(stdout);
}

/* Tekli komutu ön/arka plan olarak çalıştırır */
void execute_single_command(char *cmdLine) {
    char *args[MAX_ARG_COUNT];
    int arg_count = 0;
    int background = 0;

    /* Komut satırının sonundaki '\n' karakterini temizle */
    size_t len = strlen(cmdLine);
    if (len > 0 && cmdLine[len - 1] == '\n') {
        cmdLine[len - 1] = '\0';
        len--;
    }

    /* 
       Eğer son karakter '&' ise arka plan isteği var. 
       '&' karakterini ve gerekirse son boşlukları kaldır.
    */
    if (len > 0 && cmdLine[len - 1] == '&') {
        background = 1;
        cmdLine[len - 1] = '\0';
        len--;
        /* Sondaki boşlukları da silelim */
        while (len > 0 && (cmdLine[len - 1] == ' ' || cmdLine[len - 1] == '\t')) {
            cmdLine[--len] = '\0';
        }
    }

    /* Boşluklara göre tokenize et */
    char *token = strtok(cmdLine, " \t");
    while (token != NULL && arg_count < MAX_ARG_COUNT - 1) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t");
    }
    args[arg_count] = NULL; // Argümanları NULL ile sonlandır

    /* Boş komut girildiyse hiçbir şey yapma */
    if (arg_count == 0) {
        return;
    }

    /* fork ile child süreci oluştur */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork hatası");
        return;
    } 
    else if (pid == 0) {
        /* Çocuk süreç: komutu exec ile çalıştır */
        if (execvp(args[0], args) == -1) {
            perror("execvp hatası");
            exit(EXIT_FAILURE);
        }
        /* execvp başarılı olursa buraya dönmez */
    } 
    else {
        /* Ebeveyn süreç */
        if (!background) {
            /* Ön planda çalıştırma: child bitene kadar bekle */
            int status;
            waitpid(pid, &status, 0);
        } 
        else {
            /* Arka plan çalıştırma: child'ı beklemeden devam et, sayıcıyı artır */
            backgroundCount++;
            printf("[PID %d] running in background\n", pid);
            fflush(stdout);
            print_prompt(); // Arka plan süreci başlatıldığında prompt yazdır
        }
    }
}

int main() {
    /* SIGCHLD sinyalini handle_sigchld fonksiyonuna yönlendir */
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; // İsteğe bağlı: kesintili çağrıların yeniden başlamasını sağlar
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction hatası");
        exit(EXIT_FAILURE);
    }

    char input[MAX_COMMAND_LENGTH];

    while (1) {
        /* 1) Prompt yazdırma */
        print_prompt();

        /* Kullanıcı girişini oku */
        if (!fgets(input, sizeof(input), stdin)) {
            /* EOF veya hata */
            if (feof(stdin)) {
                /* Kullanıcı Ctrl+D yapmış olabilir, shell'i kapatıyoruz */
                break;
            }
            continue;
        }

        /* Satır boş mu kontrol et */
        if (strcmp(input, "\n") == 0) {
            /* Sadece Enter basılmışsa tekrar döngü başına dön */
            continue;
        }

        /* 2) Quit komutu mu? */
        /* Daha güvenli olsun diye 'quit\n' veya 'quit ' vb. durumları da handle edebiliriz */
        if (strncmp(input, "quit", 4) == 0) {
            /* 
               Eğer arka plan süreci çalışıyorsa, 
               yeni komutlar alınmayacak, hepsinin bitmesi beklenecek.
            */
            quitRequested = 1;
            break; 
        }

        /* 3) Tekli komutları çalıştır */
        execute_single_command(input);
    }

    /* Eğer 'quit' girildiyse, arka plan süreçleri varsa bitmelerini bekle */
    if (quitRequested) {
        /* Arka planda çalışan bir süreç varsa, 
           yeni komutlar alınmayacak, burada bekliyoruz. */
        if (backgroundCount > 0) {
            printf("Shell is waiting for %d background process(es) to finish...\n", backgroundCount);
            fflush(stdout);
        }

        /* Arka plan süreçleri bitene kadar bekle */
        while (backgroundCount > 0) {
            /* pause() -> bir sinyal bekler. 
               Child bittiğinde SIGCHLD gelir, handler backgroundCount-- yapar. */
            pause();
        }
        printf("All background processes have finished. Exiting shell...\n");
    }

    return 0;
}
