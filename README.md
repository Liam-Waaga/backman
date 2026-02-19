# Backman
Simple backup manager written in c++

## Building
```$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -B build```

```$ cmake --build ./build```

```# cmake --install build```

## Example config
See the [example config](example_config.ini)

## Known Issues
- The ini parser is ill fitted for anything but ascii, and has very little restrictions for ensuring sensible outputs
- Not much error handling

## Major TODO's
- Rewrite the garbage code for running everything
- Support more than just xz compression
- Add support for custom tar flags
- Abstract away argument parsing to a custom library
- 

## Hopes and Dreams
- Save file over ssh to remote or some other kind of saving to a remote
