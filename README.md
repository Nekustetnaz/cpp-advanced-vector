# Custom Vector Implementation

A C++ implementation of a custom vector container that mimics the behavior of `std::vector` with additional performance tracking capabilities.

## Features

### Core Functionality
- **Dynamic Array**: Resizable array implementation with automatic memory management
- **STL Compatibility**: Implements standard iterator interfaces and container operations
- **Exception Safety**: Strong exception guarantee for most operations
- **Move Semantics**: Efficient move operations for optimized performance
- **Memory Management**: Custom raw memory handling with proper construction/destruction

### Special Operations
- **Emplace Operations**: Direct construction of elements in-place
- **Resize/Reserve**: Flexible capacity management
- **Insert/Erase**: Efficient element manipulation at arbitrary positions
- **Object Lifetime Tracking**: Built-in counters for construction/destruction operations

## Implementation Details

### Key Components
- **`RawMemory`**: Low-level memory management wrapper
  - Manual allocation/deallocation
  - Move semantics support
  - Pointer arithmetic operations
- **`Vector`**: High-level container implementation
  - Value semantics with copy/move operations
  - Iterator support (begin/end, const variants)
  - Capacity management (reserve/resize)
  - Element access/modification

### Performance Characteristics
- Amortized O(1) push_back operations
- O(n) insert/erase operations
- Optimal memory usage with geometric growth strategy
- Move semantics for efficient object transfers

## Testing Framework

The implementation includes comprehensive test cases that verify:
- Basic functionality (construction, assignment, access)
- Exception safety guarantees
- Move semantics correctness
- Iterator validity
- Object lifetime management
- Edge case handling

### Test Categories
1. **Basic Operations**: Capacity, size, element access
2. **Exception Handling**: Construction failure scenarios
3. **Move Semantics**: Transfer of ownership tests
4. **Resize Operations**: Size adjustment behavior
5. **Emplace/Insert**: In-place construction
6. **Benchmarking**: Performance comparison with std::vector
