NAME        = cargoGTU

CC          = gcc
CFLAGS      = -Wall 
LDFLAGS     = -lpthread
MAKEFLAGS   += --no-print-directory

SRC         = 	main.c              \
				./src/order.c		\
				./src/parser.c		\
				./src/priority_queue.c \
				./src/courier.c	\
				./src/log.c \
				./src/stats.c \
				./src/signal_handler.c \
				./src/parse_cli_args.c



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