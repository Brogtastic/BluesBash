@echo off
rem alway start with the correct working directory
cd %~dp0

pushd bin

start "Game" "BluesBash.exe"
start "Server" "BluesBash_Server.exe"

popd
