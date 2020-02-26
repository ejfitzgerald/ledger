#!/bin/bash
set -e

# check the number of arguments
if [ "$#" -ne 1 ] || ! [ -f "$1" ]; then
  echo "Usage: $0 <BINARY TX FILE>" >&2
  exit 1
fi

# run CURL
curl  -H "Content-Type: application/vnd.fetch-ai.transaction+bulk" --data-binary @$1 http://127.0.0.1:8000/api/contract/submit
