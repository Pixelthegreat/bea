# Module `std::range`

## Table of Contents
- [Classes](#classes)
  - [`std::range::Range`](#stdrangerange)
    - [`init` \(0\)](#stdrangerangeinit-0)
    - [`init` \(1\)](#stdrangerangeinit-1)
    - [`init` \(2\)](#stdrangerangeinit-2)

## Classes

### `std::range::Range`
```bea
class std::range::Range
```
_Implements:_
- `std::iterator::Iterable<Int>`
- `std::iterator::Iterator<Int>`

Brief: Range iterator

Ranges can either be constructed like `new Range(0, 10)`, or they can be constructed using the builtin range syntax like `0..10`.

_Fields:_
- `start: Int`: The start of the range
- `end: Int`: The end of the range
- `step: Int`: The step of the range (default is 1)
- `position: Int`: The current position of the range
#### `std::range::Range::init` \(0\)
```bea
func init(end: Int) -> std::none::None
```
Brief: Construct range with end

_Parameters:_
- `end: Int`: The end value of the range

_Returns:_
- `std::none::None`

#### `std::range::Range::init` \(1\)
```bea
func init(start: Int, end: Int) -> std::none::None
```
Brief: Construct range with start and end

_Parameters:_
- `start: Int`: The start value of the range
- `end: Int`: The end value of the range

_Returns:_
- `std::none::None`

#### `std::range::Range::init` \(2\)
```bea
func init(start: Int, end: Int, step: Int) -> std::none::None
```
Brief: Construct range with start, end and step

_Parameters:_
- `start: Int`: The start value of the range
- `end: Int`: The end value of the range
- `step: Int`: The step value of the range

_Returns:_
- `std::none::None`

