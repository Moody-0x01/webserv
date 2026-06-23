#!/usr/bin/env bash
# Usage: ./send_file.sh <file_path> <url>
set -euo pipefail

FILE="${1:-}"
URL="${2:-}/$FILE"

if [[ -z "$FILE" || -z "$URL" ]]; then
  echo "Usage: $0 <file_path> <url>"
  exit 1
fi

if [[ ! -f "$FILE" ]]; then
  echo "Error: File '$FILE' not found."
  exit 1
fi

echo "Sending '$(basename "$FILE")' to '$URL'..."

HTTP_STATUS=$(curl \
  --silent \
  --output /dev/null \
  --write-out "%{http_code}" \
  --request POST \
  --header "Transfer-Encoding: chunked" \
  --header "Content-Type: application/octet-stream" \
  --data-binary "@${FILE}" \
  "$URL")

if [[ "$HTTP_STATUS" -ge 200 && "$HTTP_STATUS" -lt 300 ]]; then
  echo "✓ File sent successfully (HTTP $HTTP_STATUS)."
  exit 0
else
  echo "✗ Upload failed (HTTP $HTTP_STATUS)."
  exit 1
fi
