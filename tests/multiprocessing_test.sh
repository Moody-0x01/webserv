#!/bin/bash
TEST_SIZE=1000
seq $TEST_SIZE | xargs -P 98 -I {} curl -s -o /dev/null 'localhost:8080'
