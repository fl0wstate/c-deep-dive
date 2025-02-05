#!/usr/bin/env bash

# this will make a list
selected=$(find ~/$@ -mindepth 1 -maxdepth 1 -type d | fzf)
if [[ -z "$selected" ]]; then
  exit 0
fi
# strips out path of the files and gets the basename
selected_name=$(basename $selected | tr ".,: " "____")

# echo "selected!! $selected -- selected_name $selected_name"
switch-to() {
  if [[ -z "$TMUX" ]]; then
    tmux attach-session -t $selected_name
  else
    tmux switch-client -t $selected_name
  fi
}

if tmux has-session -t="$selected_name"; then
  switch-to
else
  tmux new-session -ds "$selected_name" -c "$selected"
  switch-to

  # some personal command that will help you out
  tmux send-keys -t "$selected_name" "Tmux is ready"
  tmux send-keys -t "$selected_name" "make scripts for me to execute"
fi
