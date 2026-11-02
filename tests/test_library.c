/* make test. No framework: CHECK counts what passed and shows what failed */

#include "../src/library.h"

static int passed = 0, failed = 0;

#define CHECK(cond) do {                                              \
    if (cond) { passed++; }                                           \
    else { failed++; printf("  FAILED  %s:%d  %s\n", __FILE__, __LINE__, #cond); } \
} while (0)

static Book collection[MAX_BOOKS];
static Book loaded[MAX_BOOKS];

static Book new_book(const char *title, const char *author, int year) {
    Book b = {0};
    snprintf(b.title, MAX_TITLE, "%s", title);
    snprintf(b.author, MAX_AUTHOR, "%s", author);
    b.year = year;
    return b;
}

static int fill(void) {
    int total = 0;
    Book a = new_book("Clean Code", "Robert C. Martin", 2008);
    Book b = new_book("Dom Casmurro", "Machado de Assis", 1899);
    Book c = new_book("Memórias Póstumas de Brás Cubas", "Machado de Assis", 1881);
    add_book(collection, &total, &a);
    add_book(collection, &total, &b);
    add_book(collection, &total, &c);
    return total;
}

static void test_add_generates_ids(void) {
    int total = fill();
    CHECK(total == 3);
    CHECK(collection[0].id == 1 && collection[2].id == 3);
    CHECK(collection[1].available == 1);
    CHECK(collection[1].times_borrowed == 0);

    /* after removing the last one, the id goes back to max+1 */
    CHECK(remove_book(collection, &total, 3) == OK);
    Book d = new_book("New", "Author", 2020);
    CHECK(add_book(collection, &total, &d) == 3);
}

static void test_same_isbn_is_refused(void) {
    int total = 0;
    Book a = new_book("Dom Casmurro", "Machado de Assis", 1899);
    Book b = new_book("Dom Casmurro (again)", "Machado de Assis", 1899);
    Book c = new_book("No ISBN", "Someone", 2000);
    Book d = new_book("Also no ISBN", "Someone", 2001);
    snprintf(a.isbn, MAX_ISBN, "978-85-359-0277-1");
    snprintf(b.isbn, MAX_ISBN, "9788535902771");

    CHECK(add_book(collection, &total, &a) == 1);
    CHECK(add_book(collection, &total, &b) == ERR_DUPLICATE);
    CHECK(total == 1);
    CHECK(add_book(collection, &total, &c) == 2);
    CHECK(add_book(collection, &total, &d) == 3);
    CHECK(find_by_isbn(collection, total, " 978 8535902771 ") == &collection[0]);
    CHECK(find_by_isbn(collection, total, "") == NULL);
}

static void test_search_is_case_insensitive(void) {
    int total = fill();
    int r[MAX_BOOKS];

    CHECK(search_books(collection, total, FIELD_TITLE, "clean", r) == 1 && r[0] == 0);
    CHECK(search_books(collection, total, FIELD_TITLE, "CASMURRO", r) == 1 && r[0] == 1);
    CHECK(search_books(collection, total, FIELD_AUTHOR, "machado", r) == 2);
    CHECK(search_books(collection, total, FIELD_TITLE, "nonexistent", r) == 0);

    snprintf(collection[1].genre, MAX_GENRE, "Literature");
    snprintf(collection[2].genre, MAX_GENRE, "Brazilian literature");
    CHECK(search_books(collection, total, FIELD_GENRE, "LITERATURE", r) == 2 && r[0] == 1);
    CHECK(search_books(collection, total, FIELD_TITLE, "", r) == 3);

    CHECK(contains_ignore_case("Clean Code", "N CO"));
    CHECK(!contains_ignore_case("abc", "abcd"));
}

static void test_borrow_and_return(void) {
    int total = fill();

    CHECK(borrow_book(collection, total, 1) == OK);
    CHECK(collection[0].available == 0 && collection[0].times_borrowed == 1);
    CHECK(borrow_book(collection, total, 1) == ERR_BORROWED);
    CHECK(borrow_book(collection, total, 99) == ERR_NOT_FOUND);

    CHECK(return_book(collection, total, 1) == OK);
    CHECK(return_book(collection, total, 1) == ERR_AVAILABLE);
    CHECK(borrow_book(collection, total, 1) == OK);
    CHECK(collection[0].times_borrowed == 2);
}

static void test_remove_keeps_order(void) {
    int total = fill();

    CHECK(borrow_book(collection, total, 2) == OK);
    CHECK(remove_book(collection, &total, 2) == ERR_BORROWED);
    CHECK(total == 3);

    CHECK(return_book(collection, total, 2) == OK);
    CHECK(remove_book(collection, &total, 1) == OK);
    CHECK(total == 2);
    CHECK(collection[0].id == 2 && collection[1].id == 3);
    CHECK(remove_book(collection, &total, 1) == ERR_NOT_FOUND);
}

static void test_collection_limit(void) {
    int total = MAX_BOOKS;
    Book b = new_book("X", "Y", 2000);
    CHECK(add_book(collection, &total, &b) == ERR_FULL);
    CHECK(total == MAX_BOOKS);
}

static void test_save_and_load(void) {
    const char *file = "test_library.dat";
    int total = fill();
    borrow_book(collection, total, 3);

    CHECK(save_books(file, collection, total) == 1);

    int total_read = -1;
    CHECK(load_books(file, loaded, &total_read) == 1);
    CHECK(total_read == 3);
    CHECK(strcmp(loaded[2].title, "Memórias Póstumas de Brás Cubas") == 0);
    CHECK(loaded[2].available == 0 && loaded[2].times_borrowed == 1);

    remove(file);
    CHECK(load_books(file, loaded, &total_read) == 0);
    CHECK(total_read == 0);
}

static void test_corrupted_file(void) {
    const char *file = "test_corrupted.dat";
    int total_read = -1;

    /* total bigger than the array */
    FILE *f = fopen(file, "wb");
    int fake = MAX_BOOKS * 50;
    fwrite(&fake, sizeof(int), 1, f);
    fclose(f);
    CHECK(load_books(file, loaded, &total_read) == -1);
    CHECK(total_read == 0);

    /* says it has 2 books but the file ends before that */
    f = fopen(file, "wb");
    int two = 2;
    fwrite(&two, sizeof(int), 1, f);
    fwrite(&collection[0], sizeof(Book), 1, f);
    fclose(f);
    CHECK(load_books(file, loaded, &total_read) == -1);

    remove(file);
}

static void test_column_counts_characters_not_bytes(void) {
    char buf[COLUMN_SIZE(10)];

    /* "ç" and "ã" take 2 bytes each, but count as 1 character */
    format_column(buf, "Ação", 6);
    CHECK(strcmp(buf, "Ação  ") == 0);

    /* cuts with "..." at the end, without splitting the "ó" in half */
    format_column(buf, "Memórias Póstumas", 8);
    CHECK(strcmp(buf, "Memór...") == 0);

    /* text with exactly the width isn't cut */
    format_column(buf, "Memórias", 8);
    CHECK(strcmp(buf, "Memórias") == 0);

    format_column(buf, "", 3);
    CHECK(strcmp(buf, "   ") == 0);

    /* text that ends in the middle of a character can't be read past the end */
    format_column(buf, "a\xc3", 4);
    CHECK(strcmp(buf, "a\xc3  ") == 0);
}

static void test_sort_orders(void) {
    int total = fill();
    Book d = new_book("anna karenina", "Leo Tolstoy", 1878);
    add_book(collection, &total, &d);
    const Book *out[MAX_BOOKS];

    sort_books(collection, total, ORDER_TITLE, out);
    /* lower case "anna" still comes first */
    CHECK(strcmp(out[0]->title, "anna karenina") == 0);
    CHECK(strcmp(out[1]->title, "Clean Code") == 0);

    sort_books(collection, total, ORDER_YEAR, out);
    CHECK(out[0]->year == 1878 && out[3]->year == 2008);

    /* same author: by title */
    sort_books(collection, total, ORDER_AUTHOR, out);
    CHECK(strcmp(out[0]->author, "Leo Tolstoy") == 0);
    CHECK(strcmp(out[1]->title, "Dom Casmurro") == 0);
    CHECK(strcmp(out[2]->title, "Memórias Póstumas de Brás Cubas") == 0);

    /* the collection itself isn't touched */
    sort_books(collection, total, ORDER_ID, out);
    CHECK(out[0] == &collection[0] && collection[3].id == 4);
}

static void test_export_csv(void) {
    const char *file = "test_export.csv";
    int total = fill();
    snprintf(collection[0].title, MAX_TITLE, "%s", "The \"Clean\" Code, 2nd");
    borrow_book(collection, total, 2);
    CHECK(export_csv(file, collection, total) == 1);

    char content[2048] = {0};
    FILE *f = fopen(file, "rb");
    size_t n = fread(content, 1, sizeof(content) - 1, f);
    fclose(f);
    remove(file);
    content[n] = '\0';

    CHECK(strncmp(content, "\xEF\xBB\xBFid,title,author", 18) == 0);
    /* quotes doubled and the comma stays inside the field */
    CHECK(strstr(content, "1,\"The \"\"Clean\"\" Code, 2nd\",\"Robert C. Martin\"") != NULL);
    CHECK(strstr(content, ",1899,borrowed,1\r\n") != NULL);
    CHECK(strstr(content, "Memórias Póstumas") != NULL);
    CHECK(export_csv("no/such/folder/x.csv", collection, total) == 0);
}

int main(void) {
    test_add_generates_ids();
    test_same_isbn_is_refused();
    test_search_is_case_insensitive();
    test_borrow_and_return();
    test_remove_keeps_order();
    test_collection_limit();
    test_save_and_load();
    test_corrupted_file();
    test_column_counts_characters_not_bytes();
    test_sort_orders();
    test_export_csv();

    printf("\nResult: %d check(s) passed, %d failed.\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
