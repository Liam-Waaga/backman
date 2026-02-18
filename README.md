# Backman
Simple backup manager written in c++

## Building
```cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -B build```

```cmake --build ./build```

```cmake --install build```

## Example config
See the [example config](example_config.ini)

## Known Issues
- The ini parser is ill fitted for anything but ascii, and has very little restrictions for ensuring sensible outputs
- Not much error handling
