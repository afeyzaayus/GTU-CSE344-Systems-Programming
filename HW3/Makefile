NAME        = hw3

CC          = gcc
CFLAGS      = -Wall -Iinc -Iget_next_line
LDFLAGS     = -lpthread
MAKEFLAGS   += --no-print-directory

SRC         = ./src/main.c              \
              ./src/check/file_control.c      \
              ./src/check/args.c              \
              ./src/shared_memory/shm.c               \
              ./src/shared_memory/shm_init_utils.c    \
              ./src/processes/process_spawn.c     \
              ./src/processes/word_carrier.c      \
              ./src/processes/letter_carrier.c    \
              ./src/processes/sorting_proc.c      \
              ./src/processes/elevator.c          \
              ./src/check/output.c \
              ./src/check/log.c \
              ./src/check/validate_words.c            \
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