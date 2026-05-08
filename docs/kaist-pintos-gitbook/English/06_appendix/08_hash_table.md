# 08_hash_table

# Hash Table

Pintos provides a hash table data structure in `lib/kernel/hash.c`. To use it
you will need to include its header file, '`lib/kernel/hash.h`', with
`#include <hash.h>`. No code provided with Pintos uses the hash table, which
means that you are free to use it as is, modify its implementation for your own
purposes, or ignore it, as you wish.

Most implementations of the virtual memory project use a hash table to translate
pages to frames. You may find other uses for hash tables as well.

## Data Types

A hash table is represented by struct hash.

---

```c
struct hash;
```

> Represents an entire hash table. The actual members of `struct hash` are
> "opaque." That is, code that uses a hash table should not access `struct hash`
> members directly, nor should it need to. Instead, use hash table functions
> and macros.

The hash table operates on elements of type `struct hash_elem`.

---

```c
struct hash_elem;
```

> Embed a `struct hash_elem` member in the structure you want to include in a
> hash table. Like `struct hash`, `struct hash_elem` is opaque. All functions
> for operating on hash table elements actually take and return pointers to
> `struct hash_elem`, not pointers to your hash table's real element type.

You will often need to obtain a `struct hash_elem` given a real element of the
hash table, and vice versa. Given a real element of the hash table, you may use
the `&` operator to obtain a pointer to its `struct hash_elem`. Use the
`hash_entry()` macro to go the other direction.

---

```c
#define hash_entry (elem, type, member) { /* Omit details */ }
```

> Returns a pointer to the structure that elem, a pointer to a
> `struct hash_elem`, is embedded within. You must provide type, the name of the
> structure that elem is inside, and member, the name of the member in type that
> elem points to.
>
> For example, suppose `h` is a `struct hash_elem *` variable that points to a
> struct thread member (of type `struct hash_elem`) named `h_elem`. Then,
> `hash_entry` (`h, struct thread, h_elem`) yields the address of the
> `struct thread` that `h` points within.

Each hash table element must contain a key, that is, data that identifies and
distinguishes elements, which must be unique among elements in the hash table.
(Elements may also contain non-key data that need not be unique.) While an
element is in a hash table, its key data must not be changed. Instead, if need
be, remove the element from the hash table, modify its key, then reinsert the
element.

For each hash table, you must write two functions that act on keys: a hash
function and a comparison function. These functions must match the following
prototypes:

---

```c
typedef unsigned hash_hash_func (const struct hash_elem *e, void *aux);
```

> Returns a hash of element's data, as a value anywhere in the range of unsigned
> int. The hash of an element should be a pseudo-random function of the
> element's key. It must not depend on non-key data in the element or on any
> non-constant data other than the key. Pintos provides the following functions
> as a suitable basis for hash functions.
>
> `unsigned hash_bytes (const void *buf, size t *size)`
>
>> Returns a hash of the size bytes starting at buf. The implementation is the
>> general-purpose Fowler-Noll-Vo hash
>> (`http://en.wikipedia.org/wiki/Fowler_Noll_Vo_hash`) for 32-bit words.
>>
>
> `unsigned hash_string (const char *s)`
>
>> Returns a hash of null-terminated string s.
>>
>
> `unsigned hash_int (int i)`
>
>> Returns a hash of integer i.
>>
>
> If your key is a single piece of data of an appropriate type, it is sensible
> for your hash function to directly return the output of one of these
> functions. For multiple pieces of data, you may wish to combine the output of
> more than one call to them using, e.g., the '^' (exclusive or) operator.
> Finally, you may entirely ignore these functions and write your own hash
> function from scratch, but remember that your goal is to build an operating
> system kernel, not to design a hash function. See Section
> [Hash Auxiliary Data], for an explanation of aux.
>
> `bool hash_less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux)`
>
>> Compares the keys stored in elements a and b. Returns true if a is less than
>> b, false if a is greater than or equal to b.
>> If two elements compare equal, then they must hash to equal values.
>>
>
> See Section [Hash Auxiliary Data], for an explanation of aux.
> See Section [Hash Table Example], for hash and comparison function examples.
> A few functions accept a pointer to a third kind of function as an argument:
>
> `void hash_action_func (struct hash_elem *element, void *aux)`
>
>> Performs some kind of action, chosen by the caller, on element.
>> See Section [Hash Auxiliary Data], for an explanation of aux.
>>

## Basic Functions

These functions create, destroy, and inspect hash tables.

---

```c
bool hash_init (struct hash *hash, hash_hash_func *hash_func,
                    hash_less_func *less_func, void *aux)
```

> Initializes hash as a hash table with hash func as hash function, less func as
> comparison function, and aux as auxiliary data. Returns true if successful,
> false on failure. `hash_init()` calls `malloc()` and fails if memory cannot be
> allocated. See Hash Auxiliary Data, for an explanation of aux, which is
> most often a null pointer.

---

```c
void hash_clear (struct hash *hash, hash_action_func *action)
```

> Removes all the elements from hash, which must have been previously
> initialized with `hash_init()`.
> If action is non-null, then it is called once for each element in the hash
> table, which gives the caller an opportunity to deallocate any memory or other
> resources used by the element. For example, if the hash table elements are
> dynamically allocated using `malloc()`, then action could `free()` the
> element. This is safe because `hash_clear()` will not access the memory in a
> given hash element after calling action on it. However, action must not call
> any function that may modify the hash table, such as `hash_insert()` or
> `hash_delete()`.

---

```c
void hash_destroy (struct hash *hash, hash_action_func *action);
```

> If action is non-null, calls it for each element in the hash, with the same
> semantics as a call to `hash_clear()`. Then, frees the memory held by hash.
> Afterward, hash must not be passed to any hash table function, absent an
> intervening call to hash_init().

---

```c
size_t hash_size (struct hash *hash);
```

> Returns the number of elements currently stored in hash.

---

```c
bool hash_empty (struct hash *hash);
```

> Returns true if hash currently contains no elements, false if hash contains at
> least one element.

## Search Functions

Each of these functions searches a hash table for an element that compares equal
to one provided. Based on the success of the search, they perform some action,
such as inserting a new element into the hash table, or simply return the result
of the search.

---

```c
struct hash_elem *hash_insert (struct hash *hash, struct hash elem *element);
```

> Searches hash for an element equal to element. If none is found, inserts
> element into hash and returns a null pointer. If the table already contains an
> element equal to element, it is returned without modifying hash.

---

```c
struct hash_elem *hash_replace (struct hash *hash, struct hash elem *element);
```

> Inserts element into hash. Any element equal to element already in hash is removed.
> Returns the element removed, or a null pointer if hash did not contain an element
> equal to element.
>
> The caller is responsible for deallocating any resources associated with the returned
> element, as appropriate. For example, if the hash table elements are dynamically
> allocated using `malloc()`, then the caller must `free()` the element after it is no
> longer needed.

The element passed to the following functions is only used for hashing and
comparison purposes. It is never actually inserted into the hash table. Thus,
only key data in the element needs to be initialized, and other data in the
element will not be used. It often makes sense to declare an instance of the
element type as a local variable, initialize the key data, and then pass the
address of its `struct hash_elem` to `hash_find()` or `hash_delete()`. See
Hash Table Example, for an example. (Large
structures should not be allocated as local variables. See
[struct thread](01_threads.md), for more information.)

---

```c
struct hash_elem *hash_find (struct hash *hash, struct hash elem *element);
```

> Searches hash for an element equal to element. Returns the element found,
> if any, or a null pointer otherwise.

---

```c
struct hash_elem *hash_delete (struct hash *hash, struct hash elem *element);
```

> Searches hash for an element equal to element. If one is found, it is removed
> from hash and returned. Otherwise, a null pointer is returned and hash is
> unchanged.
>
> The caller is responsible for deallocating any resources associated with the
> returned element, as appropriate. For example, if the hash table elements are
> dynamically allocated using `malloc()`, then the caller must `free()` the
> element after it is no longer needed.

## Iteration Functions

These functions allow iterating through the elements in a hash table. Two
interfaces are supplied. The first requires writing and supplying a
hash_action_func to act on each element.

---

```c
void hash_apply (struct hash *hash, hash action func *action);
```

> Calls action once for each element in hash, in arbitrary order. action must
> not call any function that may modify the hash table, such as `hash_insert()`
> or `hash_delete()`. action must not modify key data in elements, although it
> may modify any other data.

The second interface is based on an "iterator" data type. Idiomatically,
iterators are used as follows:

---

```c
struct hash_iterator i;
hash_first (&i, h);
while (hash_next (&i)) {
    struct foo *f = hash_entry (hash_cur (&i), struct foo, elem);
    . . . do something with f . . .
}
```

---

```c
struct hash_iterator;
```

> Represents a position within a hash table. Calling any function that may
> modify a hash table, such as `hash_insert()` or `hash_delete()`, invalidates
> all iterators within that hash table.
>
> Like struct hash and struct hash_elem, struct hash_elem is opaque.

---

```c
void hash_first (struct hash iterator *iterator, struct hash *hash);
```

> Initializes iterator to just before the first element in hash.

```c
struct hash_elem *hash_next (struct hash iterator *iterator);
```

> Advances iterator to the next element in hash, and returns that element.
> Returns a null pointer if no elements remain. After `hash_next()` returns null
> for iterator, calling it again yields undefined behavior.

---

```c
struct hash_elem *hash_cur (struct hash iterator *iterator);
```

> Returns the value most recently returned by `hash_next()` for iterator. Yields
> undefined behavior after `hash_first()` has been called on iterator but before
> `hash_next()` has been called for the first time.

## Hash Table Example

Suppose you have a structure, called `struct page`, that you want to put into a
hash table. First, define `struct page` to include a `struct hash_elem` member:

```c
struct page {
    struct hash_elem hash_elem; /* Hash table element. */
    void *addr; /* Virtual address. */
    /* . . . other members. . . */
};
```

We write a hash function and a comparison function using addr as the key. A
pointer can be hashed based on its bytes, and the `<` operator works fine for
comparing pointers:

```c
/* Returns a hash value for page p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED) {
  const struct page *p = hash_entry (p_, struct page, hash_elem);
  return hash_bytes (&p->addr, sizeof p->addr);
}
```

```c
/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_,
           const struct hash_elem *b_, void *aux UNUSED) {
  const struct page *a = hash_entry (a_, struct page, hash_elem);
  const struct page *b = hash_entry (b_, struct page, hash_elem);

  return a->addr < b->addr;
}
```

(The use of `UNUSED` in these functions' prototypes suppresses a warning that
aux is unused. See Function and Parameter Attributes, for information
about `UNUSED`. See Hash Auxiliary Data, for an explanation of aux.)

Then, we can create a hash table like this:

```c
struct hash pages;
hash_init (&pages, page_hash, page_less, NULL);
```

Now we can manipulate the hash table we've created. If `p` is a pointer to a
`struct page`, we can insert it into the hash table with:

```c
hash_insert (&pages, &p->hash_elem);
```

If there's a chance that pages might already contain a page with the same addr,
then we should check `hash_insert()'`s return value.

To search for an element in the hash table, use `hash_find()`. This takes a
little setup, because `hash_find()` takes an element to compare against. Here's
a function that will find and return a page based on a virtual address, assuming
that pages is defined at file scope:

```c
/* Returns the page containing the given virtual address, or a null pointer if no such page exists. */
struct page *
page_lookup (const void *address) {
  struct page p;
  struct hash_elem *e;

  p.addr = address;
  e = hash_find (&pages, &p.hash_elem);
  return e != NULL ? hash_entry (e, struct page, hash_elem) : NULL;
}
```

`struct page` is allocated as a local variable here on the assumption that it is
fairly small. Large structures should not be allocated as local variables.

A similar function could delete a page by address using `hash_delete()`.

## Auxiliary Data

In simple cases like the example above, there's no need for the aux parameters.
In these cases, just pass a null pointer to `hash_init()` for aux and ignore the
values passed to the hash function and comparison functions. (You'll get a
compiler warning if you don't use the aux parameter, but you can turn that off
with the `UNUSED` macro, as shown in the example, or you can just ignore it.)

***aux*** is useful when you have some property of the data in the hash table is
both constant and needed for hashing or comparison, but not stored in the data
items themselves. For example, if the items in a hash table are fixed-length
strings, but the items themselves don't indicate what that fixed length is, you
could pass the length as an aux parameter.

## Synchronization

The hash table does not do any internal synchronization. It is the caller's
responsibility to synchronize calls to hash table functions. In general, any
number of functions that examine but do not modify the hash table, such as
`hash_find()` or `hash_next()`, may execute simultaneously. However, these
function cannot safely execute at the same time as any function that may modify
a given hash table, such as `hash_insert()` or `hash_delete()`, nor may more
than one function that can modify a given hash table execute safely at once.

It is also the caller's responsibility to synchronize access to data in hash
table elements. How to synchronize access to this data depends on how it is
designed and organized, as with any other data structure.
