CC ?= cc
PKG_CONFIG ?= pkg-config
CFLAGS ?= -O2 -Wall -Wextra
SDL_CFLAGS = $(shell $(PKG_CONFIG) --cflags sdl2 2>/dev/null)
SDL_LIBS = $(shell $(PKG_CONFIG) --libs sdl2 2>/dev/null)
SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,.build/%.o,$(SOURCES))

.PHONY: all check-deps deps clean run
all: divseed

check-deps:
	@$(PKG_CONFIG) --atleast-version=2.0.18 sdl2 2>/dev/null || { \
	 echo 'SDL2 development libraries (2.0.18+) or pkg-config are missing.'; \
	 echo 'Debian/Ubuntu: sudo apt install build-essential pkg-config libsdl2-dev'; \
	 echo 'Or run make deps to offer installation for your distribution.'; exit 1; }

.build:
	mkdir -p $@

.build/%.o: src/%.c | check-deps .build
	$(CC) $(CPPFLAGS) $(SDL_CFLAGS) $(CFLAGS) -std=c11 -MMD -MP -c $< -o $@

divseed: $(OBJECTS)
	$(CC) $(LDFLAGS) $^ $(SDL_LIBS) $(LDLIBS) -o $@

run: divseed
	./divseed $(ARGS)

# Explicit, interactive installation; normal builds never alter the system.
deps:
	@if $(PKG_CONFIG) --atleast-version=2.0.18 sdl2 2>/dev/null && command -v $(CC) >/dev/null; then \
	 echo 'Compiler and SDL2 development libraries are available.'; exit 0; fi; \
	if command -v apt-get >/dev/null; then packages='build-essential pkg-config libsdl2-dev'; manager=apt-get; \
	elif command -v dnf >/dev/null; then packages='gcc make pkgconf-pkg-config SDL2-devel'; manager=dnf; \
	elif command -v pacman >/dev/null; then packages='base-devel pkgconf sdl2'; manager=pacman; \
	else echo 'Install a C compiler, pkg-config and SDL2 development libraries using your package manager.'; exit 1; fi; \
	printf 'Install %s using %s and sudo? [y/N] ' "$$packages" "$$manager"; \
	read answer; case "$$answer" in y|Y|yes) ;; *) echo 'Installation cancelled.'; exit 1;; esac; \
	case "$$manager" in \
	 apt-get) sudo apt-get update && sudo apt-get install $$packages ;; \
	 dnf) sudo dnf install $$packages ;; \
	 pacman) sudo pacman -S --needed $$packages ;; esac

clean:
	rm -rf .build divseed

-include $(OBJECTS:.o=.d)
