# serialControl SDK

## Brief
serialControl SDK is the software development kit to access serial port

**Note**:
> So far, only Windows is supported.

## Build and run the example code
The following steps instruct you to building and running the example code:

1. Open `script/msvc/serialControl.sln` in VisualStudio 2017. So far, the solution includes two projects which 
is named `serialTest` and `serialSDK`.

2. Build the project `serialTest`, and the executable will be generated in the `bin` directory and the 
static library in the `lib` directory.

3. Run the executable file of previously built `serialTest`, different messages will be display depends on if the serial device is connected or `com port` is correctly.


## Known issues
1. Never put the member function of  `SerialAdapter` in `SerialListener`.
2. `x64` setting is not supported
3. [serialControl][serialControl] is in early version,the public API is unstable.

[serialControl]: https://www.github.com/zhoukaisspu/serialControl