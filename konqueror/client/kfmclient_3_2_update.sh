#! /bin/sh

while read ln; do
    if (echo $ln | grep -v "^StartNewKonqueror="); then
        continue
    fi
    if test "$ln" = "StartNewKonqueror=Local only"; then
        echo "SafeParts=khtml.desktop"
    elif test "$ln" = "StartNewKonqueror=Web only"; then
        echo "SafeParts=SAFE"
    elif test "$ln" = "StartNewKonqueror=Always" \
            -o "$ln" = "StartNewKonqueror=TRUE" \
            -o "$ln" = "StartNewKonqueror=true" \
            -o "$ln" = "StartNewKonqueror=1"; then
        echo "SafeParts="
    else # =Never
        echo "SafeParts=ALL"
    fi
done

echo '# DELETE StartNewKonqueror'
