.PHONY: run

CFLAGS += -lm -g

rxtest: %: %.o lib.o
	@echo "CC " $@
	@$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	@echo "CC " $@
	@$(CC) -o $@ $^ -c $(CFLAGS)
