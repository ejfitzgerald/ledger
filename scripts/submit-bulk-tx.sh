#!/bin/bash
set -e

curl  -H "Content-Type: application/vnd.fetch-ai.transaction+bulk" --data-binary @$1 http://127.0.0.1:8000/api/contract/submit
