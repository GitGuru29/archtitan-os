if status is-interactive
    # Commands to run in interactive sessions can go here
    fastfetch
    starship init fish | source
    
    # Aliases
    alias ls='ls --color=auto'
    alias ll='ls -l'
    alias la='ls -la'
    alias grep='grep --color=auto'
    alias update='sudo pacman -Syu'
    alias install='sudo pacman -S'
    alias remove='sudo pacman -Rns'
    alias gs='git status'
    alias gc='git commit'
    alias ga='git add'
    alias gp='git push'
    alias gl='git pull'
    
    # Docker
    alias d='docker'
    alias dc='docker-compose'
    
    # Editor
    alias e='nvim'
    alias c='code'
end
