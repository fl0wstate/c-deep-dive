#!/usr/bin/env bash

./server/src/a.out &

env ./client/src/.env

uv run ./client/src/cli.py
