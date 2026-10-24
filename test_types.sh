
#!/bin/sh
if [ -d "$1" ]; then
    echo "Directory exists!"
else
    echo "Not a directory!"
fi

if [ -e "$1" ]; then
    echo "Path exists!"
else
    echo "Path does not exist!"
fi
