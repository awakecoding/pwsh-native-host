# PowerShell Native Host

## Getting Started

Install [PowerShell 7.2.0-preview6 or later](https://github.com/PowerShell/PowerShell/releases) for the required `LoadAssemblyFromNativeMemory` function, then export the PowerShell base installation path:

```
$Env:PWSH_BASE_PATH="${Env:ProgramFiles}\PowerShell\7-preview"
$Env:PWSH_BASE_PATH="/opt/microsoft/powershell/7-preview"
```

Build the NativeHost managed DLL containing the bindings:

```
cd NativeHost
dotnet build . -c Release
$Env:PWSH_HOST_DLL = "$PWD/bin/Release/net5.0/NativeHost.dll"
```

Build the PowerShell native host program:

```
cd native-host
cmake .
cmake --build .
```

```
cd native-host
cmake -G "Visual Studio 16 2019" -A x64 .
cmake --build . --config Release
```

Run the sample program in application and library mode:

```
.\native-host app
.\native-host lib
```

The application native host loads pwsh.exe and launches it with its command-line interface, yet it will not spawn a subprocess: it literally loads the PowerShell application to execute it within the current process. This is useful to avoid leaking sensitive data in command-line parameters.

The library native host loads the PowerShell SDK APIs and calls a few functions for testing.

## References

 * https://github.com/PowerShell/PowerShell/tree/master/docs/host-powershell
 * https://docs.microsoft.com/en-us/dotnet/core/tutorials/netcore-hosting
 * https://github.com/dotnet/runtime/blob/master/docs/design/features/native-hosting.md
 * https://github.com/dotnet/runtime/blob/master/docs/design/features/host-components.md
 * https://github.com/dotnet/runtime/blob/main/docs/design/features/host-error-codes.md
 * https://github.com/dotnet/samples/tree/master/core/hosting
 * https://github.com/dotnet/runtime/tree/master/src/coreclr/hosts
 * https://github.com/dotnet/runtime/tree/master/src/installer/corehost
 * https://github.com/dotnet/runtime/blob/master/docs/design/features/host-probing.md
 * https://github.com/dotnet/runtime/issues/35329
 * https://github.com/dotnet/runtime/issues/35465
 * https://github.com/dotnet/runtime/pull/36990
 * https://github.com/dotnet/docs/issues/16646
 * https://github.com/sanosdole/nodeclrhost
 * https://github.com/dotnet/runtime/issues/46652
 * https://github.com/PowerShell/PowerShell/issues/14641
 * https://github.com/PowerShell/PowerShell/pull/14652
 * https://keithbabinec.com/2020/02/15/how-to-run-powershell-core-scripts-from-net-core-applications/
