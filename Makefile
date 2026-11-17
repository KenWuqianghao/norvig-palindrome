CFLAGS = -O2 -Wall -Wno-char-subscripts -Wno-unused-function -Wno-unused-variable

tools: build/dfs build/euler build/scc build/mip

build/%: src/%.c src/dict.h src/graph.h
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -rf build
