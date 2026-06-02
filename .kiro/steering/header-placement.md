# Header File Placement

## Production Logic Headers

Pure logic headers that are included by production Arduino modules (`.cpp` or `.ino` files) belong in the `include/` directory at the project root.

These headers contain inline functions with no Arduino dependencies, making them testable on the host while also being compiled into the device firmware.

Examples: `include/running_average_logic.h`, `include/ntp_format.h`, `include/temperature_convert.h`

## Test-Only Headers

Headers that are only included by test files (`test/*.cpp`) stay in the `test/` directory.

This includes headers whose logic is not called by any production module — even if they define structs or functions that mirror production behavior. If the production `.cpp` file implements its own logic rather than including the header, the header is test-only.

Examples: `test/reading_cache_logic.h`, `test/json_serializer.h`, `test/response_parser.h`

## Rule of Thumb

If a header is `#include`d by any file outside of `test/`, it goes in `include/`. Otherwise it stays in `test/`.
