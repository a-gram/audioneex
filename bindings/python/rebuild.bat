cd ..\.. && ^
build BINARY_TYPE=static && ^
cd bindings\python && ^
del audioneex\*.pyd && ^
makebind
