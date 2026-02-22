# Backman
Simple backup manager written in c++

Not ready for production use, despite me using it so

## Building
```$ cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -B build```

```$ cmake --build ./build```

```# cmake --install build```

## Example config
See the [example config](example_config.ini)

## Known Issues
- The ini parser is ill fitted for anything but ascii, and has very little restrictions for ensuring sensible outputs
- Not much error handling

## Planned Features for v0.2
- ~Add support for custom tar flags~ Complete
- ~Support more than just xz compression~ Complete
- Verbosity controls
- Phony targets
- --keep-going to continue past failures (well, currently it is 50/50 --keep-going and not, and there is no control)
- Some general refactoring to make things easier

## Major TODO's
- Rewrite the garbage code for running everything
- Abstract away argument parsing to a custom library

## Hopes and Dreams
- Save file over ssh to remote or some other kind of saving to a remote
