# !/bin/bash
eval $(find ./autobuild/filogic -type f -regex './autobuild/filogic/.*/prepare.sh' | sed -e 's/\(^.*prepare.sh\)/source \1;/g')
