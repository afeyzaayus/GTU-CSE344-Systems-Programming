#include "./includes/struct.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int parse_arguments(int argc, char **argv, t_file *criteria) {
    int i = 1;
    int has_criteria = 0; // En az bir -f, -b, vb. var mı?

(void ) argc;

    while (argv[i]) {
        // Argüman tire '-' ile mi başlıyor ve tam 2 karakter mi? (örn: "-w")
        if (argv[i][0] == '-' && argv[i][1] != '\0' && argv[i][2] == '\0') {
            char flag = argv[i][1];

            // Bir sonraki argüman (değer) var mı?
            if (argv[i + 1] == NULL) {
                fprintf(stderr, "Hata: -%c parametresi icin bir deger girilmedi.\n", flag);
                return 0; 
            }

            // Bayrağa göre struct'ı doldur
            switch (flag) {
                case 'w':
                    criteria->target_dir = argv[i + 1];
                    break;
                case 'f':
                    criteria->filename = argv[i + 1];
                    has_criteria = 1;
                    break;
                case 'b':
                    // atoi() ile string'i tam sayıya (integer) çeviriyoruz
                    criteria->file_size = atoi(argv[i + 1]);
                    has_criteria = 1;
                    break;
                case 't':
                    criteria->file_type = argv[i + 1];
                    has_criteria = 1;
                    break;
                case 'p':
                    criteria->permissions = argv[i + 1];
                    has_criteria = 1;
                    break;
                case 'l':
                    // atoi() ile string'i tam sayıya çeviriyoruz
                    criteria->link_count = atoi(argv[i + 1]);
                    has_criteria = 1;
                    break;
                default:
                    fprintf(stderr, "Hata: Gecersiz parametre -%c\n", flag);
                    return 0;
            }

            // Değeri okuduğumuz için indeksi bir kez daha atlatıyoruz
            i++; 
        } else {
            fprintf(stderr, "Hata: Beklenmeyen formatta arguman -> %s\n", argv[i]);
            return 0; 
        }
        
        i++; // Sonraki bayrağa geç
    }

    // Ödev Kuralları Kontrolü:
    // Gerekli komut satırı argümanları eksikse program kullanım bilgisini yazdırıp çıkmalıdır[cite: 47].
    if (criteria->target_dir == NULL) {
        fprintf(stderr, "Hata: -w (hedef dizin) parametresi zorunludur.\n");
        return 0;
    }
    
    // Arama kriterleri: en az biri kullanılmalıdır[cite: 7].
    if (!has_criteria) {
        fprintf(stderr, "Hata: En az bir arama kriteri (-f, -b, -t, -p, -l) belirtilmelidir.\n");
        return 0;
    }

    return 1; // Her şey yolunda
}