X=5

if [ "$X" -gt 10 ]; then
    echo LARGE
elif [ "$X" -gt 3 ]; then
    echo MEDIUM
else
    echo SMALL
fi