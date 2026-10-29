#include "library.h"

/* ---------- file ---------- */

/* .dat format: an int with the total and then the structs one after the other.
 * returns 1 if it loaded, 0 if the file doesn't exist and -1 if it's corrupted */
int load_books(const char *path, Book books[], int *total) {
    *total = 0;
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return 0;
    }

    int count;
    if (fread(&count, sizeof(int), 1, file) != 1 || count < 0 || count > MAX_BOOKS) {
        fclose(file);
        return -1;
    }

    if (fread(books, sizeof(Book), (size_t)count, file) != (size_t)count) {
        fclose(file);
        return -1;
    }

    fclose(file);
    *total = count;
    return 1;
}

int save_books(const char *path, const Book books[], int total) {
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        printf("Error: couldn't save the data.\n");
        return 0;
    }

    int ok = fwrite(&total, sizeof(int), 1, file) == 1
          && fwrite(books, sizeof(Book), (size_t)total, file) == (size_t)total;

    if (fclose(file) != 0) ok = 0;
    if (!ok) printf("Error: failed to write %s.\n", path);
    return ok;
}

/* ---------- operations ---------- */

/* copies *data into the collection with a new id (biggest id + 1).
 * returns the id, ERR_FULL or ERR_DUPLICATE (same ISBN as a book already there) */
int add_book(Book books[], int *total, const Book *data) {
    if (*total >= MAX_BOOKS) {
        return ERR_FULL;
    }
    if (find_by_isbn(books, *total, data->isbn) != NULL) {
        return ERR_DUPLICATE;
    }

    int max_id = 0;
    for (int i = 0; i < *total; i++) {
        if (books[i].id > max_id) max_id = books[i].id;
    }

    Book *book = &books[*total];
    *book = *data;
    book->id = max_id + 1;
    book->available = 1;
    book->times_borrowed = 0;

    (*total)++;
    return book->id;
}

Book *find_by_id(Book books[], int total, int id) {
    for (int i = 0; i < total; i++) {
        if (books[i].id == id) {
            return &books[i];
        }
    }
    return NULL;
}

/* the same ISBN gets typed as 978-85-359-0277-1 or 9788535902771, so only digits and X count */
static void isbn_digits(char *dest, const char *isbn) {
    int n = 0;
    for (; *isbn && n < MAX_ISBN - 1; isbn++) {
        if (isdigit((unsigned char)*isbn) || toupper((unsigned char)*isbn) == 'X') {
            dest[n++] = (char)toupper((unsigned char)*isbn);
        }
    }
    dest[n] = '\0';
}

/* NULL when the ISBN is empty: books without one can't be compared */
Book *find_by_isbn(Book books[], int total, const char *isbn) {
    char wanted[MAX_ISBN], other[MAX_ISBN];
    isbn_digits(wanted, isbn);
    if (wanted[0] == '\0') return NULL;

    for (int i = 0; i < total; i++) {
        isbn_digits(other, books[i].isbn);
        if (strcmp(wanted, other) == 0) return &books[i];
    }
    return NULL;
}

/* case insensitive strstr */
int contains_ignore_case(const char *text, const char *term) {
    if (*term == '\0') return 1;

    for (; *text; text++) {
        const char *t = text, *p = term;
        while (*t && *p && tolower((unsigned char)*t) == tolower((unsigned char)*p)) {
            t++;
            p++;
        }
        if (*p == '\0') return 1;
    }
    return 0;
}

/* Cuts the text at `width` characters and pads it with spaces.
 * printf("%-35.35s") counts bytes, and in UTF-8 an "ó" takes 2: the table came out crooked
 * and a long title could get cut in the middle of an accented letter. Here I count
 * characters: every byte that isn't a continuation byte (10xxxxxx) starts a new character.
 * `dest` needs COLUMN_SIZE(width) bytes. */
/* counts characters, not bytes (the bytes 10xxxxxx continue a character) */
static int utf8_length(const char *text) {
    int count = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if ((*p & 0xC0) != 0x80) count++;
    }
    return count;
}

void format_column(char *dest, const char *text, int width) {
    const unsigned char *p = (const unsigned char *)text;
    int chars = 0;
    /* when the text doesn't fit, it ends in "..." so it's clear it was cut */
    int cut = width > 3 && utf8_length(text) > width;
    int limit = cut ? width - 3 : width;

    while (*p && chars < limit) {
        int bytes = *p >= 0xF0 ? 4 : *p >= 0xE0 ? 3 : *p >= 0xC0 ? 2 : 1;
        /* the *p in the loop avoids going past the end if the text ends in the middle of a character */
        for (int i = 0; i < bytes && *p; i++) *dest++ = (char)*p++;
        chars++;
    }
    if (cut) {
        memcpy(dest, "...", 3);
        dest += 3;
        chars += 3;
    }
    while (chars++ < width) *dest++ = ' ';
    *dest = '\0';
}

/* fills results[] with the indexes found and returns how many were found */
int search_books(const Book books[], int total, SearchField field,
                 const char *term, int results[]) {
    int count = 0;
    for (int i = 0; i < total; i++) {
        const char *text = field == FIELD_AUTHOR ? books[i].author : books[i].title;
        if (contains_ignore_case(text, term)) {
            results[count++] = i;
        }
    }
    return count;
}

int remove_book(Book books[], int *total, int id) {
    for (int i = 0; i < *total; i++) {
        if (books[i].id == id) {
            if (!books[i].available) {
                return ERR_BORROWED;
            }
            /* moves the next ones one position back, keeping the order */
            memmove(&books[i], &books[i + 1], (size_t)(*total - i - 1) * sizeof(Book));
            (*total)--;
            return OK;
        }
    }
    return ERR_NOT_FOUND;
}

int borrow_book(Book books[], int total, int id) {
    Book *book = find_by_id(books, total, id);
    if (book == NULL) return ERR_NOT_FOUND;
    if (!book->available) return ERR_BORROWED;

    book->available = 0;
    book->times_borrowed++;
    return OK;
}

int return_book(Book books[], int total, int id) {
    Book *book = find_by_id(books, total, id);
    if (book == NULL) return ERR_NOT_FOUND;
    if (book->available) return ERR_AVAILABLE;

    book->available = 1;
    return OK;
}

/* ---------- screens ---------- */

void screen_add(Book books[], int *total) {
    if (*total >= MAX_BOOKS) {
        printf("Library is full! Limit of %d books reached.\n", MAX_BOOKS);
        return;
    }

    Book data = {0};
    printf("\n--- ADD BOOK ---\n");
    do {
        read_string("Title", data.title, MAX_TITLE);
    } while (data.title[0] == '\0');
    read_string("Author", data.author, MAX_AUTHOR);
    read_string("ISBN", data.isbn, MAX_ISBN);
    read_string("Genre", data.genre, MAX_GENRE);
    data.year = read_int("Publication year", 1000, 2100);

    const Book *same = find_by_isbn(books, *total, data.isbn);
    if (same != NULL) {
        printf("\nThere's already a book with this ISBN: [%d] %s\n", same->id, same->title);
        return;
    }

    int id = add_book(books, total, &data);
    save_books(DB_FILE, books, *total);
    printf("\nBook added! ID: %d\n", id);
}

/* shows the current value and reads the new one; Enter keeps it. The read is already
 * limited to `size`, so the text always fits in the destination. */
static void edit_field(const char *current_label, const char *new_label,
                       char *dest, int size) {
    char input[MAX_TITLE];
    printf("%s: %s\n", current_label, dest);
    read_string(new_label, input, size);
    if (input[0]) memcpy(dest, input, strlen(input) + 1);
}

void screen_edit(Book books[], int total) {
    int id = read_int("ID of the book to edit", 1, 99999);
    Book *book = find_by_id(books, total, id);
    if (book == NULL) {
        printf("Book with ID %d not found.\n", id);
        return;
    }

    char input[8];
    printf("\n--- EDIT BOOK %d --- (Enter keeps the current value)\n", id);

    edit_field("Current title", "New title", book->title, MAX_TITLE);
    edit_field("Current author", "New author", book->author, MAX_AUTHOR);
    edit_field("Current ISBN", "New ISBN", book->isbn, MAX_ISBN);
    edit_field("Current genre", "New genre", book->genre, MAX_GENRE);

    printf("Current year: %d\n", book->year);
    read_string("New year", input, (int)sizeof(input));
    if (input[0]) {
        int year = atoi(input);
        if (year >= 1000 && year <= 2100) book->year = year;
        else printf("Invalid year, kept %d.\n", book->year);
    }

    save_books(DB_FILE, books, total);
    printf("Book updated!\n");
}

void list_books(const Book books[], int total) {
    if (total == 0) {
        printf("\nNo books yet.\n");
        return;
    }

    char title[COLUMN_SIZE(35)], author[COLUMN_SIZE(20)], genre[COLUMN_SIZE(12)];

    printf("\n%-4s  %-35s  %-20s  %-4s  %-12s  %s\n", "ID", "Title", "Author", "Year", "Genre", "Status");
    printf("%s\n", "----------------------------------------------------------------------------------------------");

    for (int i = 0; i < total; i++) {
        const Book *b = &books[i];
        format_column(title, b->title, 35);
        format_column(author, b->author, 20);
        format_column(genre, b->genre, 12);
        printf("%-4d  %s  %s  %-4d  %s  %s\n",
               b->id, title, author, b->year, genre,
               b->available ? "Available" : "Borrowed");
    }

    printf("\nTotal: %d book(s)\n", total);
}

/* ---------- reports ---------- */

void report_available(const Book books[], int total) {
    int count = 0;
    printf("\n--- AVAILABLE BOOKS ---\n");
    for (int i = 0; i < total; i++) {
        if (books[i].available) {
            printf("  [%d] %s - %s (%d)\n",
                   books[i].id, books[i].title,
                   books[i].author, books[i].year);
            count++;
        }
    }
    printf("\nAvailable: %d of %d\n", count, total);
}

/* qsort gets pointers to the elements; here each element is already a Book* */
static int compare_borrows(const void *a, const void *b) {
    const Book *ba = *(const Book *const *)a;
    const Book *bb = *(const Book *const *)b;
    return bb->times_borrowed - ba->times_borrowed;
}

void report_most_borrowed(const Book books[], int total) {
    if (total == 0) { printf("No books yet.\n"); return; }

    /* sorts an array of pointers so the structs don't get copied around */
    const Book *sorted[MAX_BOOKS];
    for (int i = 0; i < total; i++) sorted[i] = &books[i];
    qsort(sorted, (size_t)total, sizeof(sorted[0]), compare_borrows);

    /* books nobody borrowed yet aren't a ranking, they just filled the list with 0x */
    int limit = 0;
    while (limit < total && limit < 5 && sorted[limit]->times_borrowed > 0) limit++;
    if (limit == 0) { printf("\nNo book was borrowed yet.\n"); return; }

    printf("\n--- TOP %d MOST BORROWED ---\n", limit);
    for (int i = 0; i < limit; i++) {
        printf("  %d. [%dx] %s - %s\n",
               i + 1, sorted[i]->times_borrowed,
               sorted[i]->title, sorted[i]->author);
    }
}

void report_summary(const Book books[], int total) {
    int borrowed = 0, total_borrows = 0;
    int oldest = -1;
    for (int i = 0; i < total; i++) {
        if (!books[i].available) borrowed++;
        total_borrows += books[i].times_borrowed;
        if (oldest < 0 || books[i].year < books[oldest].year) {
            oldest = i;
        }
    }

    printf("\n--- COLLECTION SUMMARY ---\n");
    printf("  Books:                  %d\n", total);
    printf("  Available:              %d\n", total - borrowed);
    printf("  Borrowed now:           %d\n", borrowed);
    printf("  Borrows (all time):     %d\n", total_borrows);
    if (oldest >= 0) {
        printf("  Oldest:                 %s (%d)\n",
               books[oldest].title, books[oldest].year);
    }
}

/* ---------- input ---------- */

/* Ctrl+D / Ctrl+Z: quits instead of reading forever */
static void end_of_input(void) {
    printf("\nInput closed. The data is already saved. Bye!\n");
    exit(0);
}

void clear_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int read_int(const char *prompt, int min, int max) {
    int value;
    for (;;) {
        printf("%s (%d-%d): ", prompt, min, max);
        int got = scanf("%d", &value);
        if (got == EOF) end_of_input();
        clear_buffer();
        if (got == 1 && value >= min && value <= max) return value;
        printf("Invalid input. ");
    }
}

void read_string(const char *prompt, char *dest, int size) {
    printf("%s: ", prompt);
    if (fgets(dest, size, stdin) == NULL) end_of_input();

    size_t len = strcspn(dest, "\n");
    if (dest[len] == '\n') {
        dest[len] = '\0';
    } else {
        /* line longer than the buffer: throw the rest away so it doesn't fall into the next prompt */
        clear_buffer();
    }
}

void wait_enter(void) {
    printf("\nPress Enter to continue...");
    int c = getchar();
    if (c == EOF) end_of_input();
    if (c != '\n') clear_buffer();
}
