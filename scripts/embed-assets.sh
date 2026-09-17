#!/bin/sh
set -eu

output=${1:?output path required}
index_file=${2:?index path required}
style_file=${3:?style path required}

{
    printf '%s\n' '#include "embedded_assets.h"'
    xxd -i "$index_file" | sed 's/assets_web_index_html/embedded_index_html/g'
    xxd -i "$style_file" | sed 's/assets_web_style_css/embedded_style_css/g'
} > "$output"
