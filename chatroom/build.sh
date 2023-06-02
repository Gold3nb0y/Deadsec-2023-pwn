#!/bin/bash

gcc client.c -o bin/client -lpthread -lrt
gcc server.c -o bin/server -lpthread -lrt
