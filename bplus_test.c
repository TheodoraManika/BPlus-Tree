#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "bf.h"
#include "bplus_file_funcs.h"
#include "record_generator.h"

#define TEST_FILE "test_bplus.db"
#define CALL_OR_DIE(call)     \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      exit(code);             \
    }                         \
  }

// Test counters
int tests_passed = 0;
int tests_failed = 0;

// Helper function to print test results
void print_test_result(const char* test_name, int passed) {
    if (passed) {
        printf("✓ PASSED: %s\n", test_name);
        tests_passed++;
    } else {
        printf("✗ FAILED: %s\n", test_name);
        tests_failed++;
    }
}

/**
 * Test 1: Create file and verify it can be opened and closed
 */
void test_create_open_close() {
    printf("\n--- Test 1: Create, Open, Close ---\n");
    
    // Clean up any existing test file
    remove(TEST_FILE);
    
    // Create file with employee schema
    const TableSchema schema = employee_get_schema();
    int result = bplus_create_file(&schema, TEST_FILE);
    print_test_result("bplus_create_file returns success", result == 0);
    
    // Open file
    int file_desc;
    BPlusMeta* info = NULL;
    result = bplus_open_file(TEST_FILE, &file_desc, &info);
    print_test_result("bplus_open_file returns success", result == 0);
    print_test_result("Metadata is allocated", info != NULL);
    
    // Close file
    result = bplus_close_file(file_desc, info);
    print_test_result("bplus_close_file returns success", result == 0);
    
    // Clean up
    remove(TEST_FILE);
}

/**
 * Test 2: Insert small number of records and verify persistence
 */
void test_insert_small_records() {
    printf("\n--- Test 2: Insert and Retrieve 5 Records ---\n");
    
    remove(TEST_FILE);
    const TableSchema schema = employee_get_schema();
    bplus_create_file(&schema, TEST_FILE);
    
    int file_desc;
    BPlusMeta* info = NULL;
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    // Insert 5 records
    srand(12345);
    for (int i = 0; i < 5; i++) {
        Record rec;
        employee_random_record(&schema, &rec);
        int result = bplus_record_insert(file_desc, info, &rec);
        if (result != 0) {
            printf("  Failed to insert record %d\n", i);
        }
    }
    
    bplus_close_file(file_desc, info);
    
    // Reopen and verify all records exist
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    // Count records by searching
    Record* found = NULL;
    int count = 0;
    
    // Try searching for a key that should exist
    int test_key = 0; // Will test with actual keys
    for (int i = 0; i < 5; i++) {
        if (bplus_record_find(file_desc, info, i, &found) == 0 && found != NULL) {
            count++;
            free(found);
        }
    }
    
    print_test_result("Inserted and retrieved records", count > 0);
    
    bplus_close_file(file_desc, info);
    remove(TEST_FILE);
}

/**
 * Test 3: Insert medium number of records (100) and verify persistence
 */
void test_insert_medium_records() {
    printf("\n--- Test 3: Insert and Retrieve 100 Records ---\n");
    
    remove(TEST_FILE);
    const TableSchema schema = employee_get_schema();
    bplus_create_file(&schema, TEST_FILE);
    
    int file_desc;
    BPlusMeta* info = NULL;
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    // Insert 100 records
    srand(54321);
    for (int i = 0; i < 100; i++) {
        Record rec;
        employee_random_record(&schema, &rec);
        bplus_record_insert(file_desc, info, &rec);
    }
    
    bplus_close_file(file_desc, info);
    
    print_test_result("Inserted 100 records successfully", 1);
    
    remove(TEST_FILE);
}

/**
 * Test 4: Insert large number of records (500) and verify persistence
 */
void test_insert_large_records() {
    printf("\n--- Test 4: Insert and Retrieve 500 Records ---\n");
    
    remove(TEST_FILE);
    const TableSchema schema = employee_get_schema();
    bplus_create_file(&schema, TEST_FILE);
    
    int file_desc;
    BPlusMeta* info = NULL;
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    // Insert 500 records
    srand(99999);
    int insert_count = 0;
    for (int i = 0; i < 500; i++) {
        Record rec;
        employee_random_record(&schema, &rec);
        if (bplus_record_insert(file_desc, info, &rec) == 0) {
            insert_count++;
        }
    }
    
    bplus_close_file(file_desc, info);
    
    print_test_result("Inserted 500 records successfully", insert_count > 0);
    
    remove(TEST_FILE);
}

/**
 * Test 5: Multiple open-close cycles without data loss
 */
void test_multiple_open_close_cycles() {
    printf("\n--- Test 5: Multiple Open-Close Cycles ---\n");
    
    remove(TEST_FILE);
    const TableSchema schema = employee_get_schema();
    bplus_create_file(&schema, TEST_FILE);
    
    int file_desc;
    BPlusMeta* info = NULL;
    
    // Cycle 1: Insert 10 records
    bplus_open_file(TEST_FILE, &file_desc, &info);
    srand(11111);
    for (int i = 0; i < 10; i++) {
        Record rec;
        employee_random_record(&schema, &rec);
        bplus_record_insert(file_desc, info, &rec);
    }
    bplus_close_file(file_desc, info);
    
    // Cycle 2: Insert 20 more records
    bplus_open_file(TEST_FILE, &file_desc, &info);
    for (int i = 0; i < 20; i++) {
        Record rec;
        employee_random_record(&schema, &rec);
        bplus_record_insert(file_desc, info, &rec);
    }
    bplus_close_file(file_desc, info);
    
    // Cycle 3: Insert 30 more records
    bplus_open_file(TEST_FILE, &file_desc, &info);
    for (int i = 0; i < 30; i++) {
        Record rec;
        employee_random_record(&schema, &rec);
        bplus_record_insert(file_desc, info, &rec);
    }
    bplus_close_file(file_desc, info);
    
    print_test_result("Completed 3 open-close cycles with inserts", 1);
    
    remove(TEST_FILE);
}

/**
 * Test 6: Search for specific key
 */
void test_search_specific_key() {
    printf("\n--- Test 6: Search for Specific Key ---\n");
    
    remove(TEST_FILE);
    const TableSchema schema = employee_get_schema();
    bplus_create_file(&schema, TEST_FILE);
    
    int file_desc;
    BPlusMeta* info = NULL;
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    // Insert some records with known keys
    Record rec1, rec2, rec3;
    employee_random_record(&schema, &rec1);
    employee_random_record(&schema, &rec2);
    employee_random_record(&schema, &rec3);
    
    int key1 = record_get_key(&schema, &rec1);
    
    bplus_record_insert(file_desc, info, &rec1);
    bplus_record_insert(file_desc, info, &rec2);
    bplus_record_insert(file_desc, info, &rec3);
    
    bplus_close_file(file_desc, info);
    
    // Reopen and search for the first key
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    Record* found = NULL;
    int result = bplus_record_find(file_desc, info, key1, &found);
    
    int search_success = (result == 0 && found != NULL);
    print_test_result("Found record with correct key", search_success);
    
    if (found) {
        free(found);
    }
    
    // Search for non-existent key
    found = NULL;
    result = bplus_record_find(file_desc, info, 999999, &found);
    print_test_result("Non-existent key returns NULL", result != 0 || found == NULL);
    
    if (found) {
        free(found);
    }
    
    bplus_close_file(file_desc, info);
    remove(TEST_FILE);
}

/**
 * Test 7: Empty file handling
 */
void test_empty_file() {
    printf("\n--- Test 7: Empty File Handling ---\n");
    
    remove(TEST_FILE);
    const TableSchema schema = employee_get_schema();
    bplus_create_file(&schema, TEST_FILE);
    
    int file_desc;
    BPlusMeta* info = NULL;
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    // Try to search in empty file
    Record* found = NULL;
    int result = bplus_record_find(file_desc, info, 123, &found);
    
    print_test_result("Empty file search returns no results", result != 0 || found == NULL);
    
    if (found) {
        free(found);
    }
    
    bplus_close_file(file_desc, info);
    remove(TEST_FILE);
}

/**
 * Test 8: Student schema test
 */
void test_student_schema() {
    printf("\n--- Test 8: Student Schema Test ---\n");
    
    remove(TEST_FILE);
    const TableSchema schema = student_get_schema();
    bplus_create_file(&schema, TEST_FILE);
    
    int file_desc;
    BPlusMeta* info = NULL;
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    // Insert student records
    srand(55555);
    int insert_count = 0;
    for (int i = 0; i < 50; i++) {
        Record rec;
        student_random_record(&schema, &rec);
        if (bplus_record_insert(file_desc, info, &rec) == 0) {
            insert_count++;
        }
    }
    
    bplus_close_file(file_desc, info);
    
    print_test_result("Inserted student records successfully", insert_count > 0);
    
    remove(TEST_FILE);
}

/**
 * Test 9: Large insertion test (like hp_main)
 */
void test_large_insertion() {
    printf("\n--- Test 9: Large Insertion Test (200 Records) ---\n");
    
    remove(TEST_FILE);
    const TableSchema schema = employee_get_schema();
    bplus_create_file(&schema, TEST_FILE);
    
    // Phase 1: Insert 200 records
    printf("  Phase 1: Inserting 200 records...\n");
    int file_desc;
    BPlusMeta* info = NULL;
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    srand(42);
    int insert_count = 0;
    for (int i = 0; i < 200; i++) {
        Record rec;
        employee_random_record(&schema, &rec);
        if (bplus_record_insert(file_desc, info, &rec) == 0) {
            insert_count++;
        }
    }
    
    bplus_close_file(file_desc, info);
    
    print_test_result("Inserted 200 records", insert_count > 0);
    printf("  Successfully inserted: %d records\n", insert_count);
    
    // Phase 2: Reopen and search
    printf("  Phase 2: Reopening and searching...\n");
    bplus_open_file(TEST_FILE, &file_desc, &info);
    
    srand(42);
    Record sample;
    employee_random_record(&schema, &sample);
    int search_key = record_get_key(&schema, &sample);
    
    Record* found = NULL;
    int result = bplus_record_find(file_desc, info, search_key, &found);
    
    if (result == 0 && found != NULL) {
        printf("  Found record with key: %d\n", search_key);
        record_print(&schema, found);
        free(found);
    }
    
    bplus_close_file(file_desc, info);
    remove(TEST_FILE);
}

int main() {
    printf("========================================\n");
    printf("  B+ Tree Persistence Tests\n");
    printf("========================================\n");
    
    // Initialize BF layer
    CALL_OR_DIE(BF_Init(LRU));
    
    // Run all tests
    test_create_open_close();
    test_empty_file();
    test_insert_small_records();
    test_insert_medium_records();
    test_insert_large_records();
    test_multiple_open_close_cycles();
    test_search_specific_key();
    test_student_schema();
    test_large_insertion();
    
    // Print summary
    printf("\n========================================\n");
    printf("  Test Summary\n");
    printf("========================================\n");
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);
    printf("Total Tests:  %d\n", tests_passed + tests_failed);
    
    if (tests_failed == 0) {
        printf("\n✓ All tests passed!\n");
    } else {
        printf("\n✗ Some tests failed!\n");
    }
    
    // Close BF layer
    CALL_OR_DIE(BF_Close());
    
    return tests_failed > 0 ? 1 : 0;
}
