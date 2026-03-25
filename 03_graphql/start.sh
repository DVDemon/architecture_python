#!/bin/bash
# Демонстрационный GraphQL сервис (данные в памяти, без БД)
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-/usr/local/lib}"
./build/hl_graphql