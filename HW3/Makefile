NAME        = hw3

CC          = gcc
CFLAGS      = -Wall -Wextra -Iinc -Iget_next_line
LDFLAGS     = -lpthread
MAKEFLAGS   += --no-print-directory

SRC         = ./src/main.c              \
              ./src/file_control.c      \
              ./src/args.c              \
              ./src/shm.c               \
              ./src/shm_init_utils.c    \
              ./src/process_spawn.c     \
              ./src/word_carrier.c      \
              ./src/letter_carrier.c    \
              ./src/sorting_proc.c      \
              ./src/elevator.c          \
              ./src/output.c \
              ./src/log.c \
              ./src/validate_words.c            \
              ./get_next_line/get_next_line.c       \
              ./get_next_line/get_next_line_utils.c

OBJ         = $(SRC:.c=.o)

GREEN       = \033[0;32m
CYAN        = \033[0;36m
YELLOW      = \033[1;33m
RESET       = \033[0m
CLEAR_LINE  = \033[K

all: $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(CFLAGS) $(OBJ) $(LDFLAGS) -o $(NAME)
	@printf "\r$(GREEN)✔️  Built      :$(RESET) $(YELLOW)$(NAME)$(RESET) executable created$(CLEAR_LINE)\n"

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@printf "\r$(GREEN)📃 Compiling :$(RESET) $(CYAN)$<$(RESET) $(CLEAR_LINE)"

clean:
	@rm -rf $(OBJ)
	@echo "$(GREEN)🧹 Cleaned   :$(RESET) $(CYAN)object files$(RESET)"

fclean: clean
	@rm -rf $(NAME)
	@echo "$(GREEN)🗑️  Removed   :$(RESET) $(YELLOW)$(NAME)$(RESET) executable"

re: fclean all

.PHONY: all clean fclean re