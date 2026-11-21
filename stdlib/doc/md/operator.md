# Module `std::operator`

## Table of Contents
- [Classes](#classes)
  - [`std::operator::Add`](#stdoperatoradd)
    - [`add` \(0\)](#stdoperatoraddadd-0)
    - [`preIncrement` \(0\)](#stdoperatoraddpreincrement-0)
    - [`postIncrement` \(0\)](#stdoperatoraddpostincrement-0)
    - [`preDecrement` \(0\)](#stdoperatoraddpredecrement-0)
    - [`postDecrement` \(0\)](#stdoperatoraddpostdecrement-0)
  - [`std::operator::Subtract`](#stdoperatorsubtract)
    - [`subtract` \(0\)](#stdoperatorsubtractsubtract-0)
    - [`negate` \(0\)](#stdoperatorsubtractnegate-0)
  - [`std::operator::Multiply`](#stdoperatormultiply)
    - [`multiply` \(0\)](#stdoperatormultiplymultiply-0)
  - [`std::operator::Divide`](#stdoperatordivide)
    - [`divide` \(0\)](#stdoperatordividedivide-0)
  - [`std::operator::Modulo`](#stdoperatormodulo)
    - [`modulo` \(0\)](#stdoperatormodulomodulo-0)
  - [`std::operator::BitwiseOr`](#stdoperatorbitwiseor)
    - [`bitwiseOr` \(0\)](#stdoperatorbitwiseorbitwiseor-0)
  - [`std::operator::BitwiseAnd`](#stdoperatorbitwiseand)
    - [`bitwiseAnd` \(0\)](#stdoperatorbitwiseandbitwiseand-0)
  - [`std::operator::BitwiseXor`](#stdoperatorbitwisexor)
    - [`bitwiseXor` \(0\)](#stdoperatorbitwisexorbitwisexor-0)
    - [`bitwiseNot` \(0\)](#stdoperatorbitwisexorbitwisenot-0)
  - [`std::operator::BitwiseLeftShift`](#stdoperatorbitwiseleftshift)
    - [`bitwiseLeftShift` \(0\)](#stdoperatorbitwiseleftshiftbitwiseleftshift-0)
  - [`std::operator::BitwiseRightShift`](#stdoperatorbitwiserightshift)
    - [`bitwiseRightShift` \(0\)](#stdoperatorbitwiserightshiftbitwiserightshift-0)
  - [`std::operator::Equals`](#stdoperatorequals)
    - [`equals` \(0\)](#stdoperatorequalsequals-0)
  - [`std::operator::LessThan`](#stdoperatorlessthan)
    - [`lessThan` \(0\)](#stdoperatorlessthanlessthan-0)
  - [`std::operator::GreaterThan`](#stdoperatorgreaterthan)
    - [`greaterThan` \(0\)](#stdoperatorgreaterthangreaterthan-0)
  - [`std::operator::True`](#stdoperatortrue)
    - [`isTrue` \(0\)](#stdoperatortrueistrue-0)
  - [`std::operator::Index`](#stdoperatorindex)
    - [`get` \(0\)](#stdoperatorindexget-0)
    - [`set` \(0\)](#stdoperatorindexset-0)

## Classes

### `std::operator::Add`
```bea
abstract class std::operator::Add
```
Brief: Addition operator

---

#### `std::operator::Add::add` \(0\)
```bea
abstract func add(other: std::operator::Add) -> std::operator::Add
```
Brief: Add values

_Parameters:_
- `other: Add`: Value to add to self

_Returns:_
- `Add`

---

#### `std::operator::Add::preIncrement` \(0\)
```bea
abstract func preIncrement() -> std::operator::Add
```
Brief: Prefix increment value

_Parameters:_

_Returns:_
- `Add`

---

#### `std::operator::Add::postIncrement` \(0\)
```bea
abstract func postIncrement() -> std::operator::Add
```
Brief: Postfix increment value

_Parameters:_

_Returns:_
- `Add`

---

#### `std::operator::Add::preDecrement` \(0\)
```bea
abstract func preDecrement() -> std::operator::Add
```
Brief: Prefix decrement value

_Parameters:_

_Returns:_
- `Add`

---

#### `std::operator::Add::postDecrement` \(0\)
```bea
abstract func postDecrement() -> std::operator::Add
```
Brief: Postfix decrement value

_Parameters:_

_Returns:_
- `Add`

---

### `std::operator::Subtract`
```bea
abstract class std::operator::Subtract
```
Brief: Subtraction operator

---

#### `std::operator::Subtract::subtract` \(0\)
```bea
abstract func subtract(other: std::operator::Subtract) -> std::operator::Subtract
```
Brief: Subtract values

_Parameters:_
- `other: Subtract`: Value to subtract from self

_Returns:_
- `Subtract`

---

#### `std::operator::Subtract::negate` \(0\)
```bea
abstract func negate() -> std::operator::Subtract
```
Brief: Negate value

_Parameters:_

_Returns:_
- `Subtract`

---

### `std::operator::Multiply`
```bea
abstract class std::operator::Multiply
```
Brief: Multiplication operator

---

#### `std::operator::Multiply::multiply` \(0\)
```bea
abstract func multiply(other: std::operator::Multiply) -> std::operator::Multiply
```
Brief: Multiply values

_Parameters:_
- `other: Multiply`: Value to multiply by self

_Returns:_
- `Multiply`

---

### `std::operator::Divide`
```bea
abstract class std::operator::Divide
```
Brief: Division operator

---

#### `std::operator::Divide::divide` \(0\)
```bea
abstract func divide(other: std::operator::Divide) -> std::operator::Divide
```
Brief: Divide values

_Parameters:_
- `other: Divide`: Value to divide self by

_Returns:_
- `Divide`

---

### `std::operator::Modulo`
```bea
abstract class std::operator::Modulo
```
Brief: Modulus operator

---

#### `std::operator::Modulo::modulo` \(0\)
```bea
abstract func modulo(other: std::operator::Modulo) -> std::operator::Modulo
```
Brief: Get remainder of division operation

_Parameters:_
- `other: Modulo`: Value to divide self by

_Returns:_
- `Modulo`

---

### `std::operator::BitwiseOr`
```bea
abstract class std::operator::BitwiseOr
```
Brief: Bitwise or operator

---

#### `std::operator::BitwiseOr::bitwiseOr` \(0\)
```bea
abstract func bitwiseOr(other: std::operator::BitwiseOr) -> std::operator::BitwiseOr
```
Brief: Bitwise or values

_Parameters:_
- `other: BitwiseOr`: Value to bitwise or with

_Returns:_
- `BitwiseOr`

---

### `std::operator::BitwiseAnd`
```bea
abstract class std::operator::BitwiseAnd
```
Brief: Bitwise and operator

---

#### `std::operator::BitwiseAnd::bitwiseAnd` \(0\)
```bea
abstract func bitwiseAnd(other: std::operator::BitwiseAnd) -> std::operator::BitwiseAnd
```
Brief: Bitwise and values

_Parameters:_
- `other: BitwiseAnd`: Value to bitwise and with

_Returns:_
- `BitwiseAnd`

---

### `std::operator::BitwiseXor`
```bea
abstract class std::operator::BitwiseXor
```
Brief: Bitwise xor operator

---

#### `std::operator::BitwiseXor::bitwiseXor` \(0\)
```bea
abstract func bitwiseXor(other: std::operator::BitwiseXor) -> std::operator::BitwiseXor
```
Brief: Bitwise xor values

_Parameters:_
- `other: BitwiseXor`: Value to bitwise xor with

_Returns:_
- `BitwiseXor`

---

#### `std::operator::BitwiseXor::bitwiseNot` \(0\)
```bea
abstract func bitwiseNot() -> std::operator::BitwiseXor
```
Brief: Bitwise not value (Equivalent to xor'ing by all ones)

_Parameters:_

_Returns:_
- `BitwiseXor`

---

### `std::operator::BitwiseLeftShift`
```bea
abstract class std::operator::BitwiseLeftShift
```
Brief: Bitwise left shift operator

---

#### `std::operator::BitwiseLeftShift::bitwiseLeftShift` \(0\)
```bea
abstract func bitwiseLeftShift(other: std::operator::BitwiseLeftShift) -> std::operator::BitwiseLeftShift
```
Brief: Bitwise left shift value

_Parameters:_
- `other: BitwiseLeftShift`: Value to bitwise left shift by

_Returns:_
- `BitwiseLeftShift`

---

### `std::operator::BitwiseRightShift`
```bea
abstract class std::operator::BitwiseRightShift
```
Brief: Bitwise right shift operator

---

#### `std::operator::BitwiseRightShift::bitwiseRightShift` \(0\)
```bea
abstract func bitwiseRightShift(other: std::operator::BitwiseRightShift) -> std::operator::BitwiseRightShift
```
Brief: Bitwise right shift value

_Parameters:_
- `other: BitwiseRightShift`: Value to bitwise right shift by

_Returns:_
- `BitwiseRightShift`

---

### `std::operator::Equals`
```bea
abstract class std::operator::Equals
```
Brief: Equals operator

---

#### `std::operator::Equals::equals` \(0\)
```bea
abstract func equals(other: std::operator::Equals) -> Boolean
```
Brief: Compare equality of values

_Parameters:_
- `other: Equals`: Value to compare with

_Returns:_
- `Boolean`

---

### `std::operator::LessThan`
```bea
abstract class std::operator::LessThan
```
Brief: Less than operator

---

#### `std::operator::LessThan::lessThan` \(0\)
```bea
abstract func lessThan(other: std::operator::LessThan) -> Boolean
```
Brief: Compare order of values

_Parameters:_
- `other: LessThan`: Value to compare with

_Returns:_
- `Boolean`

---

### `std::operator::GreaterThan`
```bea
abstract class std::operator::GreaterThan
```
Brief: Greater than operator

---

#### `std::operator::GreaterThan::greaterThan` \(0\)
```bea
abstract func greaterThan(other: std::operator::GreaterThan) -> Boolean
```
Brief: Compare order of values

_Parameters:_
- `other: Equals`: Value to compare with

_Returns:_
- `Boolean`

---

### `std::operator::True`
```bea
abstract class std::operator::True
```
Brief: Truthiness operator

---

#### `std::operator::True::isTrue` \(0\)
```bea
abstract func isTrue() -> Boolean
```
Brief: Get truthiness of value

_Parameters:_

_Returns:_
- `Boolean`

---

### `std::operator::Index`
```bea
abstract class std::operator::Index<K, V>
```
Brief: Index operator

_Generic parameters:_
- `K`: The type of key used as an index
- `V`: The type of value stored by the container

---

#### `std::operator::Index::get` \(0\)
```bea
abstract func get(index: K) -> V
```
Brief: Get value at index

This function should make use of assertions for bounds checks.

_Parameters:_
- `index: K`: The index of the value

_Returns:_
- `V`

---

#### `std::operator::Index::set` \(0\)
```bea
abstract func set(index: K, value: V) -> V
```
Brief: Set value at index

This function should make use of assertions for bounds checks.

_Parameters:_
- `index: K`: The index of the value
- `value: V`: The value to set

_Returns:_
- `V`: The value assigned

---

