#!/usr/bin/env bash
set -euo pipefail

URL="https://s3.ap-northeast-2.wasabisys.com/pinto-model-zoo/133_Real-ESRGAN/Real-ESRGAN_onnx_only.tar.gz"
ARCHIVE="resources.tar.gz"

download_file() {
	if command -v curl >/dev/null 2>&1; then
		curl -L "$URL" -o "$ARCHIVE"
		return 0
	fi

	if command -v wget >/dev/null 2>&1; then
		wget -O "$ARCHIVE" "$URL"
		return 0
	fi

	echo "Error: neither curl nor wget is installed." >&2
	echo "Install one of them and retry:" >&2
	echo "  sudo apt-get update && sudo apt-get install -y curl" >&2
	echo "  or" >&2
	echo "  sudo apt-get update && sudo apt-get install -y wget" >&2
	return 1
}

download_file
tar -zxf "$ARCHIVE"
rm -f "$ARCHIVE"

echo "Download finished."
