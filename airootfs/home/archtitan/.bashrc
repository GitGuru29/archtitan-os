#
# ~/.bashrc
#

# If not running interactively, don't do anything
[[ $- != *i* ]] && return

alias ls='ls --color=auto'
alias ll='ls -l --color=auto'
alias grep='grep --color=auto'
alias update='sudo pacman -Syu'
alias install='sudo pacman -S'
alias remove='sudo pacman -Rns'

# Editor
export EDITOR='nvim'
export VISUAL='nvim'

# Run fastfetch on startup
fastfetch

# Starship prompt (if installed)
if command -v starship &> /dev/null; then
    eval "$(starship init bash)"
else
    PS1='[\u@\h \W]\$ '
fi
