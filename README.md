# B+ Tree Database Implementation

Ένα C implementation ενός B+ Tree για διαχείριση και ευρετηρίαση εγγραφών (records) σε αρχεία.

## Δομή του Έργου

### Core Components

#### 1. **bplus_file_funcs.c** - Κύριες Λειτουργίες Αρχείου
Διαχειρίζεται τις κύριες πράξεις του B+ Tree:
- `bplus_create_file()` - Δημιουργία νέου αρχείου βάσης δεδομένων
- `bplus_open_file()` - Άνοιγμα υπάρχοντος αρχείου
- `bplus_close_file()` - Κλείσιμο αρχείου με αποθήκευση μεταδεδομένων
- `bplus_record_insert()` - Εισαγωγή εγγραφής στο δέντρο
- `bplus_record_find()` - Αναζήτηση εγγραφής κατά κλειδί

**Κύρια χαρακτηριστικά:**
- Αναδρομική διαπέραση του δέντρου
- Αυτόματη διάσπαση κόμβων όταν γεμίσουν
- Διαχείριση μεταδεδομένων (root, height, schema)
- Χρήση BF (Buffer Frame) library για disk I/O

#### 2. **bplus_index_node.c** - Δείκτες Ευρετηρίου
Διαχειρίζεται τους εσωτερικούς κόμβους του δέντρου:
- `indexnode_init()` - Αρχικοποίηση κόμβου ευρετηρίου
- `indexnode_get_key()` - Ανάκτηση κλειδιού
- `indexnode_set_key()` - Ορισμός κλειδιού
- `indexnode_get_pointer()` / `indexnode_set_pointer()` - Διαχείριση δεικτών παιδιών
- `indexnode_add_key_pointer()` - Προσθήκη ζεύγους κλειδιού-δείκτη
- `indexnode_find_child()` - Εύρεση κατάλληλου παιδιού για κλειδί
- `indexnode_split()` - Διάσπαση κόμβου ευρετηρίου

**Δομή δεδομένων:**
```c
typedef struct {
  int num_keys;                    // Αριθμός κλειδιών
  int is_leaf;                     // Είναι κόμβος φύλλου;
  int max_keys;                    // Μέγιστο όριο κλειδιών
  char data[...];                  // Αποθήκευση κλειδιών και δεικτών
} BPlusIndexNode;
```

#### 3. **bplus_datanode.c** - Κόμβοι Δεδομένων
Διαχειρίζεται τους κόμβους φύλλων που αποθηκεύουν εγγραφές:
- `datanode_init()` - Αρχικοποίηση κόμβου δεδομένων
- `datanode_get_num_records()` - Πλήθος εγγραφών
- `datanode_get_record()` - Ανάκτηση εγγραφής
- `datanode_set_record()` - Ορισμός εγγραφής
- `datanode_add_record()` - Προσθήκη εγγραφής
- `datanode_split()` - Διάσπαση κόμβου δεδομένων
- `datanode_get_next_leaf()` / `datanode_set_next_leaf()` - Διαχείριση σύνδεσμων φύλλων

**Δομή δεδομένων:**
```c
typedef struct {
  int num_records;                 // Αριθμός εγγραφών
  int next_leaf;                   // Επόμενο φύλλο (-1 αν τελευταίο)
  int prev_leaf;                   // Προηγούμενο φύλλο
  char data[...];                  // Αποθήκευση εγγραφών
} BPlusDataNode;
```

## Λειτουργία του B+ Tree

### Δομή Αρχείου
- **Block 0**: Μεταδεδομένα (root block ID, ύψος δέντρου, schema)
- **Block 1+**: Κόμβοι ευρετηρίου και δεδομένων

### Εισαγωγή Εγγραφής
1. Αν το δέντρο είναι κενό, δημιουργία αρχικών κόμβων
2. Διαπέραση από root έως κατάλληλο φύλλο
3. Εισαγωγή εγγραφής με διατήρηση ταξινόμησης
4. Αν γεμίσει κόμβος, διάσπαση και ανύψωση κλειδιού

### Αναζήτηση Εγγραφής
1. Έναρξη από root node
2. Διαπέραση δέντρου χρησιμοποιώντας κλειδιά για πλοήγηση
3. Αναζήτηση στο φύλλο
4. Επιστροφή αποτελέσματος

## Κύριες Δομές Δεδομένων

### BPlusMeta (Μεταδεδομένα)
```c
typedef struct {
  int root_block;        // Αναγνωριστικό μπλοκ root
  int height;            // Ύψος δέντρου
  int file_desc;         // Διακριτικό αρχείου
  TableSchema schema;    // Σχήμα πίνακα
} BPlusMeta;
```

### Record (Εγγραφή)
```c
typedef struct {
  FieldValue values[MAX_ATTRIBUTES];  // Τιμές πεδίων
} Record;
```

## Σχεδίαση Κόμβων

### Χωρητικότητα
- **Δεδομένων**: `(BF_BLOCK_SIZE - 3*sizeof(int)) / sizeof(Record)` εγγραφές
- **Ευρετηρίου**: `(BF_BLOCK_SIZE - 2*sizeof(int) - sizeof(int)) / (2*sizeof(int))` κλειδιά

### Λειτουργίες Διάσπασης
- Όταν κόμβος φύλλου γεμίσει: Διαίρεση εγγραφών, ανύψωση μεσαίου κλειδιού
- Όταν κόμβος ευρετηρίου γεμίσει: Διαίρεση κλειδιών, ανύψωση μεσαίου κλειδιού στον γονέα
- Αν root διασπαθεί: Δημιουργία νέας root, αύξηση ύψους

## Ροή Επεξεργασίας

```
insert_record
├── Αν δέντρο κενό: δημιουργία αρχικής δομής
├── insert_recursive
│   ├── Εάν φύλλο: insert_into_leaf
│   │   ├── Εύρεση ταξινομημένης θέσης
│   │   ├── Εάν γεμάτο: split + recursive insert
│   │   └── Διαφορετικά: απλή εισαγωγή
│   └── Εάν ευρετήριο: πλοήγηση σε παιδί
└── Εάν root διασπάθηκε: δημιουργία νέας root
```

## Χρήση

### Δημιουργία Βάσης
```c
TableSchema schema = {...};
bplus_create_file(&schema, "database.db");
```

### Άνοιγμα/Κλείσιμο
```c
int file_desc;
BPlusMeta *metadata;
bplus_open_file("database.db", &file_desc, &metadata);
// ... λειτουργίες ...
bplus_close_file(file_desc, metadata);
```

### Εισαγωγή Εγγραφής
```c
Record record = {...};
bplus_record_insert(file_desc, metadata, &record);
```

### Αναζήτηση
```c
Record *result = NULL;
bplus_record_find(file_desc, metadata, key, &result);
```

## Βιβλιοθήκες Εξάρτησης

- **BF (Buffer Frame)**: Διαχείριση disk blocks με LRU caching
- **Standard C Library**: stdio, stdlib, string

## Σημειώσεις Υλοποίησης

- Όλοι οι κόμβοι αποθηκεύονται σε blocks του αρχείου
- Τα μεταδεδομένα ενημερώνονται στο block 0 μετά από διασπάσεις
- Χρησιμοποιείται σχήμα-ευέλικτη αποθήκευση εγγραφών
- Διατηρούνται σύνδεσμοι φύλλων για sequential access
