#ifndef LIBRARY_H
#define LIBRARY_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_TITLE      150
#define MAX_AUTHOR      80
#define MAX_ISBN        20
#define MAX_GENRE       50
#define MAX_BOOKS     1000
#define DB_FILE     "library.dat"

typedef struct {
    int    id;
    char   title[MAX_TITLE];
    char   author[MAX_AUTHOR];
    char   isbn[MAX_ISBN];
    char   genre[MAX_GENRE];
    int    year;
    int    available;   /* 1 = available, 0 = borrowed */
    int    times_borrowed;
} Book;

typedef enum {
    FIELD_TITLE,
    FIELD_AUTHOR
} SearchField;

/* return codes for the borrow/remove functions */
#define OK             1
#define ERR_NOT_FOUND  0
#define ERR_BORROWED  -1
#define ERR_AVAILABLE -2
#define ERR_FULL      -3
#define ERR_DUPLICATE -4

/* file */
int   load_books(const char *path, Book books[], int *total);
int   save_books(const char *path, const Book books[], int total);

/* operations (no printf/scanf, so they can be tested) */
int   add_book(Book books[], int *total, const Book *data);
Book *find_by_id(Book books[], int total, int id);
Book *find_by_isbn(Book books[], int total, const char *isbn);
int   search_books(const Book books[], int total, SearchField field,
                   const char *term, int results[]);
int   remove_book(Book books[], int *total, int id);
int   borrow_book(Book books[], int total, int id);
int   return_book(Book books[], int total, int id);
int   contains_ignore_case(const char *text, const char *term);

/* buffer size for format_column (each UTF-8 character takes up to 4 bytes) */
#define COLUMN_SIZE(width) ((width) * 4 + 1)
void  format_column(char *dest, const char *text, int width);

/* screens */
void  screen_add(Book books[], int *total);
void  screen_edit(Book books[], int total);
void  list_books(const Book books[], int total);
void  report_available(const Book books[], int total);
void  report_most_borrowed(const Book books[], int total);
void  report_summary(const Book books[], int total);

/* keyboard input */
void  clear_buffer(void);
int   read_int(const char *prompt, int min, int max);
void  read_string(const char *prompt, char *dest, int size);
void  wait_enter(void);

#endif
