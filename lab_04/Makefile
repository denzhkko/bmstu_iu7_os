all: 01 02 03 04 05

clean:
	rm -f 01 02 03 04 05

%: src/main_%.c
	$(CC) -o $@ $^
