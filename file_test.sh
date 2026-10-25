if [ -f "nested.sh" ]; then
    echo FILE_EXISTS
else
    echo FILE_MISSING
fi

if [ -d "." ]; then
    echo DIRECTORY_EXISTS
else
    echo DIRECTORY_MISSING
fi