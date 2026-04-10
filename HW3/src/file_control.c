#include "../inc/structs.h"
#include "../get_next_line/get_next_line.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int open_file(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        write(STDERR_FILENO, "Error: Could not open file\n", 27);
        return -1;
    }
    return fd;
}

void trim_newline(char *line)
{
    int len;

    if (!line)
        return;

    len = strlen(line);

    // önce \n sil
    if (len > 0 && line[len - 1] == '\n')
    {
        line[len - 1] = '\0';
        len--;
    }

    // sonra \r kontrol et
    if (len > 0 && line[len - 1] == '\r')
    {
        line[len - 1] = '\0';
    }
}

int is_word_valid(char *word) {
    if (word == NULL) {
        return 0;
    }
    for (int i = 0; word[i] != '\0'; i++) {
        if (word[i] < 'a' || word[i] > 'z') {
            return 0;
        }
    }
    return 1;
}

int is_valid_line(char *line)
{
    int i = 0;

    // id
    if (!isdigit(line[i]))
        return 0;
    while (isdigit(line[i]))
        i++;

    // space
    if (line[i] != ' ')
        return 0;
    i++;

    // word
    if (line[i] < 'a' || line[i] > 'z')
        return 0;
    while (line[i] >= 'a' && line[i] <= 'z')
        i++;

    // space
    if (line[i] != ' ')
        return 0;
    i++;

    // floor
    if (!isdigit(line[i]))
        return 0;
    while (isdigit(line[i]))
        i++;

    // SON: sadece \0 olmalı (newline'lar trim edilmiş olmalı)
    if (line[i] == '\0')
        return 1;

    // Trailing space veya başka karakter varsa hata
    return 0;
}

t_line_input parse_line(char *line)
{
    t_line_input w;
    char *id;
    char *word;
    char *floor;

    id = strtok(line, " \n");
    word = strtok(NULL, " \n");
    floor = strtok(NULL, " \n");

    w.word_id = atoi(id);
    w.word = strdup(word);
    w.sorting_floor = atoi(floor);

    return w;
}

t_line_input *ft_read_input(const char *filename, int *count)
{
    int fd = open_file(filename);
    char *line;
    t_line_input *arr;
    int capacity = 10;
    int i = 0;

    if (fd < 0)
        exit(EXIT_FAILURE);

    arr = malloc(sizeof(t_line_input) * capacity);
    if (!arr)
        exit(EXIT_FAILURE);

    while ((line = get_next_line(fd)))
    {
        trim_newline(line);   // ⭐ BURAYA

        if (line[0] == '\0')
        {
            write(STDERR_FILENO, "Error: Empty line detected\n", 27);
            exit(EXIT_FAILURE);
        }

        if (!is_valid_line(line))
        {
            write(STDERR_FILENO, "Error: Invalid line format\n", 27);
            exit(EXIT_FAILURE);
        }

        if (i >= capacity)
        {
            capacity *= 2;
            t_line_input *tmp = realloc(arr, sizeof(t_line_input) * capacity);
            if (!tmp)
                exit(EXIT_FAILURE);
            arr = tmp;
        }

        arr[i++] = parse_line(line);
        free(line);
    }

    close(fd);
    *count = i;
    return arr;
}