hihat=(x . x . x . x .)
snare=(. . . o . . . .)
  tom=(O . o o . o . .)
 bass=(o . . . o . . o)

eighth=0.263 # 60/114/2 seconds

instruments=${1:-t}

selected=()
for ((i=0;i<${#instruments};i++)); do
  case "${instruments:i:1}" in
    h) selected+=(hihat) ;;
    s) selected+=(snare) ;;
    t) selected+=(tom) ;;
    b) selected+=(bass) ;;
  esac
done

active=()
for name in "${selected[@]}"; do
  declare -n pattern="$name"
  for beat in "${pattern[@]}"; do
    if [[ "$beat" != "." ]]; then
      active+=("$name")
      break
    fi
  done
done

for ((bar=0;;bar++)); do
  for i in {0..7}; do
    playing=0
    for name in "${active[@]}"; do
      declare -n pattern="$name"
      if [[ "${pattern[i]}" != "." ]]; then
        playing=1
        break
      fi
    done
    if ((playing)); then
      line=""
      for name in "${active[@]}"; do
        declare -n pattern="$name"
        line+="${pattern[i]}"
      done
      printf '%s\n' "$line"
    fi
    sleep "$eighth"
  done
done
