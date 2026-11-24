# Module `std::result`

## Table of Contents
- [Classes](#classes)
  - [`std::result::Error`](#stdresulterror)
    - [`init` \(0\)](#stdresulterrorinit-0)
  - [`std::result::Result`](#stdresultresult)
    - [`init` \(0\)](#stdresultresultinit-0)
    - [`init` \(1\)](#stdresultresultinit-1)
    - [`init` \(2\)](#stdresultresultinit-2)

## Classes

### `std::result::Error`
```bea
class std::result::Error
```
Brief: Common error

_Fields:_
- `message: std::string::String`: Error message

---

#### `std::result::Error::init` \(0\)
```bea
func init(message: std::string::String) -> std::none::None
```
Brief: Construct error

_Parameters:_
- `message: std::string::String`: The error message

_Returns:_
- `std::none::None`

---

### `std::result::Result`
```bea
struct std::result::Result<T>
```
Brief: Common result

_Generic parameters:_
- `T`: The type of value carried by the result

_Fields:_
- `value: T`: The value carried by the result
- `error: std::result::Error`: The optional error carried by the result
- `okay: Boolean`: Indicates if the result is a success

---

#### `std::result::Result::init` \(0\)
```bea
func init() -> std::none::None
```
Brief: Construct result with default value

_Parameters:_

_Returns:_
- `std::none::None`

---

#### `std::result::Result::init` \(1\)
```bea
func init(value: T) -> std::none::None
```
Brief: Construct result with value

_Parameters:_
- `value: T`: The value

_Returns:_
- `std::none::None`

---

#### `std::result::Result::init` \(2\)
```bea
func init(error: std::result::Error) -> std::none::None
```
Brief: Construct fail result with error

_Parameters:_
- `error: Error`: The error

_Returns:_
- `std::none::None`

---

