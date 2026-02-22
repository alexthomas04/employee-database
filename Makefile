SERVER_TARGET = bin/server
SERVER_SRC = $(wildcard src/server/*.c)
SERVER_OBJ = $(patsubst src/server/%.c, obj/server/%.o, $(SERVER_SRC)) $(patsubst src/common/%.c, obj/common/%.o, $(COMMON_SRC))

CLIENT_TARGET = bin/cli
CLIENT_SRC = $(wildcard src/cli/*.c)
CLIENT_OBJ = $(patsubst src/cli/%.c, obj/cli/%.o, $(CLIENT_SRC)) $(patsubst src/common/%.c, obj/common/%.o, $(COMMON_SRC))

COMMON_SRC = $(wildcard src/common/*c)


default: directories clean $(SERVER_TARGET) $(CLIENT_TARGET)

directories:
	mkdir -p bin obj/server obj/cli  obj/common


clean:
	rm -f obj/*/*.o
	rm -f bin/*
	rm -f *.db

$(SERVER_TARGET): $(SERVER_OBJ)
	gcc -g -o $@ $?

$(CLIENT_TARGET): $(CLIENT_OBJ)
	gcc -g -o $@ $?

obj/%.o : src/%.c
	gcc -g -c $< -o $@ -Iinclude


