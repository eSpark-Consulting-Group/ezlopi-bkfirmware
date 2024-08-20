#!/bin/sh

# Determine the directory of the script
SCRIPT_DIR=$(dirname "$(realpath "$0")")

# Set the output filename
header_filename="${SCRIPT_DIR}/ezlopi_util_version.h"

# Get the build date and compile time
build_date=$(date +%s) # Unix timestamp
compile_time=$(date "+%a %b %d %r %Z %Y") # Human-readable format

# Get commit hash and branch name
commit_hash=$(git rev-parse HEAD) # Full commit hash
branch_name=$(git rev-parse --abbrev-ref HEAD) # Current branch name

# Get the developer's email from Git config
developer_email=$(git config user.email)

# Write the header content
cat <<EOF > "$header_filename"
#ifndef __VERSION_H__
#define __VERSION_H__

#ifdef __cplusplus
extern "C" {
#endif

#define BUILD_DATE $build_date
#define MAJOR 4
#define MINOR 1
#define BATCH 4
#define BUILD 194
#define VERSION_STR "4.1.4"
#define DEVELOPER "$developer_email"
#define COMPILE_TIME "$compile_time"
#define CURRENT_BRANCH "$branch_name"
#define COMMIT_HASH "$commit_hash"

#ifdef __cplusplus
}
#endif

#endif // __VERSION_H__
EOF

echo "$header_filename has been created with the specified content."
