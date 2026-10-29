# Library Manager

![Tests](https://github.com/ViniciuscLemos/library-manager-c/actions/workflows/tests.yml/badge.svg)

A library system for the terminal, written in C. You can add, edit, search and remove books, keep track of who borrowed what and see a few reports.

The data is saved to a binary file (`library.dat`) using `fwrite`/`fread`.

## Building

You need gcc (on Windows I recommend MSYS2 or WSL).

```bash
make
./library
```

Or without make:

```bash
gcc -Wall -Wextra -std=c17 -o library src/main.c src/library.c
```

Tests:

```bash
make test
```

## What's in the menu

- add, edit and remove books (a second book with the same ISBN is refused, with or without the dashes)
- list all books
- search by title, author or genre (case insensitive)
- borrow and return
- reports: available books, the 5 most borrowed and a summary of the collection

The first time it runs, it already adds 3 sample books.

## What it looks like

Listing the books and the collection summary:

![The menu, the book list and the summary in the terminal](docs/screenshot.png)

This table gave me some work. `printf("%-35.35s")` aligns by counting bytes, but in UTF-8 letters like "á" and "ê" take 2 bytes. Because of that, every row with an accented title (like "Memórias Póstumas de Brás Cubas") came out shorter than the others, and when the title went over the limit it could get cut in the middle of a letter, showing a broken character. Now text that doesn't fit ends in `...`. I wrote `format_column`, which counts characters instead of bytes (every byte that doesn't start with `10` in binary starts a new character). The tests even cover a text that ends in the middle of a character, and in CI they run with AddressSanitizer to make sure nothing is read outside the memory.

## Files

```
src/main.c          menu
src/library.c       the system's functions
src/library.h       Book struct and declarations
tests/              tests
Makefile
```
