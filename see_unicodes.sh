while IFS= read -d $'\000' -n 1 x; do printf '%2s -> UTF-16: [%04X], [%b]\n' "$x" "'$x" "$x"; done
