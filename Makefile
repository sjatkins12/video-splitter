NAME = ffmpeg_tuto

CC = gcc

FLAGS = -Wall -Wextra -Werror -g -fsanitize=address
CFLAGS = -Wall -Wextra -Werror -c

################################################################################
# SOURCE FILES                                                                 #
################################################################################

SRC =  \
			main

################################################################################
# Source directories identifiers                                               #
################################################################################

SRCDIR = src/

FFMPEGSRC = $(patsubst %, %.o, $(addprefix $(SRCDIR), $(SRC)))

################################################################################
# INCLUDE PATHS                                                                #
################################################################################

INCLUDES = \
		-I inc

LIB_LINK = \
	-l avutil \
	-l avformat \
	-l avcodec \
	-l z \
	-l swscale \
	-l m 

################################################################################
# COLOR                                                                        #
################################################################################

RED = \033[1;31m
GREEN = \033[1;32m
YELLOW = \033[1;33m
CYAN = \033[1;36m
RES = \033[0m

################################################################################
# RULES                                                                        #
################################################################################

all: $(NAME)

$(NAME): $(FFMPEGSRC)
	@ echo "$(YELLOW)Compiling programs$(RES)"
	$(CC) $(FLAGS) $(LIB_LINK) $(INCLUDES) $(FFMPEGSRC) -o $(NAME)
	@echo "$(GREEN)Binaries Compiled$(RES)"

%.o: %.c
	@ echo "$(YELLOW)Compiling $<...$(RES)"
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

debug: CFLAGS += -g -DDEBUG
debug: clean $(NAME)

clean:
	rm -f $(FFMPEGSRC)
	@ echo "$(RED)Cleaning folders of object files...$(RES)"

fclean: clean
	rm -f $(NAME)
	@ echo "$(RED)Removing library file and binary...$(RES)"

re: fclean all
	@ echo "$(GREEN)Binary Remade$(RES)"
