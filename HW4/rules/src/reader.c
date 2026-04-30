#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "reader.h"
#include "log_entry.h"
#include "shm.h"

// --------------------------------------------------
// İÇ BUFFER STRUCT
// Sadece bu Reader process içinde kullanılır.
// PTHREAD_PROCESS_SHARED değil — default attribute yeterli.
// --------------------------------------------------
typedef struct s_internal_buffer
{
    t_log_entry     *data;
    int             capacity;
    int             head;
    int             tail;
    int             count;
    int             done;           // tüm reader thread'ler bitti mi?
    int             active_readers; // hâlâ çalışan reader thread sayısı
    pthread_mutex_t mutex;
    pthread_cond_t  not_full;
    pthread_cond_t  not_empty;
}   t_internal_buffer;

// --------------------------------------------------
// READER THREAD ARGÜMANLARI
// --------------------------------------------------
typedef struct s_reader_thread_args
{
    int                 thread_id;
    int                 reader_index;   // kaçıncı Reader process'iz
    char                *filepath;
    long                start_byte;
    long                end_byte;
    t_internal_buffer   *ibuf;
    int                 pipe_write_fd;
    t_args              *args;
}   t_reader_thread_args;

// --------------------------------------------------
// PARSER THREAD ARGÜMANLARI
// --------------------------------------------------
typedef struct s_parser_thread_args
{
    t_internal_buffer   *ibuf;
    t_region_a          *region_a;
    t_args              *args;
    int                 reader_index;
}   t_parser_thread_args;

// ==================================================
// İÇ BUFFER FONKSİYONLARI
// ==================================================

static int ibuf_init(t_internal_buffer *ibuf, int capacity)
{
    ibuf->data = malloc(sizeof(t_log_entry) * capacity);
    if (!ibuf->data)
        return (perror("ibuf malloc"), 0);

    ibuf->capacity      = capacity;
    ibuf->head          = 0;
    ibuf->tail          = 0;
    ibuf->count         = 0;
    ibuf->done          = 0;
    ibuf->active_readers = 0;

    // default attribute — PROCESS_SHARED değil
    if (pthread_mutex_init(&ibuf->mutex, NULL) != 0)
        return (perror("ibuf mutex_init"), 0);
    if (pthread_cond_init(&ibuf->not_full, NULL) != 0)
        return (perror("ibuf cond not_full"), 0);
    if (pthread_cond_init(&ibuf->not_empty, NULL) != 0)
        return (perror("ibuf cond not_empty"), 0);

    return (1);
}

static void ibuf_free(t_internal_buffer *ibuf)
{
    pthread_mutex_destroy(&ibuf->mutex);
    pthread_cond_destroy(&ibuf->not_full);
    pthread_cond_destroy(&ibuf->not_empty);
    free(ibuf->data);
}

// Producer: reader thread çağırır
static void ibuf_push(t_internal_buffer *ibuf, t_log_entry *entry)
{
    pthread_mutex_lock(&ibuf->mutex);

    while (ibuf->count == ibuf->capacity)
        pthread_cond_wait(&ibuf->not_full, &ibuf->mutex);

    ibuf->data[ibuf->tail] = *entry;
    ibuf->tail = (ibuf->tail + 1) % ibuf->capacity;
    ibuf->count++;

    pthread_cond_signal(&ibuf->not_empty);
    pthread_mutex_unlock(&ibuf->mutex);
}

// Consumer: parser thread çağırır
// Döndürür: 1 = entry alındı, 0 = buffer boş VE done
static int ibuf_pop(t_internal_buffer *ibuf, t_log_entry *entry)
{
    pthread_mutex_lock(&ibuf->mutex);

    while (ibuf->count == 0 && !ibuf->done)
        pthread_cond_wait(&ibuf->not_empty, &ibuf->mutex);

    if (ibuf->count == 0 && ibuf->done)
    {
        pthread_mutex_unlock(&ibuf->mutex);
        return (0); // bitti
    }

    *entry = ibuf->data[ibuf->head];
    ibuf->head = (ibuf->head + 1) % ibuf->capacity;
    ibuf->count--;

    pthread_cond_signal(&ibuf->not_full);
    pthread_mutex_unlock(&ibuf->mutex);
    return (1);
}

// Bir reader thread bitince çağrılır
static void ibuf_reader_done(t_internal_buffer *ibuf)
{
    pthread_mutex_lock(&ibuf->mutex);
    ibuf->active_readers--;
    if (ibuf->active_readers == 0)
    {
        ibuf->done = 1;
        pthread_cond_signal(&ibuf->not_empty); // parser thread'i uyandır
    }
    pthread_mutex_unlock(&ibuf->mutex);
}

// ==================================================
// BYTE RANGE HESAPLAMA
// Dosyayı T eşit parçaya böler.
// Range sınırı satır ortasına denk gelirse,
// bir sonraki '\n'e kadar ilerler.
// ==================================================
static long find_line_start(FILE *fp, long pos)
{
    // pos == 0 ise zaten satır başındayız
    if (pos == 0)
        return (0);

    fseek(fp, pos - 1, SEEK_SET);
    int c = fgetc(fp);

    // Önceki karakter '\n' ise bu pos zaten satır başı
    if (c == '\n' || c == EOF)
        return (pos);

    // Değilse bir sonraki '\n'i bul
    while ((c = fgetc(fp)) != EOF && c != '\n')
        ;

    return (ftell(fp));
}

// ==================================================
// PIPE HEARTBEAT
// Her 50 satırda bir pipe'a yazar.
// ==================================================
static void send_heartbeat(int pipe_fd, int reader_index, long line_count)
{
    char buf[128];
    int  len;

    len = snprintf(buf, sizeof(buf),
                   "[R%d] %ld lines processed\n",
                   reader_index, line_count);
    if (len > 0)
        write(pipe_fd, buf, len);
}

// ==================================================
// READER THREAD FONKSİYONU
// ==================================================
static void *reader_thread_func(void *arg)
{
    t_reader_thread_args    *rarg = (t_reader_thread_args *)arg;
    FILE                    *fp;
    char                    line[1024];
    t_log_entry             entry;
    long                    lines_read = 0;
    long                    malformed  = 0;

    printf("[PID:%d][TID:%ld] Reader thread %d: range [%ld, %ld) bytes\n",
           getpid(), (long)pthread_self(),
           rarg->thread_id, rarg->start_byte, rarg->end_byte);

    fp = fopen(rarg->filepath, "r");
    if (!fp)
    {
        perror("reader thread fopen");
        ibuf_reader_done(rarg->ibuf);
        return (NULL);
    }

    // Gerçek satır başını bul (range sınırı satır ortasına denk gelebilir)
    long real_start = find_line_start(fp, rarg->start_byte);
    fseek(fp, real_start, SEEK_SET);

    while (ftell(fp) < rarg->end_byte)
    {
        if (!fgets(line, sizeof(line), fp))
            break;

        // Satır sonu karakterlerini temizle
        line[strcspn(line, "\r\n")] = '\0';

        if (line[0] == '\0')
            continue;

        if (parse_log_line(line, &entry))
        {
            ibuf_push(rarg->ibuf, &entry);
            lines_read++;

            // Her 50 satırda heartbeat gönder
            if (lines_read % 50 == 0)
                send_heartbeat(rarg->pipe_write_fd,
                               rarg->reader_index, lines_read);
        }
        else
            malformed++;
    }

    fclose(fp);

    printf("[PID:%d][TID:%ld] Reader thread %d: finished, lines_read=%ld, malformed=%ld\n",
           getpid(), (long)pthread_self(),
           rarg->thread_id, lines_read, malformed);

    // Son heartbeat (kalan satırlar için)
    if (lines_read % 50 != 0)
        send_heartbeat(rarg->pipe_write_fd, rarg->reader_index, lines_read);

    ibuf_reader_done(rarg->ibuf);
    return (NULL);
}

// ==================================================
// PARSER THREAD FONKSİYONU
// İç buffer'dan okur, Region A'ya yazar.
// ==================================================
static void *parser_thread_func(void *arg)
{
    t_parser_thread_args    *parg = (t_parser_thread_args *)arg;
    t_log_entry             entry;
    long                    dispatched[4] = {0, 0, 0, 0}; // E, W, I, D

    while (ibuf_pop(parg->ibuf, &entry))
    {
        if (entry.level < 0 || entry.level >= LV_INVALID)
            continue;

        // Region A'ya push et (mutex + condvar ile)
        pthread_mutex_lock(&parg->region_a->mutex);

        while (parg->region_a->count == parg->region_a->capacity)
            pthread_cond_wait(&parg->region_a->not_full, &parg->region_a->mutex);

        region_a_push(parg->region_a, &entry);
        dispatched[entry.level]++;

        pthread_cond_signal(&parg->region_a->not_empty);
        pthread_mutex_unlock(&parg->region_a->mutex);
    }

    // EOF marker'ları gönder — her level için bir tane
    // is_eof = 1 olan özel entry'ler
    t_log_entry eof_entry;
    memset(&eof_entry, 0, sizeof(t_log_entry));
    eof_entry.is_eof = 1;

    for (int lv = 0; lv < 4; lv++)
    {
        eof_entry.level = (t_level)lv;

        pthread_mutex_lock(&parg->region_a->mutex);

        while (parg->region_a->count == parg->region_a->capacity)
            pthread_cond_wait(&parg->region_a->not_full, &parg->region_a->mutex);

        region_a_push(parg->region_a, &eof_entry);

        // EOF sayacını güncelle
        parg->region_a->eof_count_per_level[lv]++;

        pthread_cond_signal(&parg->region_a->not_empty);
        pthread_mutex_unlock(&parg->region_a->mutex);
    }

    printf("[PID:%d] Parser thread: dispatched E:%ld W:%ld I:%ld D:%ld -> Region A\n",
           getpid(),
           dispatched[LV_ERROR],
           dispatched[LV_WARN],
           dispatched[LV_INFO],
           dispatched[LV_DEBUG]);

    return (NULL);
}

// ==================================================
// RUN_READER — Ana Fonksiyon
// main.c'den fork sonrası çağrılır.
// ==================================================
void run_reader(t_args *args, t_shm *shm, int reader_index, int pipe_write_fd)
{
    t_internal_buffer       ibuf;
    pthread_t               *reader_tids;
    pthread_t               parser_tid;
    t_reader_thread_args    *rargs;
    t_parser_thread_args    parg;
    int                     T = args->reader_threads;

    printf("[PID:%d] Reader %d started. File: %s, Threads: %d\n",
           getpid(), reader_index, args->log_files[reader_index], T);

    // 1. İç buffer init (capacity = cap_b, makul bir değer)
    if (!ibuf_init(&ibuf, args->cap_b))
    {
        fprintf(stderr, "Reader %d: ibuf_init failed\n", reader_index);
        return;
    }
    ibuf.active_readers = T;

    // 2. Dosya boyutunu öğren
    FILE *fp = fopen(args->log_files[reader_index], "r");
    if (!fp)
    {
        perror("run_reader fopen");
        ibuf_free(&ibuf);
        return;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fclose(fp);

    if (file_size == 0)
    {
        fprintf(stderr, "Reader %d: empty file, skipping\n", reader_index);
        ibuf.done = 1;
        // EOF marker'ları yine de gönder
    }

    // 3. Reader thread argümanları hazırla
    reader_tids = malloc(sizeof(pthread_t) * T);
    rargs       = malloc(sizeof(t_reader_thread_args) * T);

    if (!reader_tids || !rargs)
    {
        perror("malloc reader args");
        ibuf_free(&ibuf);
        free(reader_tids);
        free(rargs);
        return;
    }

    long chunk = (file_size + T - 1) / T; // ceiling division

    for (int i = 0; i < T; i++)
    {
        rargs[i].thread_id      = i;
        rargs[i].reader_index   = reader_index;
        rargs[i].filepath       = args->log_files[reader_index];
        rargs[i].start_byte     = i * chunk;
        rargs[i].end_byte       = (i + 1) * chunk;
        rargs[i].ibuf           = &ibuf;
        rargs[i].pipe_write_fd  = pipe_write_fd;
        rargs[i].args           = args;

        // Son thread dosyanın sonuna kadar gider
        if (rargs[i].end_byte > file_size || i == T - 1)
            rargs[i].end_byte = file_size;

        printf("[PID:%d][TID:%ld] Reader thread %d: range [%ld, %ld) bytes\n",
               getpid(), (long)pthread_self(),
               i, rargs[i].start_byte, rargs[i].end_byte);
    }

    // 4. Parser thread argümanı hazırla
    parg.ibuf           = &ibuf;
    parg.region_a       = shm->a;
    parg.args           = args;
    parg.reader_index   = reader_index;

    // 5. Parser thread'i başlat (reader thread'lerden ÖNCE)
    if (pthread_create(&parser_tid, NULL, parser_thread_func, &parg) != 0)
    {
        perror("pthread_create parser");
        ibuf_free(&ibuf);
        free(reader_tids);
        free(rargs);
        return;
    }

    // 6. Reader thread'leri başlat
    for (int i = 0; i < T; i++)
    {
        if (pthread_create(&reader_tids[i], NULL, reader_thread_func, &rargs[i]) != 0)
        {
            perror("pthread_create reader thread");
            // Başlanan thread'leri join et
            for (int j = 0; j < i; j++)
                pthread_join(reader_tids[j], NULL);
            // ibuf'u done yap ki parser çıksın
            pthread_mutex_lock(&ibuf.mutex);
            ibuf.done = 1;
            pthread_cond_signal(&ibuf.not_empty);
            pthread_mutex_unlock(&ibuf.mutex);
            pthread_join(parser_tid, NULL);
            ibuf_free(&ibuf);
            free(reader_tids);
            free(rargs);
            return;
        }
    }

    // 7. Tüm reader thread'leri join et
    for (int i = 0; i < T; i++)
        pthread_join(reader_tids[i], NULL);

    // 8. Parser thread'i join et
    pthread_join(parser_tid, NULL);

    // 9. Cleanup
    ibuf_free(&ibuf);
    free(reader_tids);
    free(rargs);

    printf("[PID:%d] Reader %d exiting.\n", getpid(), reader_index);
}