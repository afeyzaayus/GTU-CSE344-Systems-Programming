NAME        = myFind

# Derleyici ve Bayraklar
CC          = gcc
CFLAGS      = -Wall
MAKEFLAGS   += --no-print-directory

# Dosya Yolları
SRC         = ./src/main.c \
              ./src/parse_arguments.c \
              ./src/regex.c \
              ./src/search_directory.c \
              ./src/compare.c \
              ./src/print_tree.c \
			  ./src/signal.c

OBJ         = $(SRC:.c=.o)

# Renkler ve Karakterler
GREEN       = \033[0;32m
CYAN        = \033[0;36m
YELLOW      = \033[1;33m
RESET       = \033[0m
CLEAR_LINE  = \033[K
WINKY       = 😊
FILE_ICON   = 📄

# Kurallar
all: $(NAME)

$(NAME): $(OBJ)
	@$(CC) $(CFLAGS) $(OBJ) -o $(NAME)
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
	@printf "$(GREEN)🛠️  Rebuilt   :$(RESET) $(YELLOW)$(NAME)$(RESET) project $(WINKY)$(CLEAR_LINE)\n"

.PHONY: all clean fclean re