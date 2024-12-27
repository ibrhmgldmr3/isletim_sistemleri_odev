#include <stdio.h>      // Standart girdi/çıktı işlemleri için (printf, fprintf, fgets vb.)
#include <stdlib.h>     // Genel amaçlı fonksiyonlar (malloc, free, exit vb.)
#include <unistd.h>     // İşletim sistemi API'lerine erişim (fork, execvp, waitpid vb.)
#include <string.h>     // String manipülasyon fonksiyonları (strcpy, strcmp, strtok vb.)
#include <sys/wait.h>   // Süreç bekleme fonksiyonları (wait, waitpid)
#include <stdbool.h>    // bool veri tipi için

#define MAX_KOMUT_UZUNLUK 1024 // Bir komut satırının maksimum uzunluğu
#define MAX_ARGUMAN 64      // Bir komutta bulunabilecek maksimum argüman sayısı
#define MAX_ARKA_PLAN_SUREC 10 // Aynı anda çalışabilecek maksimum arka plan süreç sayısı (örnek sınır)

// Arka planda çalışan süreçlerin bilgilerini tutmak için yapı (struct) tanımlıyoruz
typedef struct {
    pid_t pid;                     // Sürecin işlem kimliği (process ID)
    char komut[MAX_KOMUT_UZUNLUK]; // Arka planda çalışan komutun kendisi
} ArkaPlanSurec;

// Global olarak arka plan süreçlerini takip etmek için bir dizi ve sayaç tanımlıyoruz
ArkaPlanSurec arka_plan_surecleri[MAX_ARKA_PLAN_SUREC];
int arka_plan_surec_sayisi = 0;

// Yeni bir arka plan sürecini listeye ekleyen fonksiyon
void arka_plan_surec_ekle(pid_t pid, const char* komut) {
    // Önce listenin dolu olup olmadığını kontrol ediyoruz
    if (arka_plan_surec_sayisi < MAX_ARKA_PLAN_SUREC) {
        // Yeni sürecin bilgilerini listeye ekliyoruz
        arka_plan_surecleri[arka_plan_surec_sayisi].pid = pid;
        // Komutu güvenli bir şekilde kopyalıyoruz (buffer overflow'u önlemek için)
        strncpy(arka_plan_surecleri[arka_plan_surec_sayisi].komut, komut, MAX_KOMUT_UZUNLUK - 1);
        arka_plan_surecleri[arka_plan_surec_sayisi].komut[MAX_KOMUT_UZUNLUK - 1] = '\0'; // Son karakteri null ile sonlandırıyoruz
        arka_plan_surec_sayisi++; // Sayaçı bir arttırıyoruz
    } else {
        // Eğer liste doluysa kullanıcıya hata mesajı gösteriyoruz
        fprintf(stderr, "Çok fazla arka plan süreci var!\n");
    }
}

// PID'i verilen bir arka plan sürecini listeden kaldıran fonksiyon
void arka_plan_surec_kaldir(pid_t pid) {
    // Listedeki tüm süreçleri kontrol ediyoruz
    for (int i = 0; i < arka_plan_surec_sayisi; i++) {
        // Eğer aradığımız PID'yi bulursak
        if (arka_plan_surecleri[i].pid == pid) {
            // Bulunan süreci listeden silmek için sonraki elemanları sola kaydırıyoruz
            for (int j = i; j < arka_plan_surec_sayisi - 1; j++) {
                arka_plan_surecleri[j] = arka_plan_surecleri[j + 1];
            }
            arka_plan_surec_sayisi--; // Sayaçı bir azaltıyoruz
            break; // Süreci bulduğumuz için döngüden çıkıyoruz
        }
    }
}

// Biten arka plan süreçlerini kontrol eden ve kullanıcıya bilgi veren fonksiyon
void arka_plan_surecleri_kontrol() {
    int status; // Biten sürecin çıkış durumunu saklamak için
    pid_t pid;  // Biten sürecin PID'ini saklamak için

    // waitpid ile biten tüm çocuk süreçleri kontrol ediyoruz (bloklamadan)
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        int exit_status = WEXITSTATUS(status); // Sürecin çıkış kodunu alıyoruz
        printf("[%d] tamamlandı (retval: %d)\n", pid, exit_status); // Kullanıcıya bilgi veriyoruz
        arka_plan_surec_kaldir(pid); // Süreci listeden kaldırıyoruz
    }
}

int main() {
    char komut_satiri[MAX_KOMUT_UZUNLUK]; // Kullanıcının girdiği komut satırını saklamak için
    char *argumanlar[MAX_ARGUMAN];      // Komutun argümanlarını saklamak için bir dizi

    // Sonsuz bir döngüde kabuk işlemlerini gerçekleştiriyoruz
    while (true) {
        printf("> ");         // Komut istemini gösteriyoruz
        fflush(stdout);     // Çıktının hemen ekrana yazılmasını sağlıyoruz

        // Kullanıcının girişini okuyoruz
        if (fgets(komut_satiri, MAX_KOMUT_UZUNLUK, stdin) == NULL) {
            printf("\n"); // Ctrl+D ile çıkılması durumunda yeni bir satır yazdır
            break;       // Döngüden çıkıyoruz
        }

        // Komut satırının sonundaki newline karakterini kaldırıyoruz
        komut_satiri[strcspn(komut_satiri, "\n")] = 0;

        // Komut satırını boşluklara göre parçalara ayırıyoruz (tokenleştirme)
        char *token = strtok(komut_satiri, " ");
        int arguman_sayisi = 0; // Argüman sayısını takip etmek için
        bool arka_plan = false; // Komutun arka planda çalıştırılıp çalıştırılmayacağını belirlemek için

        // Ayrılan parçaları (token) argümanlar dizisine kaydediyoruz
        while (token != NULL && arguman_sayisi < MAX_ARGUMAN - 1) {
            argumanlar[arguman_sayisi++] = token;
            token = strtok(NULL, " "); // Sonraki boşluğa kadar olan kısmı alıyoruz
        }
        argumanlar[arguman_sayisi] = NULL; // Argüman listesinin sonunu NULL ile işaretliyoruz (execvp için gerekli)

        // Komutun arka planda çalıştırılmak istenip istenmediğini kontrol ediyoruz
        if (arguman_sayisi > 0 && strcmp(argumanlar[arguman_sayisi - 1], "&") == 0) {
            arka_plan = true;                             // Arka plan bayrağını işaretliyoruz
            argumanlar[arguman_sayisi - 1] = NULL;           // '&' işaretini argüman listesinden çıkarıyoruz
            arguman_sayisi--;                              // Argüman sayısını güncelliyoruz
        }

        // Boş komut girilip girilmediğini kontrol ediyoruz
        if (argumanlar[0] == NULL) {
            continue; // Döngünün başına dönüyoruz
        }

        // "quit" komutunu işliyoruz
        if (strcmp(argumanlar[0], "quit") == 0) {
            printf("Çıkılıyor...\n");
            // Arka planda çalışan tüm süreçlerin bitmesini bekliyoruz
            while (wait(NULL) > 0);
            break; // Döngüden çıkıyoruz
        }

        arka_plan_surecleri_kontrol(); // Yeni komut işlemeden önce biten arka plan süreçlerini kontrol ediyoruz

        // Yeni bir süreç oluşturuyoruz (fork)
        pid_t pid = fork();

        if (pid == 0) { // Çocuk süreç
            if (arka_plan) {
                // Arka plan süreçleri için yeni bir süreç grubu oluşturuyoruz (isteğe bağlı, sinyal yönetimi için faydalı)
                setpgid(0, 0);
            }
            // Belirtilen komutu çocuk süreçte çalıştırıyoruz
            execvp(argumanlar[0], argumanlar);
            perror("execvp"); // Eğer execvp başarısız olursa hata mesajı yazdırıyoruz
            exit(1);         // Çocuk süreci hata kodu ile sonlandırıyoruz
        } else if (pid < 0) {
            perror("fork"); // Eğer fork başarısız olursa hata mesajı yazdırıyoruz
        } else { // Ana süreç
            if (!arka_plan) {
                int status;
                waitpid(pid, &status, 0); // Eğer arka planda çalışmıyorsa çocuk sürecin bitmesini bekliyoruz
            } else {
                printf("[%d] başlatıldı (arka planda)\n", pid);
                arka_plan_surec_ekle(pid, komut_satiri); // Arka plan sürecini listeye ekliyoruz
            }
        }
    }

    return 0; // Programın başarıyla sonlandığını belirtiyoruz
}