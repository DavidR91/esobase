# esobase
Super minimal interpreted language (intended as an [esolang](https://esolangs.org/wiki/Main_Page))

esobase is not an esolang in the sense it is a joke or intentionally annoying - it is just intended to be exceptionally minimal for ease of parsing. Inspirations are [K](https://en.wikipedia.org/wiki/K_(programming_language)) and [ORCA](https://github.com/hundredrabbits/Orca)

esobase's name comes from intending it to be a 'base' to build more meaningful language experimentation. It is work in progress and a proof of concept: it is not performant or efficient.

## Example
```
ml 4 16; 4 16; mb + mm a md s mm f
```

Add together 16 and 16 and allocate a memory buffer to the resulting size. Display the stack and then free the allocated memory

## Syntax
esobase is heavily stack oriented (inspired by Lua) and works by switching between several top level modes

| Code  |  Meaning |
|---|---|
| `l`  |  Literals  |
| `b`  |  Boolean operations |
| `m`  |  Memory |
| `s` |  Stack  |
| `d` | Debug  |

For example `md` switches to debug mode. Spaces have no meaning: `m    d` would be equivalent.

### Literals mode
`ml`

Push literals onto the stack.

| Code  |  Meaning |
|---|---|
| `+` | Toggle signed/unsigned mode. Unsigned is the default. _Signed values are not yet fully implemented_ |
| `?` | Boolean. Value is the following character - either `t`/`f` or `y`/`n`|
| `1` | Single byte. The value is built from the next character and every character after it until an `;` is encountered. For example `1123;` builds the byte value `123` |
| `2` | 16 bit integer. The value is built from the next character and every character after it until an `;` is encountered. For example `2123;` builds the int16 value `123` |
| `4` | 32 bit integer. The value is built from the next character and every character after it until an `;` is encountered. For example `4123;` builds the int32 value `123` |
| `8` | 64 bit integer. The value is built from the next character and every character after it until an `;` is encountered. For example `8123;` builds the int64 value `123` |
| `f` | 32 bit float. The value is built from the next character and every character after it until an `;` is encountered. For example `f123;` builds the float value `123` |
| `d` | 64 bit float (double). The value is built from the next character and every character after it until an `;` is encountered. For example `d123;` builds the double value `123` |



### Boolean mode
`mb`

| Code  |  Meaning |
|---|---|
| `+`  |  Add two numeric items on the top of the stack together and push the result onto the stack. Must be the same type |
| `a` | AND two booleans on the top of the stack and push the result onto the stack |
| `o` | OR two booleans on the top of the stack and push the result onto the stack |
| `!` | NOT the boolean on the top of the stack and push the result onto the stack |

### Memory mode
`mm`

| Code  |  Meaning |
|---|---|
| `a` | Allocate a byte buffer to the size of the integer value on the top of the stack (must be a `1`,`2`,`4` or `8`). The result is a `*` pushed onto the top of the stack |
| `f` | Free a piece of memory based on the value on the top of the stack (must be a `*`) |

### Debug mode
`md`

| Code  |  Meaning |
|---|---|
| `s` | Dump the content of the stack to stdout |
| `t` | Dump a trace of what insttiction is running to stdout |
| `a` | Assert that the two values on the top of the stack are equal to each other. **Only** the underlying value is compared - not the type or size. This behaviour is used to implement the test suite in the tests/ folder |

### Stack mode
`ms`

| Code  |  Meaning |
|---|---|
| `p` | Pop an item from the stack |