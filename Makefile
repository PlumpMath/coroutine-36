SLIBRARY = libcoroutine.a
OBJ = coroutine.o

CFLAGS = -Wall -g

all : $(SLIBRARY)

$(SLIBRARY) : $(OBJ)
	ar -rv $@ $(OBJ)
	ranlib $@
	rm $(OBJ)

%.o : %.c
	gcc $(CFLAGS) -c $< -o $@ 

clean:
	rm -rf $(OBJ) $(SLIBRARY)
