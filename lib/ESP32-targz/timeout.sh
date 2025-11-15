#!/bin/bash

# shopt extglob

ENV_QEMU_TIMEOUT="1200"

timeout=$ENV_QEMU_TIMEOUT

interval=1

string_to_grep="All tests performed"

log_file=logs.txt

while ((timeout > 0)); do
  sleep $interval
  grep_result=`tail ${log_file} | grep "${string_to_grep}"`
  if [[ "$grep_result" =~ $string_to_grep ]]; then
    echo "Tests complete";
    break
  fi
  ((timeout -= interval))
done

# kill qemu








