# Module `std::iterator`

## Table of Contents
- [Classes](#classes)
  - [`std::iterator::Iterable`](#stditeratoriterable)
    - [`iterator` \(0\)](#stditeratoriterableiterator-0)
  - [`std::iterator::Iterator`](#stditeratoriterator)
    - [`isAtEnd` \(0\)](#stditeratoriteratorisatend-0)
    - [`next` \(0\)](#stditeratoriteratornext-0)

## Classes

### `std::iterator::Iterable`
```bea
abstract class std::iterator::Iterable<T>
```
Brief: Iterable object

_Generic parameters:_
- `T`: The type of value returned by the iterator

---

#### `std::iterator::Iterable::iterator` \(0\)
```bea
abstract func iterator() -> std::iterator::Iterator<T>
```
Brief: Create an iterator for this object

_Parameters:_

_Returns:_
- `Iterator<T>`

---

### `std::iterator::Iterator`
```bea
abstract class std::iterator::Iterator<T>
```
Brief: Iterator

_Generic parameters:_
- `T`: The type of value returned by the iterator

---

#### `std::iterator::Iterator::isAtEnd` \(0\)
```bea
abstract func isAtEnd() -> Boolean
```
Brief: Check if iterator has reached end

_Parameters:_

_Returns:_
- `Boolean`

---

#### `std::iterator::Iterator::next` \(0\)
```bea
abstract func next() -> T
```
Brief: Get next item from iterator

_Parameters:_

_Returns:_
- `T`

---

