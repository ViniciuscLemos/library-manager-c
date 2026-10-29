#include "library.h"

/* static so it doesn't go on the stack (it's about 300 KB) */
static Book collection[MAX_BOOKS];
static int book_count = 0;

static void add_sample(int *total, const char *title, const char *author,
                       const char *isbn, const char *genre, int year) {
    Book b = {0};
    snprintf(b.title, MAX_TITLE, "%s", title);
    snprintf(b.author, MAX_AUTHOR, "%s", author);
    snprintf(b.isbn, MAX_ISBN, "%s", isbn);
    snprintf(b.genre, MAX_GENRE, "%s", genre);
    b.year = year;
    add_book(collection, total, &b);
}

static void add_samples(int *total) {
    printf("Adding sample books...\n");
    add_sample(total, "The Pragmatic Programmer", "David Thomas & Andrew Hunt",
               "978-0135957059", "Technology", 2019);
    add_sample(total, "Clean Code", "Robert C. Martin", "978-0132350884", "Technology", 2008);
    add_sample(total, "Dom Casmurro", "Machado de Assis", "978-8535917239", "Literature", 1899);

    /* some made-up borrows so the report isn't empty */
    collection[0].times_borrowed = 5;
    collection[1].times_borrowed = 8;
    collection[2].times_borrowed = 12;
    collection[2].available = 0;

    save_books(DB_FILE, collection, *total);
    printf("%d sample books added!\n", *total);
}

static void show_result(int code, const char *success) {
    switch (code) {
        case OK:            printf("%s\n", success); break;
        case ERR_NOT_FOUND: printf("Book not found.\n"); break;
        case ERR_BORROWED:  printf("The book is borrowed.\n"); break;
        case ERR_AVAILABLE: printf("This book isn't borrowed.\n"); break;
        default:            printf("Operation not done.\n");
    }
}

static void screen_search(SearchField field) {
    char term[MAX_TITLE];
    int results[MAX_BOOKS];

    const char *prompt = field == FIELD_AUTHOR ? "Type the author (or part of it)"
                       : field == FIELD_GENRE  ? "Type the genre (or part of it)"
                       : "Type the title (or part of it)";
    read_string(prompt, term, MAX_TITLE);
    int count = search_books(collection, book_count, field, term, results);
    if (count == 0) {
        printf("No books found.\n");
        return;
    }
    printf("\n%d result(s):\n", count);
    for (int i = 0; i < count; i++) {
        const Book *b = &collection[results[i]];
        printf("  [%d] %s - %s (%s)\n",
               b->id, b->title, b->author,
               b->available ? "Available" : "Borrowed");
    }
}

int main(void) {
    int *total = &book_count;
    int option = -1;

    printf("=================================\n");
    printf("  LIBRARY MANAGEMENT\n");
    printf("  SYSTEM\n");
    printf("=================================\n");

    int loaded = load_books(DB_FILE, collection, total);
    if (loaded == 1) {
        printf("Data loaded: %d book(s) in the collection.\n", *total);
    } else if (loaded == -1) {
        printf("Warning: %s is corrupted. Renaming it to %s.bak and starting from scratch.\n",
               DB_FILE, DB_FILE);
        remove(DB_FILE ".bak");
        rename(DB_FILE, DB_FILE ".bak");
        add_samples(total);
    } else {
        printf("No saved data. Starting an empty collection.\n");
        add_samples(total);
    }

    while (option != 0) {
        printf("\n=================================\n");
        printf("  MAIN MENU\n");
        printf("=================================\n");
        printf(" 1. Add book\n");
        printf(" 2. List all books\n");
        printf(" 3. Search by title\n");
        printf(" 4. Search by author\n");
        printf(" 5. Edit book\n");
        printf(" 6. Borrow book\n");
        printf(" 7. Return book\n");
        printf(" 8. Remove book\n");
        printf(" 9. Available books\n");
        printf("10. Most borrowed\n");
        printf("11. Collection summary\n");
        printf("12. Search by genre\n");
        printf(" 0. Quit\n\n");

        option = read_int("Choose", 0, 12);

        switch (option) {
            case 1:
                screen_add(collection, total);
                break;

            case 2:
                list_books(collection, *total);
                break;

            case 3:
                screen_search(FIELD_TITLE);
                break;

            case 4:
                screen_search(FIELD_AUTHOR);
                break;

            case 5:
                screen_edit(collection, *total);
                break;

            case 6: {
                int id = read_int("ID of the book to borrow", 1, 99999);
                int r = borrow_book(collection, *total, id);
                if (r == OK) save_books(DB_FILE, collection, *total);
                show_result(r, "Book borrowed!");
                break;
            }

            case 7: {
                int id = read_int("ID of the book to return", 1, 99999);
                int r = return_book(collection, *total, id);
                if (r == OK) save_books(DB_FILE, collection, *total);
                show_result(r, "Book returned!");
                break;
            }

            case 8: {
                int id = read_int("ID of the book to remove", 1, 99999);
                int r = remove_book(collection, total, id);
                if (r == OK) save_books(DB_FILE, collection, *total);
                show_result(r, "Book removed.");
                break;
            }

            case 9:
                report_available(collection, *total);
                break;

            case 10:
                report_most_borrowed(collection, *total);
                break;

            case 11:
                report_summary(collection, *total);
                break;

            case 12:
                screen_search(FIELD_GENRE);
                break;

            case 0:
                printf("\nQuitting... Data saved. Bye!\n");
                break;
        }

        if (option != 0) wait_enter();
    }

    return 0;
}
