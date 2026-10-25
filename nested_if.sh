if [ 1 -eq 1 ]; then
    echo OUTER_IF

    if [ 2 -gt 1 ]; then
        echo INNER_IF
    else
        echo INNER_ELSE
    fi

else
    echo OUTER_ELSE
fi