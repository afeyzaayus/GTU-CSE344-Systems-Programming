NAME        = analyzer

CC          = gcc
CFLAGS      = -Wall -Iinc -Iget_next_line
LDFLAGS     = -lpthread
MAKEFLAGS   += --no-print-directory

SRC         = ./src/main.c              \
              ./src/aggregator.c \
			  ./src/analyzer.c \
			  ./src/buffer.c \
			  ./src/dispatcher.c \
			  ./src/reader.c \
			  ./src/watchdog.c \
			  ./src/parser/config_parser.c \
			  ./src/parser/log_entry.c \
			  ./src/parser/parser_utils.c \
			  ./src/parser/parser.c \
			  ./src/parser/priority_parser.c \
			  ./src/shm/shm_queue.c \
			  ./src/shm/shm_utils.c \
			  ./src/shm/shm.c \



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